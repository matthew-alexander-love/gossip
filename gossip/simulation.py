"""
GossipSampling
Copyright (C) Matthew Love 2024 (gossipsampling@gmail.com)

This program is free software: you can redistribute it and/or modify
it under the terms of the GNU General Public License as published by
the Free Software Foundation, either version 3 of the License, or
(at your option) any later version.

This program is distributed in the hope that it will be useful,
but WITHOUT ANY WARRANTY; without even the implied warranty of
MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
GNU General Public License for more details.

You should have received a copy of the GNU General Public License
along with this program.  If not, see <http://www.gnu.org/licenses/>.
"""

from gossip import _gossip
from gossip import stats
import pandas as pd
import threading
import time
from typing import Callable
from pathlib import Path
import networkx as nx
import json
import random



def add_to_registry(registry, node_type, node):
    if node_type in registry:
        registry[node_type].append(node)
    else:
        registry[node_type] = [node]


class PandasLog(_gossip.TSLog):
    def __init__(self):
        _gossip.TSLog.__init__(self)

        self.lock = threading.Lock()
        self.df = pd.DataFrame(columns=['id', 'selected', 'time [ms]'])
    
    def push_back(self, id: str, selected: str, time: int) -> None:
        new_row = pd.DataFrame({"id": [id], 'selected': [selected], 'time [ms]': [time]})
        with self.lock:
            self.df = pd.concat([self.df, new_row], ignore_index=True)
    
    def copy_data(self) -> pd.DataFrame:
        with self.lock:
            return self.df.copy()  # Return a deep copy of the DataFrame
    
    def to_string(self) -> None:
        with self.lock:
            print(self.df)



class SimNode:
    def __init__(self, name: str, pss: _gossip.PeerSamplingService, view: _gossip.View, log: _gossip.TSLog, 
                 func: Callable[[_gossip.PeerSamplingService, _gossip.View, _gossip.TSLog, threading.Event], None],
                 entry_times: dict[str, int], exit_times: dict[str, int]):
        self.name = name
        self.address = view.self().address
        self.pss = pss
        self.view = view
        self.log = log
        self.func = func
        self._thread = None
        self._stop_event = threading.Event()
        self.entry_times = entry_times
        self.exit_times = exit_times

    def start(self):
        self.entry_times[self.address] = int(time.time() * 1000)
        self.pss.enter()
        self.pss.start()

        if self._thread is None or not self._thread.is_alive():
            self._stop_event.clear()
            self._thread = threading.Thread(target=self.func, args=(self.pss, self.view, self.log, self._stop_event))
            self._thread.start()

    def signal(self):
        if self._thread is not None:
            self._stop_event.set()
        self.pss.signal()
        if self.address not in self.exit_times:
            self.exit_times[self.address] = int(time.time() * 1000)

    def stop(self):
        self.signal()
        if self._thread is not None:
            self._stop_event.set()
            self._thread.join()

        self.pss.stop()

    def manual_insert(self, node) -> None:
        nd = _gossip.NodeDescriptor(node.view.self().address, 0)
        self.view.manual_insert(nd)



class NodeSchema():
    def __init__(self, name: str,
                 push: bool, pull: bool, wait_time: int, timeout: int,
                 func: Callable[[_gossip.PeerSamplingService, _gossip.View, _gossip.TSLog, threading.Event], None],
                 view_type: _gossip.View, selector_type: _gossip.SelectorType, **view_args):
        
        self.name = name
        self.push = push
        self.pull = pull
        self.wait_time = wait_time
        self.timeout = timeout
        self.view_type = view_type
        self.selector_type = selector_type
        self.view_args = view_args
        self.func = func
        

    def gen_node(self, address: str, entry_times: dict[str, int], exit_times: dict[str, int], entry_points: list[str]=[]) -> SimNode:
        log = PandasLog()
        view = self.view_type(address=address, **self.view_args)
        view.init_selector(self.selector_type, log)
        pss = _gossip.PeerSamplingService(push=self.push, pull=self.pull, 
                                           wait_time=self.wait_time, timeout=self.timeout,
                                           entry_points=entry_points, view=view)
        return SimNode(self.name, pss, view, log, self.func, entry_times, exit_times)



class TopologyConstructor():
    def __init__(self, name: str):
        self.name = name
        self.node_schema_registry = {}

        self.entry_port = ""
        self.entry_node_addresses = []
        self.entry_entered = {}

        self.node_port = ""
        self.node_entered = {}

        self.entry_time = {}
        self.exit_time = {}
        
    
    def to_json(self) -> dict:
        return {self.name: {}}
    
    def pass_sim_var(self, entry_node_addresses: list[str], node_schema_registry: dict[str, NodeSchema], 
                     entry_entered: dict[str, list[SimNode]], node_entered: dict[str, list[SimNode]],
                     entry_times: dict[str, int], exit_times: dict[str, int],
                     entry_removed: dict[str, list[SimNode]], node_removed: dict[str, list[SimNode]]):
        
        self.node_schema_registry = node_schema_registry

        self.entry_node_addresses = entry_node_addresses
        self.entry_removed = entry_removed
        self.entry_entered = entry_entered
        
        self.node_removed = node_removed
        self.node_entered = node_entered
        
        self.entry_times = entry_times
        self.exit_times = exit_times
    
    def schema_in_registry(self) -> bool:
        if self.entry_entered is not None:
            for n in self.entry_entered:
                if n not in self.node_schema_registry:
                    return False
        if self.node_entered is not None:
            for n in self.node_entered:
                if n not in self.node_schema_registry:
                    return False
        return True

    def _make_entry_node(self, node_type: str) -> SimNode:
        address = "0.0.0.0:" + self.entry_port
        node = self.node_schema_registry[node_type].gen_node(address, entry_times=self.entry_times, exit_times=self.exit_times)
        add_to_registry(self.entry_entered, node_type, node)
        self.entry_node_addresses.append(address)
        self.entry_port = str(int(self.entry_port) + 1)
        return node

    def _make_node(self, node_type: str, pass_entry: bool=True) -> SimNode:
        address = "0.0.0.0:" + self.node_port
        if pass_entry:
            node = self.node_schema_registry[node_type].gen_node(address, entry_times=self.entry_times, exit_times=self.exit_times, entry_points=self.entry_node_addresses)
        else:
            node = self.node_schema_registry[node_type].gen_node(address, entry_times=self.entry_times, exit_times=self.exit_times)
        add_to_registry(self.node_entered, node_type, node)
        self.node_port = str(int(self.node_port) + 1)
        return node
    
    
    def _signal_entry_node(self, node_type):
        if node_type in self.entry_entered:
            if len(self.entry_entered[node_type]):
                instance = self.entry_entered[node_type][0]
                self.entry_entered[node_type].remove(instance)
                add_to_registry(self.entry_removed, node_type, instance)
                self.entry_node_addresses.remove(instance.address)
                instance.signal()
                return instance
        return None
    
    def _signal_node(self, node_type) -> SimNode:
        if node_type in self.node_entered:
            if len(self.node_entered[node_type]):
                instance = self.node_entered[node_type][0]
                self.node_entered[node_type].remove(instance)
                add_to_registry(self.entry_removed, node_type, instance)
                instance.signal()
                return instance
        return None
    
    def _stop_entry_node(self, node_type) -> SimNode:
        node = self._signal_entry_node(node_type)
        if node is not None:
            node.stop()
        return node
    
    def _stop_node(self, node_type) -> SimNode:
        node = self._signal_node(node_type)
        if node is not None:
            node.stop()
        return node
        return None

    def num_entry_type(self, node_type: str) -> int:
        if node_type in self.entry_entered:
            return len(self.entry_entered[node_type])
        return 0
    
    def num_node_type(self, node_type: str) -> int:
        if node_type in self.node_entered:
            return len(self.node_entered[node_type])
        return 0

    def num_type(self, node_type: str) -> int:
        return self.num_entry_type(node_type) + self.num_node_type(node_type)
    

class ThreadedTopologyConstructor(TopologyConstructor):
    def __init__(self, name):
        super().__init__(name=name)
        self.lock = threading.Lock()
        self.stop_event = threading.Event()
        self.threads = []

    def _remove_entry_node(self, node_type: str, num_rem: int, wait_time: int):
        while not self.stop_event.is_set():
            with self.lock:
                for i in range(num_rem):
                    self._signal_entry_node(node_type)
            time.sleep(wait_time)

    def _remove_node(self, node_type: str, num_rem: int, wait_time: int):
        while not self.stop_event.is_set():
            with self.lock:
                for i in range(num_rem):
                    self._signal_node(node_type)
            time.sleep(wait_time)

    def _add_entry_node(self, node_type: str, num_add: int, wait_time: int):
        while not self.stop_event.is_set():
            with self.lock:
                for i in range(num_add):
                    node = self._make_entry_node(node_type)
                    node.start()
            time.sleep(wait_time)

    def _add_node(self, node_type: str, num_add: int, wait_time: int):
        while not self.stop_event.is_set():
            with self.lock:
                for i in range(num_add):
                    node = self._make_node(node_type)
                    node.start()
            time.sleep(wait_time)

    def add_nodes(self, add_rates: dict[str, tuple[int, int]]):
        self.stop_event.clear()
        for node_type, rate in add_rates.items():
            thread = threading.Thread(target=self._add_node, args=(node_type, rate[0], rate[1]), daemon=True)
            self.threads.append(thread)
            thread.start()

    def add_entry_nodes(self, add_rates: dict[str, tuple[int, int]]=None):
        self.stop_event.clear()
        for node_type, rate in add_rates.items():
            thread = threading.Thread(target=self._add_entry_node, args=(node_type, rate[0], rate[1]), daemon=True)
            self.threads.append(thread)
            thread.start()

    def remove_nodes(self, rem_rates: dict[str, tuple[int, int]]):
        self.stop_event.clear()
        for node_type, rate in rem_rates.items():
            thread = threading.Thread(target=self._remove_node, args=(node_type, rate[0], rate[1]), daemon=True)
            self.threads.append(thread)
            thread.start()

    def remove_entry_nodes(self, rem_rates: dict[str, tuple[int, int]]):
        self.stop_event.clear()
        for node_type, rate in rem_rates.items():
            thread = threading.Thread(target=self._remove_entry_node, args=(node_type, rate[0], rate[1]), daemon=True)
            self.threads.append(thread)
            thread.start()

    def stop(self):
        self.stop_event.set()
        count = 0
        for thread in self.threads:
            thread.join()
            count += 1




class RemoveRate(ThreadedTopologyConstructor):
    def __init__(self, run_time: int, rem_rates: dict[str, tuple[int, int]]=None, entry_rem_rates: dict[str, tuple[int, int]]=None):
        super().__init__(name="RemoveRate")
        self.rem_rates = rem_rates
        self.entry_rem_rates = entry_rem_rates
        self.run_time = run_time

    def to_json(self) -> dict:
        return {self.name: {"entry_rem_rates": self.entry_rem_rates, "rem_rates": self.rem_rates, "run_time": self.run_time}}
    
    def construct_graph(self, entry_port: str, node_port: str) -> tuple[str, str]:
        if self.entry_rem_rates is not None:
            self.remove_entry_nodes(self.entry_rem_rates)
        
        if self.rem_rates is not None:
            self.remove_nodes(self.rem_rates)
            
        time.sleep(self.run_time)
        self.stop()
        return entry_port, node_port
    



class ChurnRate(ThreadedTopologyConstructor):
    def __init__(self, run_time: int,
                 rem_rates: dict[str, tuple[int, int]]=None, entry_rem_rates: dict[str, tuple[int, int]]=None,  
                 add_rates: dict[str, tuple[int, int]]=None, entry_add_rates: dict[str, tuple[int, int]]=None):
        
        super().__init__(name="ChurnRate")
        self.entry_rem_rates = entry_rem_rates
        self.rem_rates = rem_rates
        self.entry_add_rates = entry_add_rates
        self.add_rates = add_rates
        self.run_time = run_time

    def to_json(self) -> dict:
        return {self.name: {"entry_rem_rates": self.entry_rem_rates, "rem_rates": self.rem_rates, 
                            "entry_add_rates": self.entry_add_rates, "add_rates": self.add_rates, 
                            "run_time": self.run_time}}   
                
    
    def construct_graph(self, entry_port: str, node_port: str) -> tuple[str, str]:
        self.entry_port = entry_port
        self.node_port = node_port

        if self.entry_rem_rates is not None:
            self.remove_entry_nodes(self.entry_rem_rates)
        
        if self.rem_rates is not None:
            self.remove_nodes(self.rem_rates)

        if self.entry_add_rates is not None:
            self.add_entry_nodes(self.entry_add_rates)

        if self.add_rates is not None:
            self.add_nodes(self.add_rates)

        time.sleep(self.run_time)
        
        self.stop()
        
        return self.entry_port, self.node_port
    


class AddRate(ThreadedTopologyConstructor):
    def __init__(self, run_time: int, add_rates: dict[str, tuple[int, int]]=None, entry_add_rates: dict[str, tuple[int, int]]=None):
        super().__init__(name="AddRate")
        self.entry_add_rates = entry_add_rates
        self.add_rates = add_rates
        self.run_time = run_time


    def to_json(self) -> dict:
        return {self.name: {"entry_add_rates": self.entry_add_rates, "add_rates": self.add_rates, "run_time": self.run_time}}
                
    
    def construct_graph(self, entry_port: str, node_port: str) -> tuple[str, str]:
        self.entry_port = entry_port
        self.node_port = node_port

        if self.entry_add_rates is not None:
            self.add_entry_nodes(self.entry_add_rates)

        if self.add_rates is not None:
            self.add_nodes(self.add_rates)

        time.sleep(self.run_time)
        self.stop()
        
        return self.entry_port, self.node_port
    


class AddDelay(TopologyConstructor):
    def __init__(self, delay_time: int):
        super().__init__("Delay")
        self.delay_time = delay_time
    
    def to_json(self) -> dict:
        return {self.name: self.delay_time}

    def construct_graph(self, entry_port: str, node_port: str) -> tuple[str, str]:
        time.sleep(self.delay_time)
        return entry_port, node_port
    


class AddLattice(TopologyConstructor):
    def __init__(self, entry_nodes: dict[str, int]=None, nodes: dict[str, int]=None, use_entry: bool=True):
        super().__init__(name="AddLattice")
        self.entry_nodes = entry_nodes
        self.nodes = nodes
        self.use_entry = use_entry


    def make_entry_node(self, node_type: str, previous: SimNode=None) -> SimNode:
        node = self._make_entry_node(node_type)
        if previous is not None:
            node.manual_insert(previous)
        return node
    

    def make_node(self, node_type: str, previous: SimNode=None) -> SimNode:
        node = self._make_node(node_type, pass_entry=self.use_entry)
        if previous is not None:
            node.manual_insert(previous)
        return node
    
        
    def find_last_port(self):
        total = 0
        port = ""
        if (self.entry_nodes is not None):
            for node_type in self.entry_nodes:
                total += self.entry_nodes[node_type]
            port = str(int(self.entry_port) + total)
        if (self.nodes is not None):
            for node_type in self.nodes:
                total += self.nodes[node_type]
            port = str(int(self.node_port) + total)
        return port
    

    def construct_graph(self, entry_port: str, node_port: str) -> tuple[str, str]:
        self.entry_port = entry_port
        self.node_port = node_port

        first_node = None
        previous_node = None

        if self.entry_nodes is not None:
            for node_type in self.entry_nodes:
                for i in range(self.entry_nodes[node_type]):
                    if first_node is None:
                        node = self.make_entry_node(node_type)
                        node.start()
                        first_node = node
                        previous_node = node
                    else:
                        node = self.make_entry_node(node_type, previous=previous_node)
                        node.start()
                        previous_node = node

        if self.nodes is not None:        
            for node_type in self.nodes:
                for i in range(self.nodes[node_type]):
                    if first_node is None:
                        node = self.make_node(node_type)
                        node.start()
                        first_node = node
                        previous_node = node
                    else:
                        node = self.make_node(node_type, previous=previous_node)
                        node.start()
                        previous_node = node

        first_node.manual_insert(previous_node)

        return self.entry_port, self.node_port 



class AddEntryServers(TopologyConstructor):
    def __init__(self, entry_nodes: dict[str, int]):
        super().__init__(name="AddEntryServers")
        self.entry_nodes = entry_nodes

    def to_json(self) -> dict:
        return {self.name: {"entry_nodes": self.entry_nodes}}
    
    # Construct Entry Servers and return current ports to be used
    def construct_graph(self, entry_port: str, node_port: str) -> tuple[str, str]:
        self.entry_port = entry_port
        self.node_port = node_port
        for node_type in self.entry_nodes:
            for i in range(0, self.entry_nodes[node_type]):
                node = self._make_entry_node(node_type)
                node.start()
        return self.entry_port, self.node_port
    


class AddErdosRenyi(TopologyConstructor):
    def __init__(self, entry_nodes: dict[str, int]=None, nodes: dict[str, int]=None, edge_prob: float=0.0, use_entry: bool=True):
        super().__init__(name="AddErdosRenyi")
        self.entry_nodes = entry_nodes
        self.nodes = nodes
        self.edge_prob = edge_prob
        self.graph = None
        self.use_entry = use_entry

    def to_json(self) -> dict:
        return {self.name: {"entry_nodes": self.entry_nodes, "nodes": self.nodes, "edge_prob": self.edge_prob}}

    def construct_graph(self, entry_port: str, node_port: str) -> tuple[str, str]:
        self.entry_port = entry_port
        self.node_port = node_port
        all_nodes = []
        total = 0
        if self.entry_nodes is not None:
            for node_type in self.entry_nodes:
                total += self.entry_nodes[node_type]
                for i in range(0, self.entry_nodes[node_type]):
                    node = self._make_entry_node(node_type)
                    all_nodes.append(node)

        if self.nodes is not None:
            for node_type in self.nodes:
                total += self.nodes[node_type]
                for i in range(0, self.nodes[node_type]):
                    node = self._make_node(node_type, pass_entry=self.use_entry)
                    all_nodes.append(node)

        graph = nx.erdos_renyi_graph(total, self.edge_prob)

        self.graph = graph

        for edge in graph.edges():
            all_nodes[edge[0]].manual_insert(all_nodes[edge[1]])
            all_nodes[edge[1]].manual_insert(all_nodes[edge[0]])

        for node in all_nodes:
            node.start()
  

        return self.entry_port, self.node_port
    

class AddUniformRandom(TopologyConstructor):
    def __init__(self, entry_nodes: dict[str, int]=None, nodes: dict[str, int]=None,  num_edges: int=0, use_entry=True):
        super().__init__(name="AddUniformRandom")
        self.entry_nodes = entry_nodes
        self.nodes = nodes
        self.num_edges = num_edges
        self.use_entry = use_entry

    def to_json(self) -> dict:
        return {self.name: {"entry_nodes": self.entry_nodes, "nodes": self.nodes, "num_edges": self.num_edges}}
    
    def construct_graph(self, entry_port: str, node_port: str) -> tuple[str, str]:
        self.entry_port = entry_port
        self.node_port = node_port

        all_nodes = []
        if self.entry_nodes is not None:
            for node_type in self.entry_nodes:
                for i in range(0, self.entry_nodes[node_type]):
                    node = self._make_entry_node(node_type)
                    all_nodes.append(node)

        if self.nodes is not None:
            for node_type in self.nodes:
                for i in range(0, self.nodes[node_type]):
                    node = self._make_node(node_type, pass_entry=self.use_entry)
                    all_nodes.append(node)

        for node in all_nodes:
            ne = self.num_edges
            if self.num_edges > node.view.max_size():
                ne = node.view.max_size()
            for i in range(ne):
                idx = random.randint(0, len(all_nodes))
                node.manual_insert(all_nodes[idx])

        for node in all_nodes:
            node.start()

        return self.entry_port, self.node_port



class Simulator:
    def __init__(self, schema: dict[str, NodeSchema], events: list[TopologyConstructor], log_dir: Path = Path("sim_logs/"), name: str=None):
        self.name = name
        self.log_dir = log_dir
        self.node_schema_registry = schema
        
        self.entry_port = "50000"
        self.entry_node_addresses = []
        self.entry_entered = {}
        self.entry_removed = {}

        self.node_port = "80000"
        self.node_entered = {}
        self.node_removed = {}

        self.sim_start_time = 0
        self.sim_stop_time = 0
        self.events = events

        self.entry_times = {}
        self.exit_times = {}

        self.meta_data = {}
        self.selection_logs = None
        self.statistics = None
        


    def get_logs(self, log):
        logs_dfs = []
        for t, nodes in log.items():
            for node in nodes:
                logs_dfs.append(node.log.copy_data())
        return logs_dfs

    
    def save(self):
        if self.name is None:
            self.name = f"{self.sim_start_time}"
        
        log_dir = Path(self.log_dir) / self.name
        log_dir.mkdir(parents=True, exist_ok=True)
        
        meta_path = log_dir / "metadata.json"
        with meta_path.open("w") as f:
            f.write(json.dumps(self.meta_data, indent=4))

        self.selection_logs.to_csv(
            log_dir / "selection_logs.csv",  # Convert Path to string for Dask
            index=False,
        )
    

    def signal_nodes(self, nodes_to_signal, dest) -> None:
        for t, nodes in nodes_to_signal.items():
            for node in list(nodes):  
                node.signal()
                nodes.remove(node)  # Modify the original list
                add_to_registry(dest, t, node)

    
    def stop_nodes(self, nodes_to_stop) -> None:
        for t, nodes in nodes_to_stop.items():
            for node in nodes:  
                node.signal()


    def stop(self, stats_interval, save=True):
        self.signal_nodes(self.entry_entered, self.entry_removed)
        self.signal_nodes(self.node_entered, self.node_removed)
        self.stop_nodes(self.entry_removed)
        self.stop_nodes(self.node_removed)
        
        self.sim_stop_time = int(time.time())
        self.meta_data = {
            "sim_start_time": self.sim_start_time,
            "sim_stop_time": self.sim_stop_time,
            "sim_events": [event.to_json() for event in self.events],
            "entry_times": self.entry_times,
            "exit_times": self.exit_times,
        }

        node_logs = self.get_logs(self.node_removed)
        entry_logs = self.get_logs(self.entry_removed)
        all_logs = node_logs + entry_logs
        if (len(all_logs)):
            combined_logs = pd.concat(all_logs)

        self.selection_logs = combined_logs

        if save:
            self.save()

        print(f"Simulation finished in {self.sim_stop_time - self.sim_start_time} seconds.")

        if stats_interval > 0:
            self.statistics = stats.StatsGenerator(connection_df=self.selection_logs, meta_data=self.meta_data, 
                                            log_dir=self.name, base_log_dir=self.log_dir)
            self.statistics.calc_statistics(stats_interval, save)

            print("Finished Calculating Statistics for the Simulation Run")

    
    def run_event(self, event: TopologyConstructor):
            event.pass_sim_var(node_schema_registry=self.node_schema_registry,
                entry_entered=self.entry_entered,
                node_entered=self.node_entered,
                entry_times=self.entry_times,
                entry_removed=self.entry_removed,
                node_removed=self.node_removed,
                entry_node_addresses=self.entry_node_addresses,
                exit_times=self.exit_times)
            if not event.schema_in_registry():
                raise ValueError(f"Nodes specified in {event.name} are not present in the schema registry.")
            self.entry_port, self.node_port = event.construct_graph(self.entry_port, self.node_port)



    def run(self, stats_interval=-1, save=True):
        def len_str_int_dict(values: dict[str, int]) -> int:
            sum = 0
            if len(values) > 0:
                for key, val in values.items():
                    sum += len(val)
            return sum

        self.sim_start_time = int(time.time())
        for event in self.events:
            print(f"Starting {event.name} with {len_str_int_dict(self.entry_entered)} entry nodes and {len_str_int_dict(self.node_entered)} nodes at time {int(time.time()) - self.sim_start_time}.")
            done = self.run_event(event)
        print("Entering Stop")
        self.stop(stats_interval, save) 
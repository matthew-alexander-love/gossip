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
import json
import matplotlib.pyplot as plt
import networkx as nx
import numpy as np
import pandas as pd
from scipy import stats
from pathlib import Path
import time
from concurrent.futures import ThreadPoolExecutor


class StatsGenerator:
    def __init__(self, connection_df: pd.DataFrame=None, meta_data: dict=None, log_dir: str=None, base_log_dir: Path = Path("sim_logs/")):
        self.path = base_log_dir / log_dir
        self.connection_df = None
        self.meta_data = None

        if connection_df is None or meta_data is None:
            self.init_from_files(log_dir, base_log_dir)
        else:
            self.connection_df = connection_df
            self.meta_data = meta_data

        self.temporal_graph_slices: list[nx.Graph] = []
        self.node_stats: list[pd.DataFrame] = []
        self.network_stats: pd.DataFrame = None
        self.path_lengths: pd.DataFrame = {}


    def init_from_files(self, log_dir: str, base_log_dir: Path = Path("sim_logs/")):
        self.path = base_log_dir / log_dir
        try:
            self.connection_df = pd.read_csv(self.path / "selection_logs.csv")
            self.meta_data = None
            with open(self.path / "metadata.json", "r") as f:
                self.meta_data = json.load(f)
        except:
            self.connection_df = None
            self.meta_data = None


    def create_network_from_timeframe(self, timestamp_lower_bound: int, timestamp_upper_bound: int) -> nx.Graph:
        # Filter DataFrame based on timestamp range
        filtered_df = self.connection_df[
            (self.connection_df['time [ms]'] >= timestamp_lower_bound) & 
            (self.connection_df['time [ms]'] <= timestamp_upper_bound)
        ]
        # Create a directed graph
        G = nx.DiGraph()

        def is_node_active(node, timestamp):
            if node in self.meta_data['entry_times']:
                return self.meta_data["entry_times"][node] <= timestamp <= self.meta_data["exit_times"][node]
            return False
        
        for _, row in filtered_df.iterrows():
            source = row['id']
            target = row['selected']
            timestamp = row['time [ms]']

            if is_node_active(source, timestamp) and is_node_active(target, timestamp):
                G.add_edge(source, target)
            elif is_node_active(source, timestamp):
                G.add_edge(source, f'DEAD_LINK_{target}')

        return G


    def create_temporal_network_slices(self, ms_interval: int) -> list[nx.Graph]:
        start_time = self.connection_df['time [ms]'].min()
        stop_time = self.connection_df['time [ms]'].max()

        num_intervals = int((stop_time - start_time) / ms_interval)
        interval_graphs = []
        for i in range(num_intervals):
            start = start_time + (i * ms_interval)
            stop = start_time + ((i+1) * ms_interval)
            g = self.create_network_from_timeframe(start, stop)
            interval_graphs.append(g)

        self.temporal_graph_slices = interval_graphs

        return interval_graphs


    def compute_node_level_statistics(self, G: nx.Graph) -> pd.DataFrame:
        def count_dead_out(G: nx.Graph, node) -> int:
            edges = G.out_edges(node)
            dead_link_count = sum(1 for _, target in edges if str(target).startswith('DEAD_LINK'))
            return dead_link_count

        # Convert to undirected graph for some calculations
        G_undirected = G.to_undirected()
        
        # Prepare lists to store node statistics
        node_stats = []
        
        # Compute various node-level metrics
        count = 0
        for node in G.nodes():
            count += 1
            dead_out = count_dead_out(G, node)
            node_data = {
                'node': node,
                
                # Degree metrics
                'dead_out': dead_out,
                'in_degree': G.in_degree(node),
                'out_degree': G.out_degree(node) - dead_out,
                'total_degree': G.degree(node),
                
                # Centrality measures
                #'degree_centrality': nx.degree_centrality(G)[node],
                #'in_degree_centrality': nx.in_degree_centrality(G)[node],
                #'out_degree_centrality': nx.out_degree_centrality(G)[node],
                
                # Betweenness centrality
                #'betweenness_centrality': nx.betweenness_centrality(G)[node],
                
                # Clustering coefficient
                'clustering_coefficient': nx.clustering(G_undirected, node),
            }
            
            node_stats.append(node_data)
        
        return pd.DataFrame(node_stats)


    def compute_network_level_statistics(self, G: nx.Graph, node_stats_df: pd.DataFrame) -> dict:
        # Convert to undirected graph for some calculations
        G_undirected = G.to_undirected()
        
        # Prepare network-level statistics
        in_degree_stats = node_stats_df['in_degree'].describe()
        #centrality_stats = node_stats_df['degree_centrality'].describe()
        paths = dict(nx.all_pairs_shortest_path_length(G))
        paths_df = pd.DataFrame.from_dict(paths, orient="index").fillna(-1)


        network_stats = {
            # Basic network properties
            'num_nodes': G.number_of_nodes(),
            'num_edges': G.number_of_edges(),
            
            # Degree statistics
            'avg_in_degree': in_degree_stats['mean'],
            'std_in_degree': in_degree_stats['std'],
            'max_in_degree': in_degree_stats['max'],
            'min_in_degree': in_degree_stats['min'],
            
            # Centrality measures
            #'avg_degree_centrality': centrality_stats['mean'],
            #'std_degree_centrality': centrality_stats['std'],
            #'max_degree_centrality': centrality_stats['max'],
            #'min_degree_centrality': centrality_stats['min'],
            
            # Clustering metrics
            'global_clustering_coefficient': nx.average_clustering(G_undirected),

            # Connectivity
            'is_connected': nx.is_connected(G_undirected),
            'is_connected_dir': nx.is_strongly_connected(G),
            'num_weakly_connected_components': nx.number_weakly_connected_components(G),
        }
        return network_stats, paths_df


    def save(self):
        self.network_stats.to_csv(
            self.path / "network_stats.csv",
            index=False,
        )

        self.node_stats.to_csv(
            self.path / "node_stats.csv",
            index=False,
        )

        self.path_lengths.to_csv(
            self.path / "path_lengths.csv",
            index=False,
        )


    def load(self):
        try:
            self.network_stats = pd.from_csv(self.path / "network_stats.csv")
        except:
            self.network_stats = None

        try:
            self.network_stats = pd.from_csv(self.path / "node_stats.csv")
        except:
            self.network_stats = None


    def process_graph_slice(self, graph_data):
        graph, idx = graph_data
        # Compute node level statistics
        node_stats = self.compute_node_level_statistics(graph)
        # Add time slice information
        node_stats['time_slice'] = idx
        # Compute network statistics
        network_stats, paths_df = self.compute_network_level_statistics(graph, node_stats)
        # Add time slice to network stats
        network_stats['time_slice'] = idx
        paths_df['time_slice'] = idx
        
        return (node_stats, network_stats, paths_df)


    def calc_statistics(self, ms_interval: int=5000, save=True):
        if self.connection_df is not None:
            start_time = time.time()
            self.create_temporal_network_slices(ms_interval=ms_interval)
            graph_data = [(graph, idx) for idx, graph in enumerate(self.temporal_graph_slices)]
    
            node_stats_dfs = []
            network_stats_dicts = []
            path_length_dfs = []
            with ThreadPoolExecutor() as executor:
                # Map the processing function to all graph slices
                results = list(executor.map(self.process_graph_slice, graph_data))
                
                # Separate results
                for node_df, network_dict, paths_df in results:
                    node_stats_dfs.append(node_df)
                    network_stats_dicts.append(network_dict)
                    path_length_dfs.append(paths_df)

            self.node_stats = pd.concat(node_stats_dfs, ignore_index=True)
            self.network_stats = pd.DataFrame(network_stats_dicts)
            self.path_lengths = pd.concat(path_length_dfs, ignore_index=True)

                    # Sort by time slice to maintain temporal order
            self.node_stats.sort_values('time_slice', inplace=True)
            self.network_stats.sort_values('time_slice', inplace=True)

        if save:
            self.save()
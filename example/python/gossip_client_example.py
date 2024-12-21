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

import time
import sys
import gossip
import gossip.pss_manager

def main(args):
    # Config Info
    address = "192.168.1.173:60000"
    if (len(args) > 1):
        address = args[1]
    
    view_size = 10
    view_healing = 3
    view_swap = 3
    push = True
    pull = True
    client_thread_sleep_time = 10 #seconds
    request_timeout = 2 #seconds
    entry_points = []
    if (len(args) > 2):
        for i in range(2, len(args)):
            entry_points.append(args[i])
    
    else:
        entry_points.append("192.168.1.173:50000")

    # Create Peer Sampling Service Manager Object
    service = gossip.PSSManager(address=address, push=push, pull=pull, 
                                wait_time=client_thread_sleep_time, timeout=request_timeout,
                                entry_points=entry_points,
                                pss_type=gossip.PeerSamplingService,
                                view_type=gossip.URView,
                                selector_type=gossip.SelectorType.TAIL,
                                size=view_size, healing=view_healing, swap=view_swap)
    
    # Enter the Overlay Network
    service.enter()

    # Start the Server and Client Threads for the gossip protocol
    service.start()

    # Create subscriptions to be able to sample peers from the overlay network
    subscriber = service.subscribe(gossip.SelectorType.TAIL)

    # Do work with the new peer selection subscription
    while(True):
        peer = subscriber.select_peer()
        if peer:
            print(f'Client subscriber ({address}): {peer}')
        else:
            print(f'Client subscriber ({address}): View is empty')

        time.sleep(client_thread_sleep_time/2)


if __name__ == "__main__":
    main(sys.argv)
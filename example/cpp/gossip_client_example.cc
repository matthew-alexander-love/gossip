/**
 * GossipSampling
 * Copyright (C) Matthew Love 2024 (gossipsampling@gmail.com)
 * 
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 * 
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 * 
 * You should have received a copy of the GNU General Public License
 * along with this program.  If not, see <http://www.gnu.org/licenses/>.
 * 
 */


#include <memory>
#include <string>
#include <vector>

#include "gossip.h"

using namespace gossip;

int main(int argc, char* argv[]) {
    // Setting up config info for the Service
    std::string address = "192.168.1.173:60000";
    if (argc > 1) {
        address = argv[1];
    }
    int view_size = 10;
    int view_healing = 3;
    int view_swap = 3;
    bool push = true;
    bool pull = true;
    unsigned int client_thread_sleep_time = 10; //seconds
    unsigned int request_timeout = 2; //seconds
    std::vector<std::string> entry_points;
    if (argc > 2) {
        for (int i = 2; i < argc; ++i) {
            entry_points.push_back(argv[i]);
        }
    }
    else {
        entry_points.push_back("192.168.1.173:50000");
    }

    // Create Peer Sampling Service Manager Object
    PSSManager<PeerSamplingService, URView, int, int, int> service(address, push, pull, client_thread_sleep_time, request_timeout,
                                                                   entry_points, SelectorType::TAIL, view_size, view_healing, view_swap);
    // Enter the Overlay Network
    service.enter();

    // Start the Server and Client Threads for the gossip protocol
    service.start();

    // Create subscriptions to be able to sample peers from the overlay network
    auto subscriber = service.subscribe(gossip::SelectorType::UNIFORM_RANDOM_NO_REPLACEMENT);

    // Do work with the new peer selection subscription
    while (true) {
        auto peer = subscriber->select_peer();
        if (peer) {
            std::cout << "Client subscriber (" << address << "): " << *peer << std::endl;
        }
        else {
            std::cout << "Client subscriber (" << address << "): View is empty" << std::endl;
        }
        std::this_thread::sleep_for(std::chrono::seconds(client_thread_sleep_time/2));
    }
    return 0;
}
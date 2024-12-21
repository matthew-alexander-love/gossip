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

#include <gtest/gtest.h>

#include <grpcpp/grpcpp.h>

#include "client.h"
#include "server.h"


using namespace gossip;

TEST(_Server_, construction) {
    std::shared_ptr<URView> view = std::make_shared<URView>("localhost:50051", 10, 5, 5);
    view->init_selector(SelectorType::TAIL);
    std::string server_address("0.0.0.0:50051");
    std::shared_ptr<Server> service = std::make_shared<Server>(view);
}

TEST(_Server_, construction_bind) {
    std::string server_address("0.0.0.0:50051");
    std::shared_ptr<URView> view = std::make_shared<URView>("localhost:50051", 10, 5, 5);
    view->init_selector(SelectorType::TAIL);
    std::shared_ptr<Server> server = std::make_shared<Server>(view);

    std::shared_ptr<Server::Thread> server_thread = server->thread();
    server_thread->start();
}


TEST(_Client_, construction) {
    const bool push = true;
    const bool pull = true;
    unsigned int wait_time = 1;
    unsigned int num_concurrent = 1;
    unsigned int timeout = 1;
    int size = 10;
    int healing = 5;
    int swap = 5;

    std::shared_ptr<URView> view = std::make_shared<URView>("localhost:50051", size, healing, swap);
    view->init_selector(SelectorType::TAIL);
    std::shared_ptr<Client> client = std::make_shared<Client>(push, pull, wait_time, timeout, view);
}

TEST(_Client_, push_view_w_address) {
    const bool push = true;
    const bool pull = true;
    unsigned int wait_time = 1;
    unsigned int timeout = 1;
    int size = 10;
    int healing = 5;
    int swap = 5;

    std::shared_ptr<URView> view = std::make_shared<URView>("localhost:50051", size, healing, swap);
    view->init_selector(SelectorType::TAIL);
    std::shared_ptr<Client> client = std::make_shared<Client>(push, pull, wait_time, timeout, view);
    client->push_view("192.168.225.1:7000");
}

TEST(_Client_, pull_view_w_address) {
    const bool push = true;
    const bool pull = true;
    unsigned int wait_time = 1;
    unsigned int timeout = 1;
    int size = 10;
    int healing = 5;
    int swap = 5;

    std::shared_ptr<URView> view = std::make_shared<URView>("localhost:50051", size, healing, swap);
    view->init_selector(SelectorType::TAIL);
    std::shared_ptr<Client> client = std::make_shared<Client>(push, pull, wait_time, timeout, view);
    client->pull_view("192.168.225.1:7000");
}

TEST(_Client_, push_pull_view_w_address) {
    const bool push = true;
    const bool pull = true;
    unsigned int wait_time = 1;
    unsigned int timeout = 1;
    int size = 10;
    int healing = 5;
    int swap = 5;

    std::shared_ptr<URView> view = std::make_shared<URView>("localhost:50051", size, healing, swap);
    view->init_selector(SelectorType::TAIL);
    std::shared_ptr<Client> client = std::make_shared<Client>(push, pull, wait_time, timeout, view);
    client->push_pull_view("192.168.225.1:7000");
}

TEST(_Client_, thread_empty_view_single) {
    const bool push = true;
    const bool pull = true;
    unsigned int wait_time = 1;
    unsigned int timeout = 1;
    int size = 10;
    int healing = 5;
    int swap = 5;

    std::shared_ptr<URView> view = std::make_shared<URView>("localhost:50051", size, healing, swap);
    view->init_selector(SelectorType::TAIL);
    std::shared_ptr<Client> client = std::make_shared<Client>(push, pull, wait_time, timeout, view);
    auto thread = client->thread();
    thread->start();
    std::this_thread::sleep_for(std::chrono::seconds(timeout+1));
}

TEST(_Client_, thread_full_view_single) {
    // Client Settings
    const bool push = true;
    const bool pull = true;
    unsigned int wait_time = 1;
    unsigned int timeout = 1;
    int size = 10;
    int healing = 5;
    int swap = 5;

    // Make View
    const std::string ip = "192.168.225.1";
    const std::string port = "5012";
    const std::string my_address = ip + ":" + port;
    std::vector<std::shared_ptr<NodeDescriptor>> nodes;
    for (int i = 0; i < size; ++i) {
        std::shared_ptr<NodeDescriptor> new_node = std::make_shared<NodeDescriptor>(ip + ":" + std::to_string((std::stoi(port) + i)), i);
        nodes.push_back(new_node);
    }
    std::shared_ptr<URView> view = std::make_shared<URView>("localhost:50051", size, healing, swap);
    view->init_selector(SelectorType::TAIL);
    view->rx_nodes(nodes);

    // Test Client
    std::shared_ptr<Client> client = std::make_shared<Client>(push, pull, wait_time, timeout, view);
    auto thread = client->thread();
    thread->start();
    std::this_thread::sleep_for(std::chrono::seconds(timeout+1));
}

/*
In the below server.Wait() isnt called, but doesn't matter because the server never leaves scope and so is active for the client requests
*/

TEST(_ClientServer_, push) {
    std::string server_address("0.0.0.0:50051");
    std::shared_ptr<URView> view_server = std::make_shared<URView>("localhost:50051", 10, 5, 5);
    std::shared_ptr<Server> server = std::make_shared<Server>(view_server);
    std::shared_ptr<Server::Thread> server_thread = server->thread();
    server_thread->start();

    const bool push = true;
    const bool pull = true;
    unsigned int wait_time = 1;
    unsigned int timeout = 1;
    int size = 10;
    int healing = 5;
    int swap = 5;

    std::shared_ptr<URView> view_client = std::make_shared<URView>("clienthost:50052", size, healing, swap);
    view_client->init_selector(SelectorType::TAIL);
    std::shared_ptr<Client> client = std::make_shared<Client>(push, pull, wait_time, timeout, view_client);

    ::grpc::Status result = client->push_view("0.0.0.0:50051");

    ASSERT_TRUE(result.ok());
}

TEST(_ClientServer_, pull) {
    std::string server_address("0.0.0.0:50051");
    std::shared_ptr<URView> view_server = std::make_shared<URView>("localhost:50051", 10, 5, 5);
    std::shared_ptr<Server> server = std::make_shared<Server>(view_server);
    std::shared_ptr<Server::Thread> server_thread = server->thread();
    server_thread->start();

    const bool push = true;
    const bool pull = true;
    unsigned int wait_time = 1;
    unsigned int timeout = 1;
    int size = 10;
    int healing = 5;
    int swap = 5;

    std::shared_ptr<URView> view_client = std::make_shared<URView>("clienthost:50052", size, healing, swap);
    std::shared_ptr<Client> client = std::make_shared<Client>(push, pull, wait_time, timeout, view_client);

    ::grpc::Status result = client->pull_view("0.0.0.0:50051");

    ASSERT_TRUE(result.ok());
}

TEST(_ClientServer_, push_pull) {
    std::string server_address("0.0.0.0:50051");
    std::shared_ptr<URView> view_server = std::make_shared<URView>("localhost:50051", 10, 5, 5);
    view_server->init_selector(SelectorType::TAIL);
    std::shared_ptr<Server> server = std::make_shared<Server>(view_server);
    std::shared_ptr<Server::Thread> server_thread = server->thread();
    server_thread->start();

    const bool push = true;
    const bool pull = true;
    unsigned int wait_time = 1;
    unsigned int timeout = 1;
    int size = 10;
    int healing = 5;
    int swap = 5;

    std::shared_ptr<URView> view_client = std::make_shared<URView>("clienthost:50052", size, healing, swap);
    view_client->init_selector(SelectorType::TAIL);
    std::shared_ptr<Client> client = std::make_shared<Client>(push, pull, wait_time, timeout, view_client);

    ::grpc::Status result = client->push_pull_view("0.0.0.0:50051");

    ASSERT_TRUE(result.ok());
}
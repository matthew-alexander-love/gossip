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

#include "peer_sampling_service.h"

using namespace gossip;

struct _PeerSamplingService_ : public ::testing::Test {
    const bool push = true;
    const bool pull = true;
    unsigned int wait_time = 1;
    unsigned int timeout = 1;
    int size = 10;
    int healing = 5;
    int swap = 5;

    std::string port = "50000";

    PeerSamplingService make_entry_server() {
        std::vector<std::string> empty_es;
        std::shared_ptr<URView> view = std::make_shared<URView>("0.0.0.0:" + port, size, healing, swap);
        view->init_selector(SelectorType::TAIL);
        return PeerSamplingService(push, pull, wait_time, timeout, empty_es, view);
    }

    PeerSamplingService make_client(int client_num) {
        std::vector<std::string> es = {"0.0.0.0:" + port};
        std::shared_ptr<URView> view = std::make_shared<URView>("0.0.0.0:" + std::to_string((std::stoi(port) + client_num + 1)), size, healing, swap);
        view->init_selector(SelectorType::TAIL);
        return PeerSamplingService(push, pull, wait_time, timeout, es, view);
    }

    std::pair<PeerSamplingService, std::shared_ptr<VectorLog>> make_logged_entry_server() {
        
        std::vector<std::string> empty_es;
        std::shared_ptr<URView> view = std::make_shared<URView>("0.0.0.0:" + port, size, healing, swap);
        std::shared_ptr<VectorLog> log = std::make_shared<VectorLog>();
        view->init_selector(SelectorType::LOGGED_TAIL, log);
        return {PeerSamplingService(push, pull, wait_time, timeout, empty_es, view), log};
    }

    std::pair<PeerSamplingService, std::shared_ptr<VectorLog>> make_logged_client(int client_num) {
        std::vector<std::string> es = {"0.0.0.0:" + port};
        std::shared_ptr<URView> view = std::make_shared<URView>("0.0.0.0:" + std::to_string((std::stoi(port) + client_num + 1)), size, healing, swap);
        std::shared_ptr<VectorLog> log = std::make_shared<VectorLog>();
        view->init_selector(SelectorType::LOGGED_TAIL, log);
        return {PeerSamplingService(push, pull, wait_time, timeout, es, view), log};
    }
};

TEST_F(_PeerSamplingService_, construction) {
    std::vector<std::string> entry_points; // Basically an entry server, we are first into network

    std::shared_ptr<URView> view = std::make_shared<URView>("0.0.0.0:50052", size, healing, swap);
    view->init_selector(SelectorType::TAIL);
    PeerSamplingService peer_sampling_service(push, pull, wait_time, timeout, entry_points, view);

    ASSERT_EQ(peer_sampling_service.push(), push);
    ASSERT_EQ(peer_sampling_service.pull(), pull);
    ASSERT_EQ(peer_sampling_service.timeout(), timeout);
    ASSERT_EQ(peer_sampling_service.wait_time(), wait_time);
}

TEST_F(_PeerSamplingService_, print) {
    std::vector<std::string> entry_points; // Basically an entry server, we are first into network

    std::shared_ptr<URView> view = std::make_shared<URView>("0.0.0.0:50052", size, healing, swap);
    view->init_selector(SelectorType::TAIL);
    PeerSamplingService peer_sampling_service(push, pull, wait_time, timeout, entry_points, view);

    std::cout << peer_sampling_service << std::endl;
}


TEST_F(_PeerSamplingService_, start_client_thread_no_entry) {

    std::vector<std::string> entry_points; // Basically an entry server, we are first into network

    std::string address = "0.0.0.0:50052";

    std::shared_ptr<URView> view = std::make_shared<URView>(address, size, healing, swap);
    view->init_selector(SelectorType::TAIL);

    PeerSamplingService peer_sampling_service(push, pull, wait_time, timeout, entry_points, view);
    peer_sampling_service.start_server();

    peer_sampling_service.start_client();
    std::this_thread::sleep_for(std::chrono::seconds(timeout+1));
}

TEST_F(_PeerSamplingService_, one_enter_from_zero) {

    std::vector<std::string> entry_points0; // Basically an entry server, we are first into network
    std::string address0 = "0.0.0.0:50050";
    std::shared_ptr<URView> view0 = std::make_shared<URView>(address0, size, healing, swap);
    view0->init_selector(SelectorType::TAIL);
    PeerSamplingService peer_sampling_service0(push, pull, wait_time, timeout, entry_points0, view0);
    peer_sampling_service0.start_server();

    std::vector<std::string> entry_points1 = {address0}; // Basically an entry server, we are first into network
    std::string address1 = "0.0.0.0:50051";
    std::shared_ptr<URView> view1 = std::make_shared<URView>(address1, size, healing, swap);
    view1->init_selector(SelectorType::TAIL);
    PeerSamplingService peer_sampling_service1(push, pull, wait_time, timeout, entry_points1, view1);
    peer_sampling_service1.start_server();

    std::cout << "Before: " << *view1 << std::endl;
    peer_sampling_service1.enter();
    std::cout << "After: " << *view1 << std::endl;
}


TEST_F(_PeerSamplingService_, one_enter_from_zero_start) {

    std::vector<std::string> entry_points0; // Basically an entry server, we are first into network
    std::string address0 = "0.0.0.0:50050";
    std::shared_ptr<URView> view0 = std::make_shared<URView>(address0, size, healing, swap);
    view0->init_selector(SelectorType::TAIL);
    PeerSamplingService peer_sampling_service0(push, pull, wait_time, timeout, entry_points0, view0);
    peer_sampling_service0.start_server();

    std::vector<std::string> entry_points1 = {address0}; // Basically an entry server, we are first into network
    std::string address1 = "0.0.0.0:50051";
    std::shared_ptr<URView> view1 = std::make_shared<URView>(address1, size, healing, swap);
    view1->init_selector(SelectorType::TAIL);
    PeerSamplingService peer_sampling_service1(push, pull, wait_time, timeout, entry_points1, view1);
    peer_sampling_service1.start_server();

    std::cout << "Before: " << *view1 << std::endl;
    peer_sampling_service1.enter();
    std::cout << "After: " << *view1 << std::endl;


    peer_sampling_service1.start_client();


    std::this_thread::sleep_for(std::chrono::seconds(timeout+1));    
}


TEST_F(_PeerSamplingService_, lan) {
    PeerSamplingService es = make_entry_server();
    PeerSamplingService c0 = make_client(0);
    PeerSamplingService c1 = make_client(1);
    PeerSamplingService c2 = make_client(2);
    PeerSamplingService c3 = make_client(3);

    auto c0_sub = c0.view()->create_subscriber(SelectorType::TAIL);
    auto c1_sub = c1.view()->create_subscriber(SelectorType::UNIFORM_RANDOM);
    auto c2_sub = c2.view()->create_subscriber(SelectorType::UNIFORM_RANDOM_NO_REPLACEMENT);
    auto c3_sub = c3.view()->create_subscriber(SelectorType::TAIL);
    
    es.start_server();
    //c0.start_server();
    //c1.start_server();
    //c2.start_server();
    //c3.start_server();

    c0.enter();
    c1.enter();
    c2.enter();
    c3.enter(); 

    c0.start();
    c1.start();
    c2.start();
    c3.start();
    
    // Check logs here to make sure it works. Ik not best grpc use, but it still catches runtime errors.
    std::this_thread::sleep_for(std::chrono::seconds(10));

    std::cout << "----------------------------------------------" << std::endl;
    std::cout << *(c0_sub->select_peer()) << std::endl;
    std::cout << *(c1_sub->select_peer()) << std::endl;
    std::cout << *(c2_sub->select_peer()) << std::endl;
    std::cout << *(c3_sub->select_peer()) << std::endl;

    c0.stop();
    c1.stop();
    c2.stop();
    c3.stop();


    std::this_thread::sleep_for(std::chrono::seconds(2));
}

TEST_F(_PeerSamplingService_, logged_lan) {
    auto es = make_logged_entry_server();
    auto c0 = make_logged_client(0);
    auto c1 = make_logged_client(1);
    auto c2 = make_logged_client(2);
    auto c3 = make_logged_client(3);

    auto c0_sub = c0.first.view()->create_subscriber(SelectorType::TAIL);
    auto c1_sub = c1.first.view()->create_subscriber(SelectorType::UNIFORM_RANDOM);
    auto c2_sub = c2.first.view()->create_subscriber(SelectorType::UNIFORM_RANDOM_NO_REPLACEMENT);
    auto c3_sub = c3.first.view()->create_subscriber(SelectorType::TAIL);
    
    es.first.start_server();
    //c0.start_server();
    //c1.start_server();
    //c2.start_server();
    //c3.start_server();

    c0.first.enter();
    c1.first.enter();
    c2.first.enter();
    c3.first.enter(); 

    c0.first.start();
    c1.first.start();
    c2.first.start();
    c3.first.start();
    
    // Check logs here to make sure it works. Ik not best grpc use, but it still catches runtime errors.
    std::this_thread::sleep_for(std::chrono::seconds(5));

    std::cout << "c0 data: " << *(c0.second) << std::endl;
    std::cout << "c1 data: " << *(c1.second) << std::endl;
    std::cout << "c2 data: " << *(c2.second) << std::endl;
    std::cout << "c3 data: " << *(c3.second) << std::endl;

    std::this_thread::sleep_for(std::chrono::seconds(5));

    std::cout << "----------------------------------------------" << std::endl;
    std::cout << *(c0_sub->select_peer()) << std::endl;
    std::cout << *(c1_sub->select_peer()) << std::endl;
    std::cout << *(c2_sub->select_peer()) << std::endl;
    std::cout << *(c3_sub->select_peer()) << std::endl;

    c0.first.stop();
    c1.first.stop();
    c2.first.stop();
    c3.first.stop();


    std::this_thread::sleep_for(std::chrono::seconds(2));
}
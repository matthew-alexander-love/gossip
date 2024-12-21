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
#include <thread>
#include <future>
#include <atomic>

#include <grpcpp/grpcpp.h>

#include "view_proto_helper.h"
#include "client.h"

namespace gossip {

::grpc::Status ClientSession::push_view(const ViewProto& tx_buf, std::shared_ptr<::google::protobuf::Empty> dummy_resp) {
    ::grpc::ClientContext context;
    auto deadline = std::chrono::system_clock::now() + std::chrono::seconds(_timeout);
    context.set_deadline(deadline);

    std::promise<::grpc::Status> status_promise;
    auto status_future = status_promise.get_future();

    _stub->async()->PushView(&context, &tx_buf, dummy_resp.get(), [&status_promise, this](::grpc::Status status) mutable {
        if (status.ok()) {
            //std::cout << "Successful Push rpc to: " << _server_address << std::endl;
        }
        else {
            //std::cout << "Failed Push rpc to: " << _server_address << std::endl;
        }
        status_promise.set_value(status);
    });
    return status_future.get();
}

::grpc::Status ClientSession::pull_view(const ::google::protobuf::Empty& dummy_req, std::shared_ptr<ViewProto> rx_buf) {

    ::grpc::ClientContext context;
    auto deadline = std::chrono::system_clock::now() + std::chrono::seconds(_timeout);
    context.set_deadline(deadline);
    
    std::promise<::grpc::Status> status_promise;
    auto status_future = status_promise.get_future();

    _stub->async()->PullView(&context, &dummy_req, rx_buf.get(), [rx_buf, &status_promise, this](::grpc::Status status) {
        if (!status.ok()) {
            //std::cout << "Failed Pull rpc to: " << _server_address << std::endl; 
        }
        else {
            // Convert ViewProto to std::vector<std::shared_ptr<NodeDescriptor>>
            std::vector<std::shared_ptr<NodeDescriptor>> new_nodes = ViewProtoHelper<NodeDescriptor>::make_internal(*rx_buf);
            // Pass to view obj
            _view->rx_nodes(new_nodes);
            _view->increment_age(); 
            //std::cout << "Successful Pull rpc to: " << _server_address << std::endl;
        }
        status_promise.set_value(status);
    });
    return status_future.get();
}

::grpc::Status ClientSession::push_pull_view(const ViewProto& tx_buf, std::shared_ptr<ViewProto> rx_buf) {
    // Make data for the push    
    ::grpc::ClientContext context;
    auto deadline = std::chrono::system_clock::now() + std::chrono::seconds(_timeout);
    context.set_deadline(deadline);

    std::promise<::grpc::Status> status_promise;
    auto status_future = status_promise.get_future();

    _stub->async()->PushPullView(&context, &tx_buf, rx_buf.get(), [rx_buf, &status_promise, this](::grpc::Status status) {
        if (!status.ok()) { 
            //std::cout << "Failed PushPull rpc to: " << _server_address << std::endl;
        }
        else {
            // Convert ViewProto to std::vector<std::shared_ptr<NodeDescriptor>>
            std::vector<std::shared_ptr<NodeDescriptor>> new_nodes = ViewProtoHelper<NodeDescriptor>::make_internal(*rx_buf);
            // Pass to view obj
            _view->rx_nodes(new_nodes);
            _view->increment_age();
            //std::cout << "Successful PushPull rpc to: " << _server_address << std::endl;
        }
        status_promise.set_value(status);
    });
    return status_future.get();
}

::grpc::Status Client::push_view() {
    std::shared_ptr<NodeDescriptor> peer = _view->select_peer();
    if (!peer) {
        return ::grpc::Status(::grpc::StatusCode::NOT_FOUND, "No Peer was selected to push view to.");
    }
    return push_view(peer->address());
}

::grpc::Status Client::pull_view() {
    std::shared_ptr<NodeDescriptor> peer = _view->select_peer();
    if (!peer) {
        return ::grpc::Status(::grpc::StatusCode::NOT_FOUND, "No Peer was selected to pull view from.");
    }
    return pull_view(peer->address());
}

::grpc::Status Client::push_pull_view() {    
    std::shared_ptr<NodeDescriptor> peer = _view->select_peer();
    if (!peer) {
        return ::grpc::Status(::grpc::StatusCode::NOT_FOUND, "No Peer was selected to push/pull from.");
    }
    return push_pull_view(peer->address());
}


::grpc::Status Client::push_view(std::string address) {
    std::vector<std::shared_ptr<NodeDescriptor>> send_nodes = _view->tx_nodes();
    _view->increment_age();
    ViewProto tx_buffer = ViewProtoHelper<NodeDescriptor>::make_proto(send_nodes);
    
    std::shared_ptr<::google::protobuf::Empty> dummy_resp = std::make_shared<::google::protobuf::Empty>();

    ClientSession sess(_view, address, _timeout);

    return sess.push_view(tx_buffer, dummy_resp);
}

::grpc::Status Client::pull_view(std::string address) {
    ::google::protobuf::Empty dummy_req;
    std::shared_ptr<ViewProto> rx_buffer = std::make_shared<ViewProto>();

    ClientSession sess(_view, address, _timeout);

    return sess.pull_view(dummy_req, rx_buffer);
}

::grpc::Status Client::push_pull_view(std::string address) {
    std::vector<std::shared_ptr<NodeDescriptor>> send_nodes = _view->tx_nodes();
    ViewProto tx_buffer = ViewProtoHelper<NodeDescriptor>::make_proto(send_nodes);
    
    std::shared_ptr<ViewProto> rx_buffer = std::make_shared<ViewProto>();

    ClientSession sess(_view, address, _timeout);

    return sess.push_pull_view(tx_buffer, rx_buffer);
}


Client::Thread::~Thread() {
    stop();
}

void Client::Thread::stop() {
    _active = false;
    if (_thread.joinable()) {
        _thread.join();
        //std::cout << "Client thread stopped" << std::endl;
    }
}

void Client::Thread::signal() {
    _active = false;
}

void Client::Thread::start() {
    _active = true;
    try {
        _thread = std::thread([this]() {
            auto thread_id = std::this_thread::get_id();
            while (_active) {
                //std::cout << "Waking Client Thread: " << thread_id << std::endl; 
                if (_client->_push && _client->_pull) {
                    _client->push_pull_view();
                }
                else if (_client->_push) {
                    _client->push_view();
                }
                else if (_client->_pull) {
                    _client->pull_view();
                }
                std::this_thread::sleep_for(std::chrono::seconds(_client->_wait_time));
            }
        });
        //std::cout << "Client thread creation succeeded." << std::endl;
    }
    catch (const std::system_error& e) {
        _active = false;
        //::cout << "Client thread creation failed: " << e.what() << std::endl;
    }
}

}
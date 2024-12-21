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

#include <grpcpp/grpcpp.h>

#include "view_proto_helper.h"

#include "server.h"

namespace gossip {

/* Rx Only on Server */
::grpc::ServerUnaryReactor* Server::PushView(::grpc::CallbackServerContext* context, const ::gossip::ViewProto* request, ::google::protobuf::Empty* response){
    class Reactor : public grpc::ServerUnaryReactor {
        public:
            Reactor(std::shared_ptr<View> view, const ::gossip::ViewProto* request) {
                // Convert ViewProto to std::vector<std::shared_ptr<NodeDescriptor>>
                std::vector<std::shared_ptr<NodeDescriptor>> new_nodes = ViewProtoHelper<NodeDescriptor>::make_internal(*request);
                // Pass to view obj
                view->rx_nodes(new_nodes);
                view->increment_age();
                Finish(grpc::Status::OK);
            }

        private:
            void OnDone() override {
                // LOG(INFO) << "RPC Completed";
                delete this;
            }

            void OnCancel() override { 
                //LOG(ERROR) << "RPC Cancelled"; 
            }
    };

    return new Reactor(_view, request);
}

/* Tx Only on Server */
::grpc::ServerUnaryReactor* Server::PullView(::grpc::CallbackServerContext* context, const ::google::protobuf::Empty* request, ::gossip::ViewProto* response) {
    class Reactor : public grpc::ServerUnaryReactor {
        public:
            Reactor(std::shared_ptr<View> view, ::gossip::ViewProto* response) {
                std::vector<std::shared_ptr<NodeDescriptor>> send_nodes = view->tx_nodes();
                view->increment_age();
                // Convert std::vector<std::shared_ptr<NodeDescriptor>> to ViewProto
                ViewProtoHelper<NodeDescriptor>::add_to_proto(send_nodes, *response);
                Finish(grpc::Status::OK);
            }

        private:
            void OnDone() override {
                // LOG(INFO) << "RPC Completed";
                delete this;
            }

            void OnCancel() override { 
                //LOG(ERROR) << "RPC Cancelled"; 
            }
    };

    return new Reactor(_view, response);
}


::grpc::ServerUnaryReactor* Server::PushPullView(::grpc::CallbackServerContext* context, const ::gossip::ViewProto* request, ::gossip::ViewProto* response) {
    class Reactor : public grpc::ServerUnaryReactor {
        public:
            Reactor(std::shared_ptr<View> view, const ::gossip::ViewProto* request, ::gossip::ViewProto* response) {
                // Must send ours before processing thiers to prevent sending back the info they just sent us
                std::vector<std::shared_ptr<NodeDescriptor>> send_nodes = view->tx_nodes();
                ViewProtoHelper<NodeDescriptor>::add_to_proto(send_nodes, *response);
                // Convert ViewProto to std::vector<std::shared_ptr<NodeDescriptor>>
                std::vector<std::shared_ptr<NodeDescriptor>> new_nodes = ViewProtoHelper<NodeDescriptor>::make_internal(*request);
                // Pass to view obj
                view->rx_nodes(new_nodes);
                view->increment_age();
                Finish(::grpc::Status::OK);
            }

        private:
            void OnDone() override {
                // LOG(INFO) << "RPC Completed";
                delete this;
            }

            void OnCancel() override { 
                //LOG(ERROR) << "RPC Cancelled"; 
            }
    };

    return new Reactor(_view, request, response);
}

Server::Thread::~Thread() {
    stop();
}

void Server::Thread::stop() {
    _stop_flag = true;

    if (_thread.joinable()) {
        _thread.join();
        //std::cout << "Server thread stopped" << std::endl;
    }
}

void Server::Thread::signal() {
    _stop_flag = true;
}

void Server::Thread::start() {
    try {
        _thread = std::thread([this]() {
            ::grpc::ServerBuilder builder;
            builder.AddListeningPort(_server->_view->self()->address(), ::grpc::InsecureServerCredentials());
            builder.RegisterService(_server.get());
            _grpc_server = builder.BuildAndStart();
            
            while (!_stop_flag) {
                std::this_thread::sleep_for(std::chrono::seconds(1));  // Optionally, add logic to periodically check the stop flag
            }
        });
        //std::cout << "Server thread creation succeeded." << std::endl;
    }
    catch (const std::system_error& e) {
        std::cout << "Server thread creation failed: " << e.what() << std::endl;
    }
}

}
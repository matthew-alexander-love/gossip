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

#pragma once

#include <memory>
#include <string>
#include <thread>

#include <grpcpp/grpcpp.h>

#include "gossip.grpc.pb.h"

#include "view.h"

namespace gossip {

class Server final : public GossipProtocol::CallbackService, public std::enable_shared_from_this<Server> {
    public:
        Server(std::shared_ptr<View> view) : _view(view) {}

        ::grpc::ServerUnaryReactor* PushView(::grpc::CallbackServerContext* context, const ::gossip::ViewProto* request, ::google::protobuf::Empty* response) override;
        ::grpc::ServerUnaryReactor* PullView(::grpc::CallbackServerContext* context, const ::google::protobuf::Empty* request, ::gossip::ViewProto* response) override;
        ::grpc::ServerUnaryReactor* PushPullView(::grpc::CallbackServerContext* context, const ::gossip::ViewProto* request, ::gossip::ViewProto* response) override;

    class Thread {
        public:
            Thread(std::shared_ptr<Server> server) : _server(server), _stop_flag(false) {}

            ~Thread();
            void start();
            void stop();
            void signal();

        private:
            std::shared_ptr<Server> _server;
            std::thread _thread;
            std::unique_ptr<::grpc::Server> _grpc_server;
            std::atomic<bool> _stop_flag;
    };

    std::shared_ptr<Server::Thread> thread() { return std::make_shared<Thread>(shared_from_this()); }

    private:
        std::shared_ptr<View> _view;

};

}

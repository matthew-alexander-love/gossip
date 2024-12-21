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
#include <vector>
#include <thread>
#include <future>
#include <atomic>

#include <grpcpp/grpcpp.h>

#include "gossip.pb.h"
#include "gossip.grpc.pb.h"

#include "view.h"


namespace gossip {

class ClientSession final {
    public:
        ClientSession(std::shared_ptr<View> view, std::string server_address, unsigned int timeout) 
                            :_server_address(server_address), _timeout(timeout), _view(view), 
                            _stub(GossipProtocol::NewStub(grpc::CreateChannel(server_address, grpc::InsecureChannelCredentials()))) {}
        
        ::grpc::Status push_view(const ViewProto& tx_buf, std::shared_ptr<::google::protobuf::Empty> dummy_resp);
        ::grpc::Status pull_view(const ::google::protobuf::Empty& dummy_req, std::shared_ptr<ViewProto> rx_buf);
        ::grpc::Status push_pull_view(const ViewProto& tx_buf, std::shared_ptr<ViewProto> rx_buf);

    private:
        const unsigned int _timeout;
        const std::string _server_address;
        std::shared_ptr<View> _view;
        std::unique_ptr<GossipProtocol::Stub> _stub;
};


/* This class must always be made from a shared ptr and is wrapped that way in gossip_peer_sampling_service */
class Client final : public std::enable_shared_from_this<Client>{
    public:
        Client(bool push, bool pull, unsigned int wait_time, 
                             unsigned int timeout, std::shared_ptr<View> view) 
                            : _push(push), _pull(pull), _view(view), 
                            _wait_time(wait_time), _timeout(timeout),
                            _name("Gossip Protocol Client") {}

        class Thread {
            public:
                Thread(std::shared_ptr<Client> client)
                       :_client(client) , _active(false) {}

                ~Thread();
                void start();
                void stop();
                void signal();

            private:
                std::shared_ptr<Client> _client;
                std::thread _thread;
                std::atomic<bool> _active;
        };

        grpc::Status push_view();
        grpc::Status pull_view();
        grpc::Status push_pull_view();

        grpc::Status push_view(std::string address);
        grpc::Status pull_view(std::string address);
        grpc::Status push_pull_view(std::string address);

        std::shared_ptr<Client::Thread> thread() { return std::make_shared<Thread>(shared_from_this()); }
    private:
        const std::string _name;
        const bool _push;
        const bool _pull;
        const unsigned int _wait_time;
        const unsigned int _timeout;
        std::shared_ptr<View> _view;
};

}
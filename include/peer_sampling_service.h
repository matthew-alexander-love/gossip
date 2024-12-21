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

#include <grpcpp/grpcpp.h>

#include "server.h"
#include "client.h"

// ToDo Add logging
namespace gossip {

class PeerSamplingService {
    public:
        PeerSamplingService(bool push, bool pull, unsigned int wait_time,
                              unsigned int timeout,
                              std::vector<std::string> entry_points,
                              std::shared_ptr<View> view);

        ~PeerSamplingService();

        virtual bool enter();
        virtual bool exit();

        void start_server();
        void stop_server();
        void signal_server();

        void start_client();
        void stop_client();
        void signal_client();

        void start();
        void stop();
        void signal();

        virtual std::string print() const;

        friend std::ostream& operator<<(std::ostream& os, const PeerSamplingService& obj) {
            os << obj.print();
            return os;
        }

        bool entered() const { return _entered; }
        bool push() const { return _push; }
        bool pull() const { return _pull; }
        unsigned int wait_time() const { return _wait_time; }
        unsigned int timeout() const { return _timeout; }
        std::shared_ptr<View> view() { return _view; }
        
    private:
        bool _entered;
        const bool _push;
        const bool _pull;
        const unsigned int _wait_time;
        const unsigned int _timeout;
        std::vector<std::string> _entry_points;
        std::shared_ptr<View> _view;
        std::shared_ptr<Client> _gossip_client;
        std::shared_ptr<Client::Thread> _client_thread;
        std::shared_ptr<Server> _gossip_server;
        std::shared_ptr<Server::Thread> _server_thread;

        void _start_server();
};

}
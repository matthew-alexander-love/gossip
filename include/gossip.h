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
#include <memory>

#include <grpcpp/grpcpp.h>

#include "node_descriptor.h"
#include "view.h"
#include "server.h"
#include "client.h"
#include "peer_sampling_service.h"

namespace gossip {

template <typename PssType, typename ViewType, typename... ViewArgs>
class PSSManager {
    public:
        PSSManager(std::string address, bool push, bool pull, 
                   unsigned int wait_time, unsigned int timeout,
                   std::vector<std::string> entry_points,
                   SelectorType type, ViewArgs... view_args) 
                   : _view(std::make_shared<ViewType>(address, std::forward<ViewArgs>(view_args)...)),
                   _service(std::make_shared<PssType>(push, pull, wait_time, timeout, entry_points, _view)) {
            _view->init_selector(type);
        }

        std::shared_ptr<View::PeerSelector> subscribe(SelectorType type, std::shared_ptr<TSLog> log=nullptr) { return _view->create_subscriber(type, log); }

        bool enter() { return _service->enter(); }
        bool exit() { return _service->exit(); }

        void start() { _service->start(); }
        void stop() { _service->stop(); }

        friend std::ostream& operator<<(std::ostream& os, const PSSManager<PssType, ViewType, ViewArgs...>& obj) {
            os << *obj._service;
            return os;
        }

        bool entered() const { return _service->entered(); }
        bool push() const { return _service->push(); }
        bool pull() const { return _service->pull(); }
        unsigned int wait_time() const { return _service->wait_time(); }
        unsigned int timeout() const { return _service->timeout(); }
        std::shared_ptr<View> view() { return _view; } // Don't shoot yourself in the foot
        std::shared_ptr<PeerSamplingService> service() { return _service; }


    private:
        std::shared_ptr<View> _view;
        std::shared_ptr<PeerSamplingService> _service;
};

}
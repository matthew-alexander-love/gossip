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

#include <grpcpp/grpcpp.h>

#include "view_proto_helper.h"
#include "peer_sampling_service.h"


namespace gossip {

PeerSamplingService::PeerSamplingService(bool push, bool pull, unsigned int wait_time,
                                            unsigned int timeout,
                                            std::vector<std::string> entry_points,
                                            std::shared_ptr<View> view) :
                                            _entered(false), _push(push), _pull(pull), _view(view), _wait_time(wait_time),
                                            _timeout(timeout), _entry_points(entry_points),
                                            _gossip_server(std::make_shared<Server>(view)),
                                            _gossip_client(std::make_shared<Client>(push, pull, wait_time, timeout, view)) {}


PeerSamplingService::~PeerSamplingService() {
    exit();
}

bool PeerSamplingService::enter() {
    if (_entry_points.size() == 0) {
        _entered = true;
        return _entered;
    }
    for (auto& ep : _entry_points) {
        // If haven't gotten responce from at least one entry server, keep track until we do
        if (!_entered) {
            if (_gossip_client->push_pull_view(ep).ok()) {
                _entered = true;
            }
        }
        else {
        // Keep talking even after 1 success because can help with healthier view.
            _gossip_client->push_pull_view(ep);
        }
    }
    return _entered;
}

bool PeerSamplingService::exit() {
    _entered = false;
    return _entered;
}

void PeerSamplingService::start_server() {
    _server_thread = _gossip_server->thread();
    _server_thread->start();
}

void PeerSamplingService::start_client() {
    if (!_entered) {
        return;
    }
    _client_thread = _gossip_client->thread();
    _client_thread->start();
}

void PeerSamplingService::stop_server() {
    //_server_thread->stop();
    _server_thread.reset();
}

void PeerSamplingService::stop_client() {
    //_client_thread->stop();
    _client_thread.reset();
}

void PeerSamplingService::signal_server() {
    _server_thread->signal();
}

void PeerSamplingService::signal_client() {
    _client_thread->signal();
}

void PeerSamplingService::start() {
    start_server();
    start_client();
}

void PeerSamplingService::stop() {
    stop_client();
    stop_server();
}

void PeerSamplingService::signal() {
    signal_client();
    signal_server();
}

std::string PeerSamplingService::print() const{
    std::string str = "PeerSamplingService(Push: " + std::to_string(_push) 
        + ", Pull: " + std::to_string(_pull)
        + ", Timeout: " + std::to_string(_timeout)
        + ", View: " + _view->print()
        + ", EntryPoints: ";
    for (auto& ep : _entry_points) {
        str += ep + ", ";
    }
    str += ")";
    return str;
}

}
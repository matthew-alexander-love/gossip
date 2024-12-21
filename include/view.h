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

#include <cstdint>
#include <vector>
#include <unordered_map>
#include <deque>
#include <string>
#include <memory>
#include <random>
#include <mutex>

#include "ts_ring_buffer.h"
#include "node_descriptor.h"

namespace gossip {

enum class SelectorType {
    TAIL = 0,
    UNIFORM_RANDOM = 1,
    UNIFORM_RANDOM_NO_REPLACEMENT = 2,
    LOGGED_TAIL = 3,
    LOGGED_UNIFORM_RANDOM = 4,
    LOGGED_UNIFORM_RANDOM_NO_REPLACEMENT = 5,
};

struct TSLog {
    virtual ~TSLog() = default;
    virtual void push_back(const std::string& id, const std::string& selected, uint64_t time) = 0;
    virtual std::string to_string() const = 0;

    friend std::ostream& operator<<(std::ostream& os, const TSLog& obj) {
        os << obj.to_string();
        return os;
    }
};

class VectorLog : public TSLog {
    public:
        struct LogEntry {
            LogEntry(const std::string& _id, const std::string& _selected, uint64_t _time) : id(_id), selected(_selected), time(_time) {}
            const std::string id;
            const std::string selected;
            const uint64_t time;

            std::string to_string() const;

            friend std::ostream& operator<<(std::ostream& os, const LogEntry& obj) {
                os << obj.to_string();
                return os;
            }

        };

        void push_back(const std::string& id, const std::string& selected, uint64_t time) override;

        std::vector<LogEntry> data_copy();

        std::string to_string() const override;

    private:
        mutable std::mutex _lock;
        std::vector<LogEntry> _log;
};

struct View {
    /* View(std::string address, args); */
    /* Public methods must be Thread Safe */
    virtual std::shared_ptr<NodeDescriptor> select_peer() = 0; //Internal Selection
    virtual ~View() = default;
    virtual std::vector<std::shared_ptr<NodeDescriptor>> tx_nodes() = 0;
    virtual void rx_nodes(std::vector<std::shared_ptr<NodeDescriptor>>& nodes) = 0;
    virtual void increment_age() = 0;

    virtual void init_selector(SelectorType type, std::shared_ptr<TSLog> log=nullptr) = 0;

    virtual const std::shared_ptr<NodeDescriptor> self() const = 0;
    virtual int size() const = 0;
    virtual bool contains(std::string address) const = 0;
    virtual std::string print() const = 0;

    friend std::ostream& operator<<(std::ostream& os, const View& obj) {
        os << obj.print();
        return os;
    }

    struct PeerSelector {
        virtual ~PeerSelector() = default;
        
        virtual void notify_add(std::shared_ptr<NodeDescriptor> new_node) {}
        virtual void notify_add(std::vector<std::shared_ptr<NodeDescriptor>>& new_nodes) {}
        virtual void notify_delete(std::string& del_address) {}
        virtual void notify_delete(std::vector<std::string>& del_addresses) {}

        virtual std::shared_ptr<NodeDescriptor> select_peer_impl() = 0;
        virtual std::shared_ptr<NodeDescriptor> select_peer() { return select_peer_impl(); }

        virtual std::string print() const = 0;

        friend std::ostream& operator<<(std::ostream& os, const PeerSelector& obj) {
            os << obj.print();
            return os;
        }
    };

    class LoggedPeerSelector : public virtual PeerSelector {
        public:
            LoggedPeerSelector(std::shared_ptr<TSLog> log, const std::string id) : _log(log), _id(id) {}
            virtual ~LoggedPeerSelector() = default;

            std::shared_ptr<NodeDescriptor> select_peer() override;

        private:
            std::shared_ptr<TSLog> _log;
            std::string _id;
    };

    virtual std::shared_ptr<PeerSelector> create_subscriber(SelectorType type, std::shared_ptr<TSLog> log=nullptr) = 0;

    /* Useful for simulation and certain static topology requirements for certain scenarios */
    virtual void manual_insert(std::shared_ptr<NodeDescriptor> new_node) = 0;
    virtual void manual_insert(std::vector<std::shared_ptr<NodeDescriptor>>& new_nodes) = 0;
};


class URView final : public View, public std::enable_shared_from_this<URView> {
    public:

        URView(std::string address, int size, int healing, int swap);
        
        // No copying with mutex
        URView(const URView& other) = delete;
        
        void init_selector(SelectorType type, std::shared_ptr<TSLog> log=nullptr) override;

        std::shared_ptr<NodeDescriptor> select_peer() override;
        std::vector<std::shared_ptr<NodeDescriptor>> tx_nodes() override; 
        void rx_nodes(std::vector<std::shared_ptr<NodeDescriptor>>& nodes) override; 
        void increment_age() override; 
        
        const std::shared_ptr<NodeDescriptor> self() const override { return _self; }
        int max_size() const { return _view.size(); }
        bool contains(std::string address) const override { return (_node_lut.find(address) != _node_lut.end()); }

        int size() const { return _size; }
        int healing() const { return _healing; }
        int swap() const { return _swap; }

        std::string print() const override;

        class TailPeerSelector : public virtual View::PeerSelector {
            public:
                TailPeerSelector(std::shared_ptr<URView> view) : _view(view) {}
                std::shared_ptr<NodeDescriptor> select_peer_impl() override;

                std::string print() const override;
            private:
                std::shared_ptr<URView> _view;
        };

        struct LoggedTailPeerSelector : public TailPeerSelector, public View::LoggedPeerSelector {
            LoggedTailPeerSelector(std::shared_ptr<URView> view, std::shared_ptr<TSLog> log=nullptr) : TailPeerSelector(view), View::LoggedPeerSelector(log, view->self()->address()) {}
        };

        class URPeerSelector : public virtual View::PeerSelector {
            public:
                URPeerSelector(std::shared_ptr<URView> view) : _view(view) {}
                std::shared_ptr<NodeDescriptor> select_peer_impl() override;

                std::string print() const override;
            private:
                std::shared_ptr<URView> _view;
        };

        struct LoggedURPeerSelector : public URPeerSelector, public View::LoggedPeerSelector {
            LoggedURPeerSelector(std::shared_ptr<URView> view, std::shared_ptr<TSLog> log=nullptr) : URPeerSelector(view), View::LoggedPeerSelector(log, view->self()->address()) {}
        };

        class URNRPeerSelector : public virtual View::PeerSelector {         
            public:
                URNRPeerSelector(std::shared_ptr<URView> view);
                std::shared_ptr<NodeDescriptor> select_peer_impl() override;
                void notify_add(std::shared_ptr<NodeDescriptor> new_node) override;
                void notify_add(std::vector<std::shared_ptr<NodeDescriptor>>& new_nodes) override;
                std::string print() const override;
            private:
                std::shared_ptr<URView> _view;
                std::deque<std::shared_ptr<NodeDescriptor>> _qos_queue;
                
                std::shared_ptr<NodeDescriptor> random_selection();
                void permute_qos_queue();
        };

        struct LoggedURNRPeerSelector : public URNRPeerSelector, public View::LoggedPeerSelector {
            LoggedURNRPeerSelector(std::shared_ptr<URView> view, std::shared_ptr<TSLog> log=nullptr) : URNRPeerSelector(view), View::LoggedPeerSelector(log, view->self()->address()) {}
        };

        
        std::shared_ptr<PeerSelector> create_subscriber(SelectorType type, std::shared_ptr<TSLog> log=nullptr) override;

        void manual_insert(std::shared_ptr<NodeDescriptor> new_node) override;
        void manual_insert(std::vector<std::shared_ptr<NodeDescriptor>>& new_nodes) override;

    private:
        mutable std::mutex _lock;
        std::random_device _rd;
        std::mt19937 _eng;
        std::shared_ptr<NodeDescriptor> _self;
        std::vector<std::shared_ptr<NodeDescriptor>> _view;
        std::unordered_map<std::string, std::shared_ptr<NodeDescriptor>> _node_lut;
        mutable std::vector<std::shared_ptr<View::PeerSelector>> _subscribers;

        std::shared_ptr<View::PeerSelector> _selector;
        const int _size;
        const int _healing;
        const int _swap;

        std::vector<std::shared_ptr<NodeDescriptor>> head(int num_get) const;
        void append(std::shared_ptr<NodeDescriptor> new_peer);
        void append(std::vector<std::shared_ptr<NodeDescriptor>>& new_peers);
        void move_old_to_back(int num_move);
        void remove_old(int num_remove);
        void remove_head(int num_remove);
        void remove_random(int num_remove);
        void permute();
};

}
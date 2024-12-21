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


#include <pybind11/pybind11.h>
#include <pybind11/stl.h>
#include <pybind11/gil.h>

#include "node_descriptor.h"
#include "view.h"
#include "peer_sampling_service.h"

namespace py = pybind11;
namespace gossip {

// Trampoline Classes
struct PyView : public View {
    using View::View;

    std::shared_ptr<NodeDescriptor> select_peer() override {
        PYBIND11_OVERRIDE_PURE(std::shared_ptr<NodeDescriptor>, View, select_peer);
    }

    std::vector<std::shared_ptr<NodeDescriptor>> tx_nodes() override {
        PYBIND11_OVERRIDE_PURE(std::vector<std::shared_ptr<NodeDescriptor>>, View, tx_nodes);
    }

    void init_selector(SelectorType type, std::shared_ptr<TSLog> log) override {
        PYBIND11_OVERRIDE_PURE(void, View, init_selector, type, log);
    }

    void rx_nodes(std::vector<std::shared_ptr<NodeDescriptor>>& nodes) override {
        PYBIND11_OVERRIDE_PURE(void, View, rx_nodes, nodes);
    }

    void increment_age() override {
        PYBIND11_OVERRIDE_PURE(void, View, increment_age);
    }

    const std::shared_ptr<NodeDescriptor> self() const override {
        PYBIND11_OVERRIDE_PURE(const std::shared_ptr<NodeDescriptor>, View, self);
    }

    int size() const override {
        PYBIND11_OVERRIDE_PURE(int, View, size);
    }

    bool contains(std::string address) const override {
        PYBIND11_OVERRIDE_PURE(bool, View, contains, address);
    }

    std::string print() const override {
        PYBIND11_OVERRIDE_PURE(std::string, View, print);
    }

    std::shared_ptr<PeerSelector> create_subscriber(SelectorType type, std::shared_ptr<TSLog> log = nullptr) override {
        PYBIND11_OVERRIDE_PURE(std::shared_ptr<PeerSelector>, View, create_subscriber, type, log);
    }

    void manual_insert(std::shared_ptr<NodeDescriptor> new_node) override {
        PYBIND11_OVERRIDE_PURE(void, View, std::shared_ptr<NodeDescriptor>);
    }

    void manual_insert(std::vector<std::shared_ptr<NodeDescriptor>>& new_nodes) override {
        PYBIND11_OVERRIDE_PURE(void, View, std::vector<std::shared_ptr<NodeDescriptor>> new_nodes);
    }
 };

struct PyPeerSelector : public View::PeerSelector {
    using View::PeerSelector::PeerSelector;

    void notify_add(std::shared_ptr<NodeDescriptor> new_node) override {
        PYBIND11_OVERRIDE(void, View::PeerSelector, notify_add, new_node);
    }

    void notify_add(std::vector<std::shared_ptr<NodeDescriptor>>& new_nodes) override {
        PYBIND11_OVERRIDE(void, View::PeerSelector, notify_add, new_nodes);
    }

    void notify_delete(std::string& del_address) override {
        PYBIND11_OVERRIDE(void, View::PeerSelector, notify_delete, del_address);
    }

    void notify_delete(std::vector<std::string>& del_addresses) override {
        PYBIND11_OVERRIDE(void, View::PeerSelector, notify_delete, del_addresses);
    }

    std::shared_ptr<NodeDescriptor> select_peer_impl() override {
        PYBIND11_OVERRIDE_PURE(std::shared_ptr<NodeDescriptor>, View::PeerSelector, select_peer_impl);
    }

    std::string print() const override {
        PYBIND11_OVERRIDE_PURE(std::string, View::PeerSelector, print);
    }
};


struct PyLoggedPeerSelector : public View::LoggedPeerSelector {
    using View::LoggedPeerSelector::LoggedPeerSelector;

    void notify_add(std::shared_ptr<NodeDescriptor> new_node) override {
        PYBIND11_OVERRIDE(void, View::PeerSelector, notify_add, new_node);
    }

    void notify_add(std::vector<std::shared_ptr<NodeDescriptor>>& new_nodes) override {
        PYBIND11_OVERRIDE(void, View::PeerSelector, notify_add, new_nodes);
    }

    void notify_delete(std::string& del_address) override {
        PYBIND11_OVERRIDE(void, View::PeerSelector, notify_delete, del_address);
    }

    void notify_delete(std::vector<std::string>& del_addresses) override {
        PYBIND11_OVERRIDE(void, View::PeerSelector, notify_delete, del_addresses);
    }

    std::shared_ptr<NodeDescriptor> select_peer_impl() override {
        PYBIND11_OVERRIDE_PURE(std::shared_ptr<NodeDescriptor>, View::PeerSelector, select_peer_impl);
    }

    std::string print() const override {
        PYBIND11_OVERRIDE_PURE(std::string, View::PeerSelector, print);
    }
};

struct PyTSLog : public TSLog {
    using TSLog::TSLog;

    void push_back(const std::string& id, const std::string& selected, uint64_t time) override {
        py::gil_scoped_acquire gil;
        PYBIND11_OVERRIDE_PURE(void, TSLog, push_back, id, selected, time);
    }

    std::string to_string() const override {
        PYBIND11_OVERRIDE_PURE(std::string, TSLog, print);
    }
};

PYBIND11_MODULE(_gossip, m) {
    // Bind NodeDescriptor
    py::class_<NodeDescriptor, std::shared_ptr<NodeDescriptor>>(m, "NodeDescriptor")
        // Constructors
        .def(py::init<std::string, uint32_t>(), py::arg("address"), py::arg("age"))
        .def("print", &NodeDescriptor::print)        
        .def_property_readonly("address", &NodeDescriptor::address)
        .def_property("age", [](NodeDescriptor &self) -> uint32_t& { return self.age(); }, // Getter
                             [](NodeDescriptor &self, uint32_t value) { self.age() = value; }) // Setter)

        .def("__str__", &NodeDescriptor::print);

    py::class_<View, PyView, std::shared_ptr<View>>(m, "View")
        .def(py::init<>())  // Default constructor
        .def("select_peer", &View::select_peer)
        .def("tx_nodes", &View::tx_nodes)
        .def("rx_nodes", &View::rx_nodes)
        .def("increment_age", &View::increment_age)
        .def("init_selector", &View::init_selector, py::arg("type"), py::arg("log") = nullptr)
        .def("self", &View::self)
        .def("size", &View::size)
        .def("contains", &View::contains)
        .def("print", &View::print)
        .def("create_subscriber", &View::create_subscriber)
        .def("manual_insert", py::overload_cast<std::shared_ptr<NodeDescriptor>>(&View::manual_insert), py::arg("new_node"))
        .def("manual_insert", py::overload_cast<std::vector<std::shared_ptr<NodeDescriptor>>&>(&View::manual_insert), py::arg("new_nodes"))
        .def("__str__", &View::print);

    // Bind Uniform Random View
    py::class_<URView, View, std::shared_ptr<URView>>(m, "URView")
        .def(py::init<std::string, int, int, int>(), py::arg("address"), py::arg("size"), py::arg("healing"), py::arg("swap"))
        .def("init_selector", &URView::init_selector, py::arg("type"), py::arg("log") = nullptr)
        .def("select_peer", &URView::select_peer)
        .def("tx_nodes", &URView::tx_nodes)
        .def("rx_nodes", &URView::rx_nodes)
        .def("increment_age", &URView::increment_age)
        .def("self", &URView::self)
        .def("max_size", &URView::max_size)
        .def("contains", &URView::contains)
        .def("size", &URView::size)
        .def("healing", &URView::healing)
        .def("swap", &URView::swap)
        .def("create_subscriber", &URView::create_subscriber)
        .def("manual_insert", py::overload_cast<std::shared_ptr<NodeDescriptor>>(&URView::manual_insert), py::arg("new_node"))
        .def("manual_insert", py::overload_cast<std::vector<std::shared_ptr<NodeDescriptor>>&>(&URView::manual_insert), py::arg("new_nodes"))
        .def("__str__", &URView::print);  // Allows the use of str() in Python;

    // Bind the Selector Type Enum
    py::enum_<SelectorType>(m, "SelectorType")
        .value("TAIL", SelectorType::TAIL)
        .value("UNIFORM_RANDOM", SelectorType::UNIFORM_RANDOM)
        .value("UNIFORM_RANDOM_NO_REPLACEMENT", SelectorType::UNIFORM_RANDOM_NO_REPLACEMENT)
        .value("LOGGED_TAIL", SelectorType::LOGGED_TAIL)
        .value("LOGGED_UNIFORM_RANDOM", SelectorType::LOGGED_UNIFORM_RANDOM)
        .value("LOGGED_UNIFORM_RANDOM_NO_REPLACEMENT", SelectorType::LOGGED_UNIFORM_RANDOM_NO_REPLACEMENT)
        .export_values();

    py::class_<View::PeerSelector, PyPeerSelector, std::shared_ptr<View::PeerSelector>>(m, "PeerSelector")
        .def(py::init<>())
        .def("notify_add", py::overload_cast<std::shared_ptr<NodeDescriptor>>(&View::PeerSelector::notify_add), py::arg("new_node"))
        .def("notify_add", py::overload_cast<std::vector<std::shared_ptr<NodeDescriptor>>&>(&View::PeerSelector::notify_add), py::arg("new_nodes"))  // Overload for vector
        .def("notify_delete", py::overload_cast<std::string&>(&View::PeerSelector::notify_delete), py::arg("del_address"))
        .def("notify_delete", py::overload_cast<std::vector<std::string>&>(&View::PeerSelector::notify_delete), py::arg("del_addresses"))  // Overload for vector
        .def("select_peer_impl", &View::PeerSelector::select_peer_impl)
        .def("select_peer", &View::PeerSelector::select_peer)
        .def("__str__", &View::PeerSelector::print);

    py::class_<TSLog, PyTSLog, std::shared_ptr<TSLog>>(m, "TSLog")
        .def(py::init<>())
        .def("push_back", &TSLog::push_back)
        .def("to_string", &TSLog::to_string)
        .def("__str__", &TSLog::to_string);

    py::class_<View::LoggedPeerSelector, View::PeerSelector, PyLoggedPeerSelector, std::shared_ptr<View::LoggedPeerSelector>>(m, "LoggedPeerSelector")
        .def(py::init<std::shared_ptr<TSLog>, const std::string>(), py::arg("log"), py::arg("id"))
        .def("select_peer", &View::LoggedPeerSelector::select_peer);
    
    py::class_<URView::TailPeerSelector, View::PeerSelector, std::shared_ptr<URView::TailPeerSelector>>(m, "TailPeerSelector")
        .def(py::init<std::shared_ptr<URView>>(), py::arg("view"))
        .def("__str__", &URView::TailPeerSelector::print);

    py::class_<URView::LoggedTailPeerSelector, URView::TailPeerSelector, View::LoggedPeerSelector, std::shared_ptr<URView::LoggedTailPeerSelector>>(m, "LoggedTailPeerSelector")
        .def(py::init<std::shared_ptr<URView>, std::shared_ptr<TSLog>>(), py::arg("view"), py::arg("log"))
        .def("__str__", &URView::TailPeerSelector::print);

    py::class_<URView::URPeerSelector, View::PeerSelector, std::shared_ptr<URView::URPeerSelector>>(m, "URPeerSelector")
        .def(py::init<std::shared_ptr<URView>>(), py::arg("view"))
        .def("__str__", &URView::URPeerSelector::print);
    
    py::class_<URView::LoggedURPeerSelector, URView::URPeerSelector, View::LoggedPeerSelector, std::shared_ptr<URView::LoggedURPeerSelector>>(m, "LoggedURPeerSelector")
        .def(py::init<std::shared_ptr<URView>, std::shared_ptr<TSLog>>(), py::arg("view"), py::arg("log"))
        .def("__str__", &URView::URPeerSelector::print);

    py::class_<URView::URNRPeerSelector, View::PeerSelector, std::shared_ptr<URView::URNRPeerSelector>>(m, "URNRPeerSelector")
        .def(py::init<std::shared_ptr<URView>>(), py::arg("view"))
        .def("__str__", &URView::URNRPeerSelector::print);

    py::class_<URView::LoggedURNRPeerSelector, URView::URNRPeerSelector, View::LoggedPeerSelector, std::shared_ptr<URView::LoggedURNRPeerSelector>>(m, "LoggedURNRPeerSelector")
        .def(py::init<std::shared_ptr<URView>, std::shared_ptr<TSLog>>(), py::arg("view"), py::arg("log"))
        .def("__str__", &URView::URNRPeerSelector::print);

    // Expose PeerSamplingService class
    py::class_<PeerSamplingService, std::shared_ptr<PeerSamplingService>>(m, "PeerSamplingService")
        .def(py::init<bool, bool, unsigned int, unsigned int, std::vector<std::string>&, std::shared_ptr<View>>(),
             py::arg("push"), py::arg("pull"), py::arg("wait_time"), py::arg("timeout"),
             py::arg("entry_points") = std::vector<std::string>(), py::arg("view"))
        .def("enter", &PeerSamplingService::enter)
        .def("exit", &PeerSamplingService::exit)
        .def("start_server", &PeerSamplingService::start_server)
        .def("stop_server", &PeerSamplingService::stop_server)
        .def("signal_server", &PeerSamplingService::signal_server)
        .def("start_client", &PeerSamplingService::start_client)
        .def("stop_client", &PeerSamplingService::stop_client)
        .def("signal_client", &PeerSamplingService::signal_client)
        .def("start", &PeerSamplingService::start)
        .def("stop", &PeerSamplingService::stop)
        .def("signal", &PeerSamplingService::signal)
        .def("push", &PeerSamplingService::push)
        .def("pull", &PeerSamplingService::pull)
        .def("entered", &PeerSamplingService::entered)
        .def("wait_time", &PeerSamplingService::wait_time)
        .def("timeout", &PeerSamplingService::timeout)
        .def("view", &PeerSamplingService::view)
        .def("__str__", &PeerSamplingService::print);  // Allows the use of str() in Python
}
}
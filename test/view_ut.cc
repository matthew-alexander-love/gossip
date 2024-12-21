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


#include <vector>
#include <string>
#include <memory>
#include <unordered_set>
#include <iostream>

#include <gtest/gtest.h>

#include <view.h>

using namespace gossip;

struct _URView_ : public ::testing::Test {
    const std::string ip = "192.168.225.1";
    const std::string port = "5012";
    const std::string my_address = ip + ":" + port;
    const int size = 10;
    const int healing = 5;
    const int swap = 5;

    std::shared_ptr<URView> urnr_view() {
        auto view = std::make_shared<URView>(my_address, size, healing, swap);
        view->init_selector(SelectorType::UNIFORM_RANDOM_NO_REPLACEMENT);
        std::cout << "urnr_view created shared_ptr with use count: " << view.use_count() << std::endl;
        return view;
    }

    std::shared_ptr<URView> ur_view() {
        auto view = std::make_shared<URView>(my_address, size, healing, swap);
        view->init_selector(SelectorType::UNIFORM_RANDOM);
        std::cout << "ur_view created shared_ptr with use count: " << view.use_count() << std::endl;
        return view;
    }

    std::shared_ptr<URView> tail_view() {
        auto view = std::make_shared<URView>(my_address, size, healing, swap);
        view->init_selector( SelectorType::TAIL);
        std::cout << "tail_view created shared_ptr with use count: " << view.use_count() << std::endl;
        return view;
    }

    std::pair<std::shared_ptr<URView>, std::shared_ptr<TSLog>> logged_urnr_view() {
        std::shared_ptr<VectorLog> log = std::make_shared<VectorLog>();
        auto view = std::make_shared<URView>(my_address, size, healing, swap);
        view->init_selector(SelectorType::LOGGED_UNIFORM_RANDOM_NO_REPLACEMENT, log);
        std::cout << "logged_urnr_view created shared_ptr with use count: " << view.use_count() << std::endl;
        return {view, log};
    }

    std::pair<std::shared_ptr<URView>, std::shared_ptr<TSLog>> logged_ur_view() {
        std::shared_ptr<VectorLog> log = std::make_shared<VectorLog>();
        auto view = std::make_shared<URView>(my_address, size, healing, swap);
        view->init_selector(SelectorType::LOGGED_UNIFORM_RANDOM, log);
        std::cout << "logged_ur_view created shared_ptr with use count: " << view.use_count() << std::endl;
        return {view, log};
    }

    std::pair<std::shared_ptr<URView>, std::shared_ptr<TSLog>> logged_tail_view() {
        std::shared_ptr<VectorLog> log = std::make_shared<VectorLog>();
        auto view = std::make_shared<URView>(my_address, size, healing, swap);
        view->init_selector(SelectorType::LOGGED_TAIL, log);
        std::cout << "logged_ur_view created shared_ptr with use count: " << view.use_count() << std::endl;
        return {view, log};
    }

    std::vector<std::shared_ptr<NodeDescriptor>> vector_of_nodes(int num_nodes) {
        std::vector<std::shared_ptr<NodeDescriptor>> nodes;
        for (int i = 0; i < num_nodes; ++i) {
            std::shared_ptr<NodeDescriptor> new_node = std::make_shared<NodeDescriptor>(ip + ":" + std::to_string((std::stoi(port) + i + 1)), i);
            nodes.push_back(new_node);
        }
        return nodes;
    }
};

TEST_F(_URView_, construct_logged_ur) {
    auto test_view = logged_ur_view();
}

TEST_F(_URView_, construct_logged_tail) {
    auto test_view = logged_tail_view();
}

TEST_F(_URView_, construct_logged_urnr) {
    auto test_view = urnr_view();
}


TEST_F(_URView_, construct_ur) {
    std::shared_ptr<URView> test_view = ur_view();
}

TEST_F(_URView_, construct_tail) {
    std::shared_ptr<URView> test_view = tail_view();
}

TEST_F(_URView_, construct_urnr) {
    std::shared_ptr<URView> test_view = urnr_view();
}

TEST_F(_URView_, select_empty_view_ur) {
    std::shared_ptr<URView> test_view = ur_view();
    std::shared_ptr<NodeDescriptor> blank_peer = test_view->select_peer();
    ASSERT_EQ(blank_peer, nullptr);
}

TEST_F(_URView_, select_empty_view_tail) {
    std::shared_ptr<URView> test_view = tail_view();
    std::shared_ptr<NodeDescriptor> blank_peer = test_view->select_peer();
    ASSERT_EQ(blank_peer, nullptr);
}

TEST_F(_URView_, select_empty_view_urnr) {
    std::shared_ptr<URView> test_view = urnr_view();
    std::shared_ptr<NodeDescriptor> blank_peer = test_view->select_peer();
    ASSERT_EQ(blank_peer, nullptr);
}

TEST_F(_URView_, select_empty_logged_view_ur) {
    auto test_view = logged_ur_view();
    std::shared_ptr<NodeDescriptor> blank_peer = test_view.first->select_peer();
    ASSERT_EQ(blank_peer, nullptr);
}

TEST_F(_URView_, select_empty_logged_view_urnr) {
    auto test_view = logged_urnr_view();
    std::shared_ptr<NodeDescriptor> blank_peer = test_view.first->select_peer();
    ASSERT_EQ(blank_peer, nullptr);
}

TEST_F(_URView_, select_empty_logged_view_tail) {
    auto test_view = logged_tail_view();
    std::shared_ptr<NodeDescriptor> blank_peer = test_view.first->select_peer();
    ASSERT_EQ(blank_peer, nullptr);
}

TEST_F(_URView_, rx_nodes_contains_ur) {
    std::shared_ptr<URView> test_view = ur_view();
    std::vector<std::shared_ptr<NodeDescriptor>> rx_nodes = vector_of_nodes(size);
    test_view->rx_nodes(rx_nodes);
    ASSERT_EQ(test_view->size(), size);
    for (auto& node : rx_nodes) {
        ASSERT_TRUE(test_view->contains(node->address()));
    }
}

TEST_F(_URView_, rx_nodes_contains_tail) {
    std::shared_ptr<URView> test_view = tail_view();
    std::vector<std::shared_ptr<NodeDescriptor>> rx_nodes = vector_of_nodes(size);
    test_view->rx_nodes(rx_nodes);
    ASSERT_EQ(test_view->size(), size);
    for (auto& node : rx_nodes) {
        ASSERT_TRUE(test_view->contains(node->address()));
    }
}

TEST_F(_URView_, rx_nodes_contains_urnr) {
    std::shared_ptr<URView> test_view = urnr_view();
    std::vector<std::shared_ptr<NodeDescriptor>> rx_nodes = vector_of_nodes(size);
    test_view->rx_nodes(rx_nodes);
    ASSERT_EQ(test_view->size(), size);
    for (auto& node : rx_nodes) {
        ASSERT_TRUE(test_view->contains(node->address()));
    }
}

TEST_F(_URView_, rx_nodes_contains_logged_ur) {
    auto test_view = logged_ur_view();
    std::vector<std::shared_ptr<NodeDescriptor>> rx_nodes = vector_of_nodes(size);
    test_view.first->rx_nodes(rx_nodes);
    ASSERT_EQ(test_view.first->size(), size);
    for (auto& node : rx_nodes) {
        ASSERT_TRUE(test_view.first->contains(node->address()));
    }
}

TEST_F(_URView_, rx_nodes_contains_logged_urnr) {
    auto test_view = logged_urnr_view();
    std::vector<std::shared_ptr<NodeDescriptor>> rx_nodes = vector_of_nodes(size);
    test_view.first->rx_nodes(rx_nodes);
    ASSERT_EQ(test_view.first->size(), size);
    for (auto& node : rx_nodes) {
        ASSERT_TRUE(test_view.first->contains(node->address()));
    }
}

TEST_F(_URView_, rx_nodes_contains_logged_tail) {
    auto test_view = logged_tail_view();
    std::vector<std::shared_ptr<NodeDescriptor>> rx_nodes = vector_of_nodes(size);
    test_view.first->rx_nodes(rx_nodes);
    ASSERT_EQ(test_view.first->size(), size);
    for (auto& node : rx_nodes) {
        ASSERT_TRUE(test_view.first->contains(node->address()));
    }
}

TEST_F(_URView_, tx_nodes_ur) {
    std::shared_ptr<URView> test_view = ur_view();
    std::vector<std::shared_ptr<NodeDescriptor>> dummy_nodes = vector_of_nodes(size);
    test_view->rx_nodes(dummy_nodes);
    int num_send = (size / 2); // (size / 2) - 1 (+ 1 <- put ourself on buffer)
    std::vector<std::shared_ptr<NodeDescriptor>> tx_nodes = test_view->tx_nodes();
    // Check head
    ASSERT_EQ(tx_nodes.size(), num_send);
    // Check Sorting by age
    for (auto& node : tx_nodes) {
        ASSERT_LT(node->age(), size / 2);
    }
}

TEST_F(_URView_, tx_nodes_tail) {
    std::shared_ptr<URView> test_view = tail_view();
    std::vector<std::shared_ptr<NodeDescriptor>> dummy_nodes = vector_of_nodes(size);
    test_view->rx_nodes(dummy_nodes);
    int num_send = (size / 2); // (size / 2) - 1 (+ 1 <- put ourself on buffer)
    std::vector<std::shared_ptr<NodeDescriptor>> tx_nodes = test_view->tx_nodes();
    // Check head
    ASSERT_EQ(tx_nodes.size(), num_send);
    // Check Sorting by age
    for (auto& node : tx_nodes) {
        ASSERT_LT(node->age(), size / 2);
    }
}

TEST_F(_URView_, tx_nodes_urnr) {
    std::shared_ptr<URView> test_view = urnr_view();
    std::vector<std::shared_ptr<NodeDescriptor>> dummy_nodes = vector_of_nodes(size);
    test_view->rx_nodes(dummy_nodes);
    int num_send = (size / 2); // (size / 2) - 1 (+ 1 <- put ourself on buffer)
    std::vector<std::shared_ptr<NodeDescriptor>> tx_nodes = test_view->tx_nodes();
    // Check head
    ASSERT_EQ(tx_nodes.size(), num_send);
    // Check Sorting by age
    for (auto& node : tx_nodes) {
        ASSERT_LT(node->age(), size / 2);
    }
}

TEST_F(_URView_, subscriber_empty_tail) {
    std::shared_ptr<URView> test_view = urnr_view();
    auto selector = test_view->create_subscriber(SelectorType::TAIL);
    auto blank_peer = selector->select_peer();
    ASSERT_EQ(blank_peer, nullptr);
}

TEST_F(_URView_, subscriber_empty_ur) {
    std::shared_ptr<URView> test_view = urnr_view();
    auto selector = test_view->create_subscriber(SelectorType::UNIFORM_RANDOM);
    auto blank_peer = selector->select_peer();
    ASSERT_EQ(blank_peer, nullptr);
}

TEST_F(_URView_, subscriber_empty_urnr) {
    std::shared_ptr<URView> test_view = urnr_view();
    auto selector = test_view->create_subscriber(SelectorType::UNIFORM_RANDOM_NO_REPLACEMENT);
    auto blank_peer = selector->select_peer();
    ASSERT_EQ(blank_peer, nullptr);
}

TEST_F(_URView_, selector_full_tail) {
    std::shared_ptr<URView> test_view = urnr_view();
    std::vector<std::shared_ptr<NodeDescriptor>> dummy_nodes = vector_of_nodes(size);
    test_view->rx_nodes(dummy_nodes);
    auto selector = test_view->create_subscriber(SelectorType::TAIL);
    std::shared_ptr<NodeDescriptor> selected = selector->select_peer();
    std::cout << "Selected: " << *selected << " Curent View: " << *test_view << std::endl;
    ASSERT_TRUE(test_view->contains(selector->select_peer()->address()));
}

TEST_F(_URView_, selector_full_ur) {
    std::shared_ptr<URView> test_view = urnr_view();
    std::vector<std::shared_ptr<NodeDescriptor>> dummy_nodes = vector_of_nodes(size);
    test_view->rx_nodes(dummy_nodes);
    auto selector = test_view->create_subscriber(SelectorType::UNIFORM_RANDOM);
    ASSERT_TRUE(test_view->contains(selector->select_peer()->address()));
}

TEST_F(_URView_, selector_full_urnr) {
    std::shared_ptr<URView> test_view = urnr_view();
    std::vector<std::shared_ptr<NodeDescriptor>> dummy_nodes = vector_of_nodes(size);
    test_view->rx_nodes(dummy_nodes);
    auto selector = test_view->create_subscriber(SelectorType::UNIFORM_RANDOM_NO_REPLACEMENT);
    std::unordered_set<std::string> returned;
    for (int i = 0; i < size; ++i) {
        std::shared_ptr<NodeDescriptor> selected = selector->select_peer();
        std::cout << "URNR Selector: " << *selector << std::endl;
        ASSERT_TRUE(returned.find(selected->address()) == returned.end()) << "Failed peer :" << *selected << std::endl;
        std::cout << "Urnr Selecting: " << *selected << std::endl;
        returned.insert(selected->address());
    }
}

TEST_F(_URView_, increment_age) {
    std::shared_ptr<URView> test_view = urnr_view();
    std::vector<std::shared_ptr<NodeDescriptor>> dummy_nodes = vector_of_nodes(size);
    std::vector<int> ages;
    for (auto& node : dummy_nodes) {
        ages.push_back(node->age());
    }
    test_view->rx_nodes(dummy_nodes);
    test_view->increment_age();
    for (int i = 0; i < ages.size(); i++) {
        ASSERT_EQ(ages[i] + 1, dummy_nodes[i]->age());
    }
}

TEST_F(_URView_, print) {
    std::shared_ptr<URView> test_view = urnr_view();
    std::cout << *test_view << std::endl;
}

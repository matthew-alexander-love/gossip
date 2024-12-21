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

#include <gtest/gtest.h>

#include "gossip.pb.h"

#include "node_descriptor.h"
#include "view_proto_helper.h"

using namespace gossip;

TEST(_ViewProtoHelper_, make_internal) {
    std::string ip = "192.168.225.1";
    std::string port = "5012";
    uint32_t age = 0;

    int num_nodes = 10;
    ::gossip::ViewProto test_view_proto;
        for (int i = 0; i < num_nodes; ++i) {
            ::gossip::NodeDescriptorProto* temp_nd = test_view_proto.add_nodes();
            temp_nd->set_address(ip + ":" + std::to_string((std::stoi(port) + i)));
            temp_nd->set_age(age + i);
        }

    std::vector<std::shared_ptr<NodeDescriptor>> converted = ViewProtoHelper<NodeDescriptor>::make_internal(test_view_proto);

    for (int i = 0; i < num_nodes; ++i) {
        ASSERT_EQ(test_view_proto.nodes(i).address(), converted[i]->address());
        ASSERT_EQ(test_view_proto.nodes(i).age(), converted[i]->age());
    }
}

TEST(_ViewProtoHelper_, add_to_proto) {
    std::string ip = "192.168.225.1";
    std::string port = "5012";
    uint32_t age = 0;

    int num_nodes = 10;
    ::gossip::ViewProto test_view_proto;
    std::vector<std::shared_ptr<NodeDescriptor>> nodes;
    for (int i = 0; i < num_nodes; ++i) {
        std::shared_ptr<NodeDescriptor> new_node = std::make_shared<NodeDescriptor>(ip + ":" + std::to_string((std::stoi(port) + i)), i);
        nodes.push_back(new_node);
    }


    ViewProtoHelper<NodeDescriptor>::add_to_proto(nodes, test_view_proto);
    for (int i = 0; i < num_nodes; ++i) {
        ASSERT_EQ(test_view_proto.nodes(i).address(), nodes[i]->address());
        ASSERT_EQ(test_view_proto.nodes(i).age(), nodes[i]->age());
    }
}

TEST(_ViewProtoHelper_, make_proto) {
    std::string ip = "192.168.225.1";
    std::string port = "5012";
    uint32_t age = 0;

    int num_nodes = 10;
    
    std::vector<std::shared_ptr<NodeDescriptor>> nodes;
    for (int i = 0; i < num_nodes; ++i) {
        std::shared_ptr<NodeDescriptor> new_node = std::make_shared<NodeDescriptor>(ip + ":" + std::to_string((std::stoi(port) + i)), i);
        nodes.push_back(new_node);
    }

    ::gossip::ViewProto test_view_proto = ViewProtoHelper<NodeDescriptor>::make_proto(nodes);
    for (int i = 0; i < num_nodes; ++i) {
        ASSERT_EQ(test_view_proto.nodes(i).address(), nodes[i]->address());
        ASSERT_EQ(test_view_proto.nodes(i).age(), nodes[i]->age());
    }
}
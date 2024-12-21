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

#include <vector>

#include "gossip.pb.h"


namespace gossip {

template <typename NodeDescriptorType>
class ViewProtoHelper {
public:
    // Convert a ViewProto to a vector of shared pointers to NodeDescriptorType
    static std::vector<std::shared_ptr<NodeDescriptorType>> make_internal(const ViewProto& proto) {
        std::vector<std::shared_ptr<NodeDescriptorType>> new_nodes;
        for (const auto& node : proto.nodes()) {
            new_nodes.push_back(std::make_shared<NodeDescriptorType>(node));
        }
        return new_nodes;
    }

    // Add a vector of shared pointers to NodeDescriptorType to a ViewProto
    static void add_to_proto(const std::vector<std::shared_ptr<NodeDescriptorType>>& in_nodes, ViewProto& out_proto) {
        for (const auto& node : in_nodes) {
            NodeDescriptorProto* proto_node = out_proto.add_nodes();
            node->make_proto(proto_node);
        }
    }

    // Create a ViewProto from a vector of shared pointers to NodeDescriptorType
    static ViewProto make_proto(const std::vector<std::shared_ptr<NodeDescriptorType>>& nodes) {
        ViewProto view;
        add_to_proto(nodes, view);
        return view;
    }
    
    static std::shared_ptr<ViewProto> make_shared_proto(const std::vector<std::shared_ptr<NodeDescriptorType>>& nodes) {
        std::shared_ptr<ViewProto> view = std::make_shared<ViewProto>();
        add_to_proto(nodes, view);
        return view;
    }
};

} 
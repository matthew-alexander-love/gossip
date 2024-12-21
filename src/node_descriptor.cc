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

#include "gossip.pb.h"
#include "node_descriptor.h"

namespace gossip {

const void NodeDescriptor::make_proto(NodeDescriptorProto* out_proto) const {
    out_proto->set_address(_address);
    out_proto->set_age(_age);
}

std::string NodeDescriptor::print() const { 
    return "NodeDescriptor(address: " + _address + ", age: " + std::to_string(_age) + ")";
}

}
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

#include <string>
#include <stdint.h>

#include "gossip.pb.h"

namespace gossip {

class NodeDescriptor {
    public:
        NodeDescriptor(std::string address, uint32_t age) : _address(address), _age(age) {}
        NodeDescriptor(const NodeDescriptorProto& proto) : _address(proto.address()), _age(proto.age()) {}

        const void make_proto(NodeDescriptorProto* out_proto) const;

        std::string print() const;

        friend std::ostream& operator<<(std::ostream& os, const NodeDescriptor& obj) {
            os << obj.print();
            return os;
        }

        const std::string& address() const { return _address; }
        uint32_t& age() { return _age; }

    private:
        std::string _address;
        uint32_t _age; 
};

}
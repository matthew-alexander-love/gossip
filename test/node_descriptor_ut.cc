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


#include <string>
#include <stdint.h>

#include <gtest/gtest.h>

#include "gossip.pb.h"

#include "node_descriptor.h"

using namespace gossip;

TEST(_NodeDescriptor_, accessors) {
    std::string address = "192.168.225.1:5012";
    uint32_t age = 0;
    NodeDescriptor test(address, age);

    ASSERT_EQ(test.address(), address);
    ASSERT_EQ(test.age(), age);
}

TEST(_NodeDescriptor_, from_proto) {
    std::string address = "192.168.225.1:5012";
    uint32_t age = 0;
    ::gossip::NodeDescriptorProto test_proto;
    test_proto.set_address(address);
    test_proto.set_age(age);
    NodeDescriptor test(test_proto);

    ASSERT_EQ(test.address(), address);
    ASSERT_EQ(test.age(), age);
}

TEST(_NodeDescriptor_, to_proto) {
    std::string address = "192.168.225.1:5012";
    uint32_t age = 0;
    NodeDescriptor test(address, age);

    ::gossip::NodeDescriptorProto test_proto;
    test.make_proto(&test_proto);

    ASSERT_EQ(test_proto.address(), address);
    ASSERT_EQ(test_proto.age(), age);
}

TEST(_NodeDescriptor_, print) {
    std::string address = "192.168.225.1:5012";
    uint32_t age = 0;
    NodeDescriptor test(address, age);
    
    std::cout << test << std::endl;
}
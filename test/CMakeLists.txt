# GossipSampling
# Copyright (C) Matthew Love 2024 (gossipsampling@gmail.com)
#
# This program is free software: you can redistribute it and/or modify
# it under the terms of the GNU General Public License as published by
# the Free Software Foundation, either version 3 of the License, or
# (at your option) any later version.
#
# This program is distributed in the hope that it will be useful,
# but WITHOUT ANY WARRANTY; without even the implied warranty of
# MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
# GNU General Public License for more details.
#
# You should have received a copy of the GNU General Public License
# along with this program.  If not, see <http://www.gnu.org/licenses/>.

cmake_minimum_required(VERSION 3.14)

set(This gossip_tests)

set(Sources
    test_main.cc
    node_descriptor_ut.cc
    view_ut.cc
    view_proto_helper_ut.cc
    client_server_ut.cc
    peer_sampling_service_ut.cc
)

add_executable(${This} ${Sources})
target_link_libraries(${This} PUBLIC 
    gtest_main
    gossipcpp
    gossip_proto
    grpc_dependencies
)

# build/networking/gossip/test
add_test(
    NAME ${This}
    COMMAND ${This}
)
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

cmake_minimum_required(VERSION 3.10)

# To be removed later when including in others
project(gossip CXX)

option(PYTHON_FE_ENABLED "Build a python front end package" ON)
option(TESTS_ENABLED "Build subsystem tests" OFF)
option(INSTALL_LOCALLY "Clone and build all dependencies in project dir" OFF)
option(BUILD_EXAMPLES "Build the example executables" ON)
option(BUILD_SHARED_LIBS "Build libraries as shared libraries" OFF)

if (PYTHON_FE_ENABLED)
    # Enable position-independent code for Python modules
    set(CMAKE_POSITION_INDEPENDENT_CODE ON)
endif()

set(CMAKE_PREFIX_PATH "${CMAKE_PREFIX_PATH};${CMAKE_SOURCE_DIR}/.local")


if(INSTALL_LOCALLY)
    if (TESTS_ENABLED)
        execute_process(
            #COMMAND sudo /bin/bash ${CMAKE_SOURCE_DIR}/install_dep_local.sh test
            COMMAND /bin/bash ${CMAKE_SOURCE_DIR}/install_dep_local.sh test
            WORKING_DIRECTORY ${CMAKE_SOURCE_DIR}
        )
    else()
        execute_process(
            #COMMAND sudo /bin/bash ${CMAKE_SOURCE_DIR}/install_dep_local.sh prod
            COMMAND /bin/bash ${CMAKE_SOURCE_DIR}/install_dep_local.sh prod
            WORKING_DIRECTORY ${CMAKE_SOURCE_DIR}
        )
    endif()

    if(TESTS_ENABLED)
        add_subdirectory(googletest)
        enable_testing()
    endif()

    set(_PROTOBUF_PROTOC "${CMAKE_SOURCE_DIR}/.local/bin/protoc" CACHE STRING "Path to protoc compiler")
    set(_GRPC_CPP_PLUGIN_EXECUTABLE "${CMAKE_SOURCE_DIR}/.local/bin/grpc_cpp_plugin" CACHE STRING "Path to gRPC compiler plugin")

else()
  
    if (TESTS_ENABLED)
        find_package(GTest REQUIRED)
        enable_testing()
    endif()

    find_program(_PROTOBUF_PROTOC 
        NAMES protoc
        PATHS /usr/bin /usr/local/bin
    )

    find_program(_GRPC_CPP_PLUGIN_EXECUTABLE 
        NAMES grpc_cpp_plugin
        PATHS /usr/bin /usr/local/bin  
    )
endif()

find_package(Protobuf CONFIG REQUIRED)
find_package(gRPC REQUIRED)


# Check if they are found
if (NOT _PROTOBUF_PROTOC)
    message(FATAL_ERROR "protoc not found in ${CMAKE_SOURCE_DIR}/.local/bin")
endif()

if (NOT _GRPC_CPP_PLUGIN_EXECUTABLE)
    message(FATAL_ERROR "grpc_cpp_plugin not found in ${CMAKE_SOURCE_DIR}/.local/bin")
endif()


add_library(grpc_dependencies INTERFACE)
target_link_libraries(grpc_dependencies INTERFACE protobuf::libprotobuf gRPC::grpc++)


# Proto Generation
get_filename_component(gossip_proto "proto/gossip.proto" ABSOLUTE)
get_filename_component(proto_path "${gossip_proto}" PATH)

set(gossip_proto_srcs "${CMAKE_CURRENT_BINARY_DIR}/gossip.pb.cc")
set(gossip_proto_hdrs "${CMAKE_CURRENT_BINARY_DIR}/gossip.pb.h")
set(gossip_grpc_srcs "${CMAKE_CURRENT_BINARY_DIR}/gossip.grpc.pb.cc")
set(gossip_grpc_hdrs "${CMAKE_CURRENT_BINARY_DIR}/gossip.grpc.pb.h")

add_custom_command(
    OUTPUT "${gossip_proto_srcs}" "${gossip_proto_hdrs}" "${gossip_grpc_srcs}" "${gossip_grpc_hdrs}"
    COMMAND ${_PROTOBUF_PROTOC}
    ARGS --grpc_out "${CMAKE_CURRENT_BINARY_DIR}"
        --cpp_out "${CMAKE_CURRENT_BINARY_DIR}"
        -I "${proto_path}"
        --plugin=protoc-gen-grpc="${_GRPC_CPP_PLUGIN_EXECUTABLE}"
        "${gossip_proto}"
    DEPENDS "${gossip_proto}"
)

add_custom_target(proto_gen ALL
    DEPENDS "${gossip_proto_srcs}" "${gossip_proto_hdrs}" "${gossip_grpc_srcs}" "${gossip_grpc_hdrs}"
)

add_library(gossip_proto
    ${gossip_proto_srcs}
    ${gossip_proto_hdrs}
    ${gossip_grpc_srcs}
    ${gossip_grpc_hdrs}
    
)
target_include_directories(gossip_proto PUBLIC "${CMAKE_CURRENT_BINARY_DIR}" "${CMAKE_CURRENT_SOURCE_DIR}/include")
target_link_libraries(gossip_proto PUBLIC grpc_dependencies)


# Gossip Peer Selection Service
set(Sources
    src/node_descriptor.cc
    src/view.cc
    src/client.cc
    src/server.cc
    src/peer_sampling_service.cc
)

set(Headers
    include/view_proto_helper.h
    include/node_descriptor.h
    include/view.h
    include/client.h
    include/server.h
    include/peer_sampling_service.h
    include/ts_ring_buffer.h
)

add_library(gossipcpp
    ${Sources}
    ${Headers}
)

target_include_directories(gossipcpp PUBLIC "${CMAKE_CURRENT_SOURCE_DIR}/include")
target_link_libraries(gossipcpp gossip_proto grpc_dependencies)

if(PYTHON_FE_ENABLED)
    #set(PYTHON_EXECUTABLE "${CMAKE_CURRENT_SOURCE_DIR}/.local/bin/python")
    #list(APPEND CMAKE_PREFIX_PATH "${CMAKE_CURRENT_SOURCE_DIR}/.local/lib/python3.10/site-packages")
    find_package(Python3 REQUIRED COMPONENTS Interpreter Development)
    message(STATUS "Python interpreter: ${Python3_EXECUTABLE}")
    message(STATUS "Python library: ${Python3_LIBRARIES}")
    message(STATUS "Python include directory: ${Python3_INCLUDE_DIRS}")
    find_package(pybind11 REQUIRED)

    pybind11_add_module(_gossip MODULE py_bindings.cc)
    target_link_libraries(_gossip PRIVATE gossipcpp pybind11::module)

    install(TARGETS _gossip DESTINATION .)
endif()

if (TESTS_ENABLED)
    add_subdirectory(test)
endif()

if (BUILD_EXAMPLES)
    add_executable(gossip_client_example example/cpp/gossip_client_example.cc)
    target_link_libraries(gossip_client_example PRIVATE gossipcpp)

    add_executable(gossip_entry_server_example example/cpp/gossip_entry_server_example.cc)
    target_link_libraries(gossip_entry_server_example PRIVATE gossipcpp)
endif()
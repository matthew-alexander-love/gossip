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

#gtest
if [ "$1" == "test" ]; then
    if [ ! -d "googletest" ]; then
        echo "Cloning gTest repository..."
        git clone https://github.com/google/googletest.git
    fi
fi

#grpc
MY_INSTALL_DIR="${PWD}/.local"
mkdir -p $MY_INSTALL_DIR
export PATH="$MY_INSTALL_DIR/bin:$PATH"
if [ ! -d "grpc" ]; then
    echo "Cloning gRPC repository..."
    git clone --recurse-submodules -b v1.66.0 --shallow-submodules https://github.com/grpc/grpc
fi

cd grpc || exit 1

mkdir -p cmake/build
cd cmake/build || exit 1

echo "Configuring gRPC build..."
cmake -DgRPC_INSTALL=ON -DgRPC_BUILD_TESTS=OFF -DCMAKE_INSTALL_PREFIX="$MY_INSTALL_DIR" ../..

if [ $? -ne 0 ]; then
    echo "CMake configuration failed!"
    exit 1
fi

echo "Building gRPC..."
make -j 4  # Adjust the number of cores as necessary

# Check if make was successful
if [ $? -ne 0 ]; then
    echo "Build failed!"
    exit 1
fi

# Install gRPC
echo "Installing gRPC..."
make install

# Verify installation
if [ $? -ne 0 ]; then
    echo "Installation failed!"
    exit 1
fi

echo "gRPC installation completed successfully!"

# Return to the previous directory
cd - || exit 1
#!/bin/bash

# Build release

# to enable lto :
# export DEB_BUILD_MAINT_OPTIONS=optimize=+lto
# to disable :
# export DEB_BUILD_MAINT_OPTIONS=optimize=-lto

# CMake should use gcc and g++ 12 (ideally)
# to force it :

# local/docker :
# export CC=/usr/local/bin/gcc
# export CXX=/usr/local/bin/g++

# CCIPL :
# export CC=/trinity/shared/apps/cv-standard/gcc/12.1.0/bin/gcc
# export CXX=/trinity/shared/apps/cv-standard/gcc/12.1.0/bin/g++

rm -rf build_release
mkdir build_release
cd build_release || exit

cmake -DCMAKE_BUILD_TYPE=Release ..
make -j
cd ..

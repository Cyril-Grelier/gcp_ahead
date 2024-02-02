cmake_minimum_required(VERSION 3.14 FATAL_ERROR)
project(gc)

set(CMAKE_CXX_STANDARD 17)

# Build with Realease if not specified
if(NOT CMAKE_BUILD_TYPE)
    set(CMAKE_BUILD_TYPE "Release")
endif()

# do not use LTO for debug builds
if(CMAKE_BUILD_TYPE MATCHES Release)
    set(CMAKE_INTERPROCEDURAL_OPTIMIZATION TRUE)
    SET(CMAKE_AR "gcc-ar")
endif()

# -Wsign-conversion -Weffc++
set(CMAKE_CXX_FLAGS_DEBUG "${CMAKE_ARCH_FLAGS} -DDEBUG -O0 -g -ldl -lm -isystem dir -Wall -Wextra -Wshadow -Wnon-virtual-dtor -Wold-style-cast -Wcast-align -Wunused -Woverloaded-virtual -Wpedantic -Wconversion -Wmisleading-indentation -Wduplicated-cond -Wduplicated-branches -Wlogical-op -Wnull-dereference -Wuseless-cast -Wdouble-promotion -Wformat=2")
set(CMAKE_CXX_FLAGS_RELEASE "${CMAKE_ARCH_FLAGS} -O3 -flto=auto -DNDEBUG -ldl -lm -Wall -s -isystem dir -Wall -Wextra -Wshadow -Wnon-virtual-dtor -Wold-style-cast -Wcast-align -Wunused -Woverloaded-virtual -Wpedantic -Wconversion -Wmisleading-indentation -Wduplicated-cond -Wduplicated-branches -Wlogical-op -Wnull-dereference -Wuseless-cast -Wdouble-promotion -Wformat=2")

list(APPEND CMAKE_PREFIX_PATH "$PWD/../thirdparty/libtorch")
find_package(Torch REQUIRED)
set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} ${TORCH_CXX_FLAGS}")

find_package(OpenMP REQUIRED)

add_executable(${CMAKE_PROJECT_NAME}
    src/main.cpp

    # utils
    src/utils/random_generator.cpp src/utils/random_generator.hpp
    src/utils/utils.cpp src/utils/utils.hpp
    src/utils/parse.cpp src/utils/parse.hpp

    # representation
    src/representation/Graph.cpp src/representation/Graph.hpp
    src/representation/Method.hpp
    src/representation/Parameters.cpp src/representation/Parameters.hpp
    src/representation/Solution.cpp src/representation/Solution.hpp
    src/representation/Node.cpp src/representation/Node.hpp

    # methods
    src/methods/adaptive.cpp src/methods/adaptive.hpp
    src/methods/crossover.cpp src/methods/crossover.hpp
    src/methods/GreedyAlgorithm.cpp src/methods/GreedyAlgorithm.hpp
    src/methods/insertion.cpp src/methods/insertion.hpp
    src/methods/LocalSearchAlgorithm.cpp src/methods/LocalSearchAlgorithm.hpp
    src/methods/MemeticAlgorithm.cpp src/methods/MemeticAlgorithm.hpp
    src/methods/neural_network.cpp src/methods/neural_network.hpp
    src/methods/selection.cpp src/methods/selection.hpp

    src/methods/MCTS.cpp src/methods/MCTS.hpp
    src/methods/SimulationHelper.cpp src/methods/SimulationHelper.hpp
)

set_property(TARGET ${CMAKE_PROJECT_NAME} PROPERTY CXX_STANDARD 17)

# add dependencies
set(CPM_DOWNLOAD_VERSION 0.35.5)

if(CPM_SOURCE_CACHE)
    set(CPM_DOWNLOAD_LOCATION "${CPM_SOURCE_CACHE}/cpm/CPM_${CPM_DOWNLOAD_VERSION}.cmake")
elseif(DEFINED ENV{CPM_SOURCE_CACHE})
    set(CPM_DOWNLOAD_LOCATION "$ENV{CPM_SOURCE_CACHE}/cpm/CPM_${CPM_DOWNLOAD_VERSION}.cmake")
else()
    set(CPM_DOWNLOAD_LOCATION "${CMAKE_BINARY_DIR}/cmake/CPM_${CPM_DOWNLOAD_VERSION}.cmake")
endif()

if(NOT(EXISTS ${CPM_DOWNLOAD_LOCATION}))
    message(STATUS "Downloading CPM.cmake to ${CPM_DOWNLOAD_LOCATION}")
    file(DOWNLOAD
        https://github.com/TheLartians/CPM.cmake/releases/download/v${CPM_DOWNLOAD_VERSION}/CPM.cmake
        ${CPM_DOWNLOAD_LOCATION}
    )
endif()

include(${CPM_DOWNLOAD_LOCATION})

CPMAddPackage(
    GITHUB_REPOSITORY jarro2783/cxxopts
    VERSION 2.2.1
    OPTIONS "CXXOPTS_BUILD_EXAMPLES NO" "CXXOPTS_BUILD_TESTS NO" "CXXOPTS_ENABLE_INSTALL YES"
)

CPMAddPackage(
    NAME fmt
    GIT_TAG 9.0.0
    GITHUB_REPOSITORY fmtlib/fmt
)

CPMAddPackage(
    NAME nlohmann_json
    VERSION 3.7.3

    # the git repo is incredibly large, so we download the archived include directory
    URL https://github.com/nlohmann/json/releases/download/v3.11.2/include.zip
    URL_HASH SHA256=e5c7a9f49a16814be27e4ed0ee900ecd0092bfb7dbfca65b5a421b774dccaaed
)

if(nlohmann_json_ADDED)
    add_library(nlohmann_json INTERFACE IMPORTED)
    target_include_directories(nlohmann_json INTERFACE ${nlohmann_json_SOURCE_DIR}/include)
endif()

# link dependencies
target_link_libraries(${CMAKE_PROJECT_NAME} fmt cxxopts OpenMP::OpenMP_CXX "${TORCH_LIBRARIES}" nlohmann_json)

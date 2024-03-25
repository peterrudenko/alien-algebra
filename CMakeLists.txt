cmake_minimum_required(VERSION 3.11)

project(egraph CXX)

#set(CMAKE_VERBOSE_MAKEFILE ON)
set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED True)
set(CMAKE_BUILD_TYPE "Debug")

include_directories(.
        ThirdParty/e-graph
        ThirdParty/cereal/include
        ThirdParty/pegtl/include)

add_executable(Game Source/Game.cpp)

cmake_minimum_required(VERSION 3.11)

project(Game CXX)

#set(CMAKE_VERBOSE_MAKEFILE ON)
set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED True)
set(CMAKE_BUILD_TYPE "Debug")

# CLI client

add_executable(CliGame Source/CliClient.cpp)

target_include_directories(CliGame PRIVATE
        Source
        ThirdParty/e-graph
        ThirdParty/cereal/include
        ThirdParty/pegtl/include)

set_target_properties(CliGame
        PROPERTIES OUTPUT_NAME "game")

# Web client

add_executable(WebGame Source/WebClient.cpp)

target_include_directories(WebGame PRIVATE
        Source
        ThirdParty/e-graph
        ThirdParty/cereal/include
        ThirdParty/pegtl/include
        /opt/cheerp/include/client)

target_compile_options(WebGame PRIVATE
        "-fexceptions")

set_target_properties(WebGame
        PROPERTIES OUTPUT_NAME "game.js")

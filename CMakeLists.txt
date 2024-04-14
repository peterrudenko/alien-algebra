cmake_minimum_required(VERSION 3.11)

project(Game CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED True)
set(CMAKE_BUILD_TYPE "Debug")

# CLI client

if(NOT DEFINED BUILD_WEB_CLIENT)

    add_executable(CliGame Source/CliClient.cpp)

    target_include_directories(CliGame PRIVATE
            Source
            ThirdParty/e-graph
            ThirdParty/pegtl/include)

    set_target_properties(CliGame
            PROPERTIES OUTPUT_NAME "game")

endif()

# Web client

if(DEFINED BUILD_WEB_CLIENT)

    add_executable(WebGame Source/WebClient.cpp)

    target_include_directories(WebGame PRIVATE
            Source
            ThirdParty/e-graph
            ThirdParty/pegtl/include
            /opt/cheerp/include/client
            c:/cheerp/include/client
            c:/cheerp/lib/clang/16/include)

    target_compile_options(WebGame PRIVATE
            "-fexceptions"
            "-Wno-unqualified-std-cast-call"
            "-Wno-unused-command-line-argument")

    target_compile_definitions(WebGame PRIVATE WEB_CLIENT=1)

    set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -O2 -cheerp-no-credits -cheerp-preexecute -target cheerp")

    set_target_properties(WebGame
            PROPERTIES OUTPUT_NAME "game.js")

endif()

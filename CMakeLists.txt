cmake_minimum_required(VERSION 3.11)

project(egraph CXX)

#set(CMAKE_VERBOSE_MAKEFILE ON)
set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED True)
set(CMAKE_BUILD_TYPE "Debug")

include_directories(.)

include(FetchContent)
FetchContent_Declare(
    pegtl
    GIT_REPOSITORY https://github.com/taocpp/PEGTL.git
    GIT_TAG        3.2.7
)
FetchContent_MakeAvailable(pegtl)

add_executable(Game Source/Game.cpp)
target_link_libraries(Game PRIVATE taocpp::pegtl)

cmake_minimum_required(VERSION 3.14)
project(CliqueProblem)

set(CMAKE_CXX_STANDARD 17)

# Fetch Highs library automatically
include(FetchContent)
FetchContent_Declare(
    highs
    GIT_REPOSITORY https://github.com/ERGO-Code/HiGHS.git
    GIT_TAG        v1.14.0
)
FetchContent_MakeAvailable(highs)

add_executable(clique_problem main.cpp)
target_link_libraries(clique_problem PRIVATE highs)

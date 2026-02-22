cmake_minimum_required(VERSION 3.14)
project(MatrixGameSolver)

set(CMAKE_CXX_STANDARD 17)

# Fetch Highs library automatically
include(FetchContent)
FetchContent_Declare(
    highs
    GIT_REPOSITORY https://github.com/ERGO-Code/HiGHS.git
    GIT_TAG        v1.7.0 # or latest stable
)
FetchContent_MakeAvailable(highs)

add_executable(matrix_solver main.cpp)
target_link_libraries(matrix_solver PRIVATE highs)

# For reading CSV files, we'll write a simple parser in main.cpp

cmake_minimum_required(VERSION 3.10)

project(MemGrid)

add_library(MemGrid AllocManager.c)

# tests
add_executable(TestAllocManager Tests/TestAllocManager)
target_link_libraries(TestAllocManager MemGrid)
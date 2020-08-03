cmake_minimum_required(VERSION 3.10)
enable_testing()

project(MemGrid)

add_library(MemGrid AllocManager.c)

# tests
add_executable(TestAllocManager Tests/TestAllocManager)
target_link_libraries(TestAllocManager MemGrid)
add_test(NAME test_alloc_manager COMMAND TestAllocManager)

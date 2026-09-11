cmake_minimum_required(VERSION 3.16)
project(PAL VERSION 1.0.0 LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} /std:c++latest /W4")

# Find Catch2 for testing
find_package(Catch2 REQUIRED)

# Main library
add_library(arllm_lib
    src/common/types.cpp
    src/tokenizer/vocabulary.cpp
    src/tokenizer/tokenizer.cpp
    src/parser/ast_nodes.cpp
    src/parser/parser.cpp
    src/type_system/type_checker.cpp
    src/gimple/builder.cpp
)

target_include_directories(arllm_lib PUBLIC ${CMAKE_CURRENT_SOURCE_DIR}/src)

# Demo executable
add_executable(demo examples/demo.cpp)
target_link_libraries(demo PRIVATE arllm_lib)
target_include_directories(demo Private ${CMAKE_CURRENT_SOURCE_DIR}/src)

# Enable testing
enable_testing()

# Test executable
add_executable(arllm_tests
    tests/test_vocabulary.cpp
    tests/test_tokenizer.cpp
)

target_link_libraries(arllm_tests PRIVATE arllm_lib Catch2::Catch2WithMain)

# Register test with ctest
add_test(NAME vocabulary_tests COMMAND arllm_tests)
add_test(NAME tokenizer_tests COMMAND arllm_tests)


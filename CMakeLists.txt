cmake_minimum_required(VERSION 3.16)
project(LowLatencyMatchingEngine LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

option(LLX_ENABLE_LTO "Enable link-time optimization" ON)
option(LLX_ENABLE_NATIVE "Enable -march=native for local benchmarking" OFF)

if(CMAKE_CXX_COMPILER_ID MATCHES "GNU|Clang")
  add_compile_options(
    -Wall
    -Wextra
    -Wpedantic
    -Wconversion
    -Wshadow
    -Wnon-virtual-dtor
    -O3
  )

  if(LLX_ENABLE_NATIVE)
    add_compile_options(-march=native)
  endif()

  if(LLX_ENABLE_LTO)
    add_compile_options(-flto)
    add_link_options(-flto)
  endif()
endif()

add_library(llx_core
  src/book/ladder_book.cpp
  src/core/matching_engine.cpp
)

target_include_directories(llx_core PUBLIC include)
target_compile_features(llx_core PUBLIC cxx_std_17)
target_link_libraries(llx_core PUBLIC pthread)

add_executable(llx_bench apps/bench_main.cpp)
target_link_libraries(llx_bench PRIVATE llx_core)

add_executable(llx_replay apps/replay_main.cpp)
target_link_libraries(llx_replay PRIVATE llx_core)

add_executable(llx_tests apps/tests_main.cpp)
target_link_libraries(llx_tests PRIVATE llx_core)

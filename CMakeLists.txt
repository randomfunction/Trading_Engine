cmake_minimum_required(VERSION 3.14)
project(TradingEngine)

set(CMAKE_CXX_STANDARD 17)

include(FetchContent)
FetchContent_Declare(
  prometheus-cpp
  GIT_REPOSITORY https://github.com/jupp0r/prometheus-cpp.git
  GIT_TAG v1.1.0
)
set(ENABLE_TESTING OFF CACHE BOOL "" FORCE)
set(ENABLE_PUSH OFF CACHE BOOL "" FORCE)
FetchContent_MakeAvailable(prometheus-cpp)

include_directories(include)

add_executable(engine src/main.cpp src/order_book.cpp)
target_link_libraries(engine PRIVATE prometheus-cpp::core prometheus-cpp::pull z)
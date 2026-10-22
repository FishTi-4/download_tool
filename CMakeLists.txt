cmake_minimum_required(VERSION 3.20)

project(download_tools CXX)

set(CMAKE_CXX_STANDARD 20)

file(GLOB SRC ./src/*.cpp)

# add_library(download_class ${SRC})
# target_include_directories(download_class PRIVATE ./include)

add_executable(test_find_servet ./test/find_socket_test.cpp)
add_executable(test_client ./test/client_test.cpp)
add_executable(test_thread ./test/thread_test.cpp)
add_executable(test_timer_wheel ./test/timer_wheel_test.cpp)


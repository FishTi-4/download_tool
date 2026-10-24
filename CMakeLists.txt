cmake_minimum_required(VERSION 3.20)

project(download_tools CXX)

set(CMAKE_CXX_STANDARD 20)

file(GLOB SRC ./src/*.cpp)

add_library(download_class ${SRC})
target_include_directories(download_class PRIVATE ./include)

if(WIN32)
  target_link_libraries(download_class PUBLIC ws2_32)
endif()

add_executable(test_find_servet ./test/find_socket_test.cpp)
target_link_libraries(test_find_servet download_class)

add_executable(test_client ./test/client_test.cpp)
target_link_libraries(test_client download_class)

add_executable(test_thread ./test/thread_test.cpp)
target_link_libraries(test_thread download_class)

add_executable(test_timer_wheel ./test/timer_wheel_test.cpp)
target_link_libraries(test_timer_wheel download_class)


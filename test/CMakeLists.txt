cmake_minimum_required(VERSION 3.15)

project(httpserver_test)

set(CMAKE_CXX_REQUIRED 11)
set(CMAKE_CXX_STANDARD_REQUIRED true)

add_executable(TestMaxConns TestMaxConns.cc)

add_executable(TestTimerManager TestTimerManager.cc ../src/timer/TimerManager.cc)
target_include_directories(TestTimerManager PUBLIC "../src")

add_executable(Webbench webbench/webbench.c)
target_compile_options(Webbench PUBLIC -O2)

add_executable(TestHttpConn TestHttpConn.cc)

add_executable(TestBlockQueue TestBlockQueue.cc)
target_include_directories(TestBlockQueue PUBLIC "../src")
target_link_options(TestBlockQueue PUBLIC -pthread)
target_compile_options(TestBlockQueue PUBLIC -pthread)

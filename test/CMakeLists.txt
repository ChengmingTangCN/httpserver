cmake_minimum_required(VERSION 3.10)

project(httpserver)

set(CMAKE_CXX_REQUIRED 11)
set(CMAKE_CXX_STANDARD_REQUIRED true)

add_executable(TestMaxConns TestMaxConns.cc)

add_executable(TestTimerManager TestTimerManager.cc ../src/timer/TimerManager.cc)
target_include_directories(TestTimerManager PUBLIC "../src")

add_executable(Webbench webbench/webbench.c)
target_compile_options(Webbench PUBLIC -g -o0)

add_executable(TestHttpConn TestHttpConn.cc)

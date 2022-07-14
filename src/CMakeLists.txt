cmake_minimum_required(VERSION 3.10)

project(httpserver)

set(CMAKE_CXX_REQUIRED 11)
set(CMAKE_CXX_STANDARD_REQUIRED true)

add_executable(HttpServer Main.cc)

add_library(Lib buffer/Buffer.cc
                http/HttpConn.cc http/HttpRequest.cc http/HttpResponse.cc
                pool/ThreadPool.cc
                timer/TimerManager.cc
                server/Epoller.cc server/Server.cc)

target_include_directories(HttpServer PUBLIC "${PROJECT_SOUCE_DIR}")
target_include_directories(Lib PUBLIC "${PROJECT_SOUCE_DIR}")
target_include_directories(Lib INTERFACE "${PROJECT_SOURCE_DIR}")

target_compile_options(Lib PUBLIC -pthread -O2)
target_link_options(Lib PUBLIC -pthread)

target_link_libraries(HttpServer PUBLIC Lib)
target_compile_options(HttpServer PUBLIC -O2)

cmake_minimum_required(VERSION 3.7)
project(ChatApp C)

set(CMAKE_C_STANDARD 99)
set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} -pthread")
add_executable(Server Server.c)
add_executable(Klient Klient.c)
cmake_minimum_required(VERSION 3.13)
project(ChatServer)

set(CMAKE_CXX_STANDARD 14)

add_executable(ChatServer main.cpp ServerFunctionality.h)
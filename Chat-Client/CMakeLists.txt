cmake_minimum_required(VERSION 3.13)
project(Chat_Client)

set(CMAKE_CXX_STANDARD 14)

include_directories(cpp-chat-client)

add_executable(Chat_Client
        cpp-chat-client/Application.cpp
        cpp-chat-client/Application.h
        cpp-chat-client/CircularLineBuffer.cpp
        cpp-chat-client/CircularLineBuffer.h
        cpp-chat-client/Client.cpp
        cpp-chat-client/Client.h
        cpp-chat-client/main.cpp
        cpp-chat-client/vusocket.cpp
        cpp-chat-client/vusocket.h)

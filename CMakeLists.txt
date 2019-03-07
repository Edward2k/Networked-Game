cmake_minimum_required(VERSION 3.13)
project(GAME)

set(CMAKE_CXX_STANDARD 14)

include_directories(Game-Client)
include_directories(Game-Server)

add_executable(CLIENT
        Game-Client/Application.cpp
        Game-Client/Application.h
        Game-Client/CircularLineBuffer.cpp
        Game-Client/CircularLineBuffer.h
        Game-Client/Client.cpp
        Game-Client/Client.h
        Game-Client/main.cpp
        Game-Client/vusocket.cpp
        Game-Client/vusocket.h)

add_executable(SERVER
        Game-Server/main_server.cpp
        Game-Server/ServerFunctionality.h)

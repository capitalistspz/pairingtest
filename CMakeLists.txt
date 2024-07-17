cmake_minimum_required(VERSION 3.28)
project(pairingtest)

set(CMAKE_CXX_STANDARD 23)

add_executable(pairingtest main.cpp
)

wut_create_rpx(pairingtest)
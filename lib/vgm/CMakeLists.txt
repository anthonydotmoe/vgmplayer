cmake_minimum_required(VERSION 3.2)
project(libvgm)

set(CMAKE_C_STANDARD 99)

add_library(libvgm vgm.c)
target_include_directories(libvgm INTERFACE .)
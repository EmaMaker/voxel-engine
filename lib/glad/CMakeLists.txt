cmake_minimum_required(VERSION 3.2)
project(glad C CXX)

set(SOURCE_FILES
    glad.h
    glad.c
)

add_library(glad SHARED STATIC ${SOURCE_FILES})
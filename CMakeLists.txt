cmake_minimum_required(VERSION 3.20)
project(StobbartRipper VERSION 1.0)


set(CMAKE_ARCHIVE_OUTPUT_DIRECTORY "${CMAKE_SOURCE_DIR}")
set(CMAKE_LIBRARY_OUTPUT_DIRECTORY "${CMAKE_SOURCE_DIR}")
set(CMAKE_RUNTIME_OUTPUT_DIRECTORY "${CMAKE_SOURCE_DIR}")
  

add_executable(StobbartRipper
     platform.h
     platform.cpp
     StobbartRipper.cpp
)

set_target_properties( StobbartRipper
    PROPERTIES
    CXX_STANDARD             11
    OUTPUT_NAME              StobbartRipper 
)

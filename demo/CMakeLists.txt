cmake_minimum_required(VERSION 3.6.0)
project(fluent-tray-demo VERSION 1.0)

add_compile_options(
  /W4
  /std:c++17
  /MT
  /D_UNICODE
  /DUNICODE
  /utf-8
  /DEBUG
  /DDEBUG
)
include_directories(../include)
add_executable(${PROJECT_NAME} demo.cpp)

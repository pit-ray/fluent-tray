cmake_minimum_required(VERSION 3.0.0)
project(fluent-tray-test VERSION 0.0.1)

enable_testing()

set(CMAKE_BUILD_TYPE Debug)

set(CMAKE_CXX_STANDARD 11)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)
set(CMAKE_EXPORT_COMPILE_COMMANDS ON)

add_compile_options(
    /W4
    /std:c11
    /D_UNICODE
    /DUNICODE
    /DEBUG
    /DDEBUG
    /utf-8
)

include(ProcessorCount)
ProcessorCount(PROC_N)
set(
    CMAKE_MAKE_PROGRAM
    "${CMAKE_MAKE_PROGRAM} -j ${PROC_N}"
)

include_directories(../include)
add_library(doctest STATIC test.cpp)

file(
    GLOB TEST_FILES
    LIST_DIRECTORIES false
    ${CMAKE_CURRENT_SOURCE_DIR}/*test.cpp
)

add_executable(${PROJECT_NAME} ${TEST_FILES})
add_test(
    NAME ${PROJECT_NAME}
    COMMAND $<TARGET_FILE:${PROJECT_NAME}>
    WORKING_DIRECTORY ..
)

set(
    CMAKE_CTEST_ARGUMENTS
    "${CMAKE_CTEST_ARGUMENTS} --verbose --no-label-summary --parallel ${PROC_N}"
)

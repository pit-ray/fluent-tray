cmake_minimum_required(VERSION 3.0.0)
project(fluent-tray-test VERSION 0.0.1)

enable_testing()

set(CMAKE_BUILD_TYPE Debug)

set(CMAKE_CXX_STANDARD 11)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)
set(CMAKE_EXPORT_COMPILE_COMMANDS ON)

set(ROOT_DIR ${PROJECT_SOURCE_DIR}/..)

if(${MSVC})
    add_compile_options(
        /W4
        /std:c11
        /D_UNICODE
        /DUNICODE
        /DEBUG
        /DDEBUG
        /utf-8
    )
    add_link_options(/FORCE:MULTIPLE)
else()
    set(CMAKE_CXX_STANDARD 11)
    set(CMAKE_CXX_STANDARD_REQUIRED ON)
    set(CMAKE_CXX_EXTENSIONS OFF)
    set(CMAKE_EXPORT_COMPILE_COMMANDS ON)

    set(CMAKE_CXX_FLAGS 
        -static
        -std=c++11
        -lstdc++
        -lstdc++fs
        -lgcc
        -Wall
        -Wextra
        -Wcast-align
        -Wno-unknown-pragmas
        -Wcast-qual
        -Wctor-dtor-privacy
        -Wdelete-non-virtual-dtor
        -Wdouble-promotion
        -Weffc++
        -Wold-style-cast
        -Woverloaded-virtual
        -Wreorder
        -Wshadow
        -Wsuggest-override
        -Wuseless-cast
        -fdiagnostics-color
        -DUNICODE
        -D_UNICODE
        -DWINVER=0x0A00
        -D_WIN32_WINNT=0x0A00
        --coverage
        -m64
        -O0
        -g3
        -DDEBUG
    )
    list(REMOVE_DUPLICATES CMAKE_CXX_FLAGS)
    string(REPLACE ";" " " CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS}")

    set(CMAKE_SH "CMAKE_SH-NOTFOUND")
endif()

include(ProcessorCount)
ProcessorCount(PROC_N)
set(
    CMAKE_MAKE_PROGRAM
    "${CMAKE_MAKE_PROGRAM} -j ${PROC_N}"
)

include_directories(../include)
add_library(doctest STATIC test.cpp)

message(STATUS ${ROOT_DIR})
function(AddTest TEST_NAME)
        add_executable(${TEST_NAME} ${ARGN})
    target_link_libraries(${TEST_NAME} doctest)
    add_test(
        NAME ${TEST_NAME}
        COMMAND $<TARGET_FILE:${TEST_NAME}>
        WORKING_DIRECTORY ${ROOT_DIR}
    )
endfunction()

AddTest(string_test string_test.cpp)
AddTest(menu_test menu_test.cpp)
AddTest(tray_test tray_test.cpp)

set(
    CMAKE_CTEST_ARGUMENTS
    "${CMAKE_CTEST_ARGUMENTS} --verbose --no-label-summary --parallel ${PROC_N}"
)

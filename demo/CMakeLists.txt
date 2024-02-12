cmake_minimum_required(VERSION 3.6.0)
project(fluent-tray-demo VERSION 1.0)

if(${MSVC})
    add_compile_options(
      /W4
      /std:c11
      # /MT
      /D_UNICODE
      /DUNICODE
      /utf-8
      /DEBUG
      /DDEBUG
    )
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

include_directories(../include)
add_executable(${PROJECT_NAME} demo.cpp)

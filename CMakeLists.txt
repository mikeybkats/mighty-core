cmake_minimum_required(VERSION 3.22.1)
project(mighty-core VERSION 1.0.0)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

if(ANDROID)
    include(FetchContent)
    FetchContent_Declare(
        oboe
        GIT_REPOSITORY https://github.com/google/oboe.git
        GIT_TAG 1.9.3
        GIT_SHALLOW TRUE
        GIT_PROGRESS TRUE
    )
    FetchContent_MakeAvailable(oboe)
endif()

add_library(mighty-core STATIC
    src/MightyMusicCore.cpp
    src/TempoEngine.cpp
)

target_include_directories(mighty-core
    PUBLIC
        ${CMAKE_CURRENT_SOURCE_DIR}/include
    PRIVATE
        ${CMAKE_CURRENT_SOURCE_DIR}/src
)

if(ANDROID)
    target_link_libraries(mighty-core
        PUBLIC oboe
        PRIVATE log
    )
endif()

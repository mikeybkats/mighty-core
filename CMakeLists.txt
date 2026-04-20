cmake_minimum_required(VERSION 3.14)  # 3.14 = FetchContent_MakeAvailable minimum
project(mighty-core VERSION 1.0.0)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

if(ANDROID)
    # Prefer the Oboe provided by Gradle via prefab (fast, no network hit per ABI).
    # Fall back to FetchContent for standalone NDK builds (e.g. CI, command-line).
    find_package(oboe QUIET CONFIG)
    if(oboe_FOUND)
        message(STATUS "mighty-core: using Oboe from Gradle prefab")
    else()
        message(STATUS "mighty-core: Oboe not found via prefab, fetching from git")
        include(FetchContent)
        FetchContent_Declare(
            oboe
            GIT_REPOSITORY https://github.com/google/oboe.git
            GIT_TAG 1.9.3
            GIT_SHALLOW TRUE
            GIT_PROGRESS TRUE
        )
        FetchContent_MakeAvailable(oboe)
        # FetchContent creates target "oboe"; alias to match the prefab namespaced target.
        add_library(oboe::oboe ALIAS oboe)
    endif()
endif()

add_library(mighty-core STATIC
    src/Scheduler.cpp
    src/MightyMusicCore.cpp
    src/TempoEngine.cpp
)

target_include_directories(mighty-core
    PUBLIC
        ${CMAKE_CURRENT_SOURCE_DIR}/include
    PRIVATE
        ${CMAKE_CURRENT_SOURCE_DIR}/src
        ${CMAKE_CURRENT_SOURCE_DIR}/third_party
)

if(ANDROID)
    target_link_libraries(mighty-core
        PUBLIC oboe::oboe
        PRIVATE log
    )
else()
    # miniaudio needs these system frameworks on macOS
    if(APPLE)
        target_link_libraries(mighty-core
            PRIVATE
                "-framework CoreAudio"
                "-framework AudioToolbox"
                "-framework CoreFoundation"
        )
    endif()

    include(FetchContent)

    # GLFW — windowing for the UI debug host
    FetchContent_Declare(
        glfw
        GIT_REPOSITORY https://github.com/glfw/glfw.git
        GIT_TAG        3.4
        GIT_SHALLOW    TRUE
    )
    set(GLFW_BUILD_DOCS     OFF CACHE BOOL "" FORCE)
    set(GLFW_BUILD_TESTS    OFF CACHE BOOL "" FORCE)
    set(GLFW_BUILD_EXAMPLES OFF CACHE BOOL "" FORCE)

    # Dear ImGui — immediate-mode UI for the debug host
    FetchContent_Declare(
        imgui
        GIT_REPOSITORY https://github.com/ocornut/imgui.git
        GIT_TAG        v1.91.6
        GIT_SHALLOW    TRUE
    )

    # Google Test — unit tests
    FetchContent_Declare(
        googletest
        GIT_REPOSITORY https://github.com/google/googletest.git
        GIT_TAG        v1.15.2
        GIT_SHALLOW    TRUE
    )
    set(gtest_force_shared_crt ON CACHE BOOL "" FORCE)

    FetchContent_MakeAvailable(glfw imgui googletest)

    # Thin static library wrapping ImGui + GLFW + OpenGL3 backends
    find_package(OpenGL REQUIRED)
    add_library(imgui_lib STATIC
        ${imgui_SOURCE_DIR}/imgui.cpp
        ${imgui_SOURCE_DIR}/imgui_draw.cpp
        ${imgui_SOURCE_DIR}/imgui_tables.cpp
        ${imgui_SOURCE_DIR}/imgui_widgets.cpp
        ${imgui_SOURCE_DIR}/backends/imgui_impl_glfw.cpp
        ${imgui_SOURCE_DIR}/backends/imgui_impl_opengl3.cpp
    )
    target_include_directories(imgui_lib PUBLIC
        ${imgui_SOURCE_DIR}
        ${imgui_SOURCE_DIR}/backends
    )
    target_link_libraries(imgui_lib PUBLIC glfw OpenGL::GL)
    if(APPLE)
        target_compile_definitions(imgui_lib PUBLIC GL_SILENCE_DEPRECATION)
        target_link_libraries(imgui_lib PUBLIC
            "-framework Cocoa"
            "-framework IOKit"
            "-framework CoreVideo"
        )
    endif()

    # CLI host harness
    add_executable(mighty-core-host-check host/main.cpp)
    target_link_libraries(mighty-core-host-check PRIVATE mighty-core)

    # GUI debug host
    add_executable(mighty-core-ui host/ui_host.cpp)
    target_link_libraries(mighty-core-ui PRIVATE mighty-core imgui_lib)

    # Unit tests
    enable_testing()
    add_subdirectory(tests)
endif()

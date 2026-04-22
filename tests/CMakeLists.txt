# GoogleTest runner for Scheduler + MightyMusicCore (linked in parent CMakeLists when not ANDROID).
add_executable(mmc-tests
    test_scheduler.cpp
    test_mmc.cpp
)

target_include_directories(mmc-tests PRIVATE
    ${CMAKE_CURRENT_SOURCE_DIR}/../src
)

target_link_libraries(mmc-tests
    PRIVATE
        mighty-core
        GTest::gtest_main
)

include(GoogleTest)
gtest_discover_tests(mmc-tests)

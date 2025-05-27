# This CMakeLists.txt is for the tests directory.

# Find the main game library (WildSparkLib)
# This assumes WildSparkLib is already defined in the parent CMakeLists.txt
# No need to find_package for WildSparkLib if it's a target in the same project.

# Define test sources
set(TEST_SOURCES
    test_scene_manager.cpp
    test_scene_manager_edge_cases.cpp
    # Add other test files here as you create them, e.g.:
    # test_auth_manager.cpp
    # test_input_manager.cpp
    # test_world_map.cpp
    # test_camera.cpp
)

# Create an executable for the tests
add_executable(run_tests ${TEST_SOURCES})

# Add Google Test include directories explicitly
# googletest_SOURCE_DIR is made available by FetchContent_MakeAvailable(googletest) in the parent CMakeLists.txt
target_include_directories(run_tests PRIVATE
    ${googletest_SOURCE_DIR}/googletest/include # For gtest/gtest.h
    ${googletest_SOURCE_DIR}/googlemock/include # For gmock/gmock.h
)

# Link the test executable against Google Test and your game library
# GTest::gmock provides both gmock and gtest. GTest::gtest_main provides main().
target_link_libraries(run_tests 
    PRIVATE 
        WildSparkLib 
        GTest::gmock
        GTest::gtest_main 
)

# Include Google Test's CTest integration
include(GoogleTest)

# Add the test executable to CTest
# This allows 'ctest' to discover and run your tests
gtest_discover_tests(run_tests)

# You might need to include directories for your source code if headers are not found
# target_include_directories(run_tests PRIVATE ../src) # Already handled by WildSparkLib PUBLIC include

# Test sources for WildSpark
set(TEST_SOURCES
    test_scene_manager.cpp
    test_scene_manager_edge_cases.cpp
    test_login_scene.cpp
    test_login_scene_edge_cases.cpp
    test_character_selection_scene.cpp
    test_character_selection_scene_edge_cases.cpp
    test_character_creation_scene.cpp
    test_character_creation_scene_edge_cases.cpp
    test_player.cpp
    test_networking.cpp
    mocks/MockAuthManager.h
    mocks/MockRenderWindow.h
    mocks/MockSceneManager.h
    mocks/MockAccountManager.h
    mocks/MockNClientFull.h
)

add_executable(run_tests ${TEST_SOURCES})

target_include_directories(run_tests PRIVATE
    ${googletest_SOURCE_DIR}/googletest/include
    ${googletest_SOURCE_DIR}/googlemock/include
    ${CMAKE_CURRENT_SOURCE_DIR}
)

target_link_libraries(run_tests PRIVATE WildSparkLib GTest::gmock GTest::gtest_main)

include(GoogleTest)
gtest_discover_tests(run_tests)

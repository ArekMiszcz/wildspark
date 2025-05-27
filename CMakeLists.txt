cmake_minimum_required(VERSION 3.25)
project(WildSpark LANGUAGES CXX)

set(SFML_TAG 3.0.1)
set(IMGUI_TAG v1.91.9)
set(IMGUI_SFML_TAG v3.0)
set(GOOGLE_TEST_TAG v1.17.0)

set(SFML_STATIC_LIBRARIES ON)
set(CMAKE_RUNTIME_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/bin)

include(FetchContent)

FetchContent_Declare(
    SFML
    GIT_REPOSITORY https://github.com/SFML/SFML.git
    GIT_TAG ${SFML_TAG}
    GIT_SHALLOW ON
    EXCLUDE_FROM_ALL
    SYSTEM
)

FetchContent_Declare(
    imgui
    GIT_REPOSITORY https://github.com/ocornut/imgui.git
    GIT_TAG ${IMGUI_TAG}
)

FetchContent_Declare(
  imgui-sfml
  GIT_REPOSITORY https://github.com/SFML/imgui-sfml.git
  GIT_TAG ${IMGUI_SFML_TAG}
)

FetchContent_Declare(
    googletest
    GIT_REPOSITORY https://github.com/google/googletest.git
    GIT_TAG ${GOOGLE_TEST_TAG} # Using the tag you defined
    GIT_SHALLOW TRUE
)

# SFML
FetchContent_MakeAvailable(SFML)

set(THREADS_PREFER_PTHREAD_FLAG ON)
find_package(Threads REQUIRED)
set(PTHREADS_LIB Threads::Threads)

find_package(optional-lite)
find_package(SFML 3 COMPONENTS System Window Graphics Network Audio REQUIRED)
find_package(nakama-sdk CONFIG REQUIRED)

# Dear ImGui
FetchContent_MakeAvailable(imgui)

# ImGui-SFML
set(IMGUI_DIR ${imgui_SOURCE_DIR})
option(IMGUI_SFML_FIND_SFML "Use find_package to find SFML" OFF)
option(IMGUI_SFML_IMGUI_DEMO "Build imgui_demo.cpp" ON)
FetchContent_MakeAvailable(imgui-sfml)

# Google Test
FetchContent_MakeAvailable(googletest)

# Define sources for the game library (excluding main.cpp)
set(LIB_SOURCES
    src/auth/AuthManager.cpp
    src/auth/clients/NakamaClient.cpp
    src/account/AccountManager.cpp
    src/scenes/SceneManager.cpp
    src/scenes/LoginScene/LoginScene.cpp
    src/scenes/CharacterScene/CharacterSelectionScene.cpp
    src/scenes/CharacterScene/CharacterCreationScene.cpp
    src/scenes/GameScene/GameScene.cpp
    src/world/WorldMap.cpp
    src/world/WorldRenderer.cpp
    src/graphics/Camera.cpp
    src/input/InputManager.cpp
)

# Create a static library for the game logic
add_library(WildSparkLib STATIC ${LIB_SOURCES})

# Set properties for WildSparkLib and link its dependencies
target_compile_features(WildSparkLib PUBLIC cxx_std_17) # PUBLIC so consumers inherit C++17 requirement
target_include_directories(WildSparkLib 
    PUBLIC 
        ${CMAKE_CURRENT_SOURCE_DIR}/src # Allows consumers to #include "header.h" from src
)
target_link_libraries(WildSparkLib 
    PUBLIC # PUBLIC because headers of WildSparkLib might include headers from these dependencies
        SFML::Graphics SFML::Window SFML::System SFML::Network SFML::Audio 
        nakama-sdk 
        ImGui-SFML::ImGui-SFML
)
set_property(TARGET WildSparkLib PROPERTY CXX_STANDARD 17)
set_property(TARGET WildSparkLib PROPERTY CXX_STANDARD_REQUIRED ON)
target_compile_definitions(WildSparkLib PRIVATE IMGUI_DISABLE_OBSOLETE_FUNCTIONS) # PRIVATE if only .cpp files use ImGui directly

# Main game executable
add_executable(wildspark_game src/main.cpp)
target_link_libraries(wildspark_game PRIVATE WildSparkLib)

# Set properties for the main executable
target_compile_features(wildspark_game PRIVATE cxx_std_17)
set_property(TARGET wildspark_game PROPERTY CXX_STANDARD 17)
set_property(TARGET wildspark_game PROPERTY CXX_STANDARD_REQUIRED ON)
# If src/main.cpp itself uses ImGui and needs IMGUI_DISABLE_OBSOLETE_FUNCTIONS, add it here.
# It appears src/main.cpp does not use ImGui directly.

# Enable testing and add tests subdirectory (should be at the end or after targets are defined)
enable_testing()
add_subdirectory(tests)

# Install step (optional) - update target name if used
# install(TARGETS wildspark_game DESTINATION bin)

# For debugging, print out some variables (can be removed for cleaner output)
message(STATUS "SFML_INCLUDE_DIR: ${SFML_INCLUDE_DIR}")
message(STATUS "SFML_LIBRARIES: ${SFML_LIBRARIES}") # Note: SFML_LIBRARIES might not be set when using modern targets
message(STATUS "NakamaCore_INCLUDE_DIRS: ${NakamaCore_INCLUDE_DIRS}") # Check if Nakama SDK provides NakamaCore_INCLUDE_DIRS or similar
message(STATUS "NakamaCore_LIBRARIES: ${NakamaCore_LIBRARIES}")
message(STATUS "IMGUI_INCLUDE_DIRS: ${IMGUI_INCLUDE_DIRS}") # From FetchContent imgui
message(STATUS "IMGUI_SFML_INCLUDE_DIRS: ${IMGUI_SFML_INCLUDE_DIRS}") # From FetchContent imgui-sfml
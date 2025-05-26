cmake_minimum_required(VERSION 3.25)
project(WildSpark LANGUAGES CXX)

set(SFML_TAG 3.0.1)
set(IMGUI_TAG v1.91.9)
set(IMGUI_SFML_TAG v3.0)

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

set(GAME_SOURCES
    src/main.cpp
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

add_executable(main ${GAME_SOURCES})
target_compile_features(main PRIVATE cxx_std_17)
target_link_libraries(main PRIVATE SFML::Graphics SFML::Window SFML::System SFML::Network SFML::Audio nakama-sdk ImGui-SFML::ImGui-SFML)

# Install step (optional)
# install(TARGETS main DESTINATION bin) # Corrected target name

# Enable C++17
set_property(TARGET main PROPERTY CXX_STANDARD 17) # Corrected target name
set_property(TARGET main PROPERTY CXX_STANDARD_REQUIRED ON) # Corrected target name

# For ImGui-SFML
target_compile_definitions(main PRIVATE IMGUI_DISABLE_OBSOLETE_FUNCTIONS) # Corrected target name

# If you have a resources directory for fonts, images, etc.
# file(COPY resources DESTINATION ${CMAKE_BINARY_DIR})

# If SFML is installed in a custom location, you might need to set SFML_DIR
# set(SFML_DIR /path/to/your/sfml/lib/cmake/SFML)

# Ensure Nakama include directories are available
# target_include_directories(main PRIVATE ${NakamaCore_INCLUDE_DIRS}) # Corrected target name, Nakama SDK should handle this via target_link_libraries

# Ensure ImGui include directories are available
# target_include_directories(main PRIVATE ${IMGUI_INCLUDE_DIRS} ${IMGUI_SFML_INCLUDE_DIRS}) # Corrected target name, ImGui-SFML should handle this

# Ensure SFML include directories are available (from FetchContent or find_package)
# target_include_directories(main PRIVATE ${SFML_INCLUDE_DIR}) # Corrected target name, SFML::Graphics etc. should handle this

# If you need to link against specific Nakama libraries (like NakamaClient, NakamaRealtimeClient)
# target_link_libraries(wildspark PRIVATE NakamaClient NakamaRealtimeClient)

# For Nakama, you might need to link against its dependencies if not handled by NakamaCore target
# For example, if NakamaCore doesn't export its dependencies like Boost or OpenSSL

# For debugging, print out some variables
message(STATUS "SFML_INCLUDE_DIR: ${SFML_INCLUDE_DIR}")
message(STATUS "SFML_LIBRARY_DIR: ${SFML_LIBRARY_DIR}") # This might not be set by FetchContent directly
message(STATUS "SFML_LIBRARIES: ${SFML_LIBRARIES}") # This might not be set by FetchContent directly
message(STATUS "NakamaCore_INCLUDE_DIRS: ${NakamaCore_INCLUDE_DIRS}")
message(STATUS "NakamaCore_LIBRARIES: ${NakamaCore_LIBRARIES}")
message(STATUS "IMGUI_INCLUDE_DIRS: ${IMGUI_INCLUDE_DIRS}")
message(STATUS "IMGUI_SFML_INCLUDE_DIRS: ${IMGUI_SFML_INCLUDE_DIRS}")
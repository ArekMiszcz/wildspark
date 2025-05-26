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
FetchContent_MakeAvailable(sfml)

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
    src/scenes/SceneManager.cpp
    src/scenes/LoginScene/LoginScene.cpp
    src/scenes/CharacterScene/CharacterSelectionScene.cpp
    src/scenes/CharacterScene/CharacterCreationScene.cpp
)

add_executable(main ${GAME_SOURCES})
target_compile_features(main PRIVATE cxx_std_17)
target_link_libraries(main PRIVATE SFML::Graphics SFML::Window SFML::System SFML::Network SFML::Audio nakama-sdk ImGui-SFML::ImGui-SFML)
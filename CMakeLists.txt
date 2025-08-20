cmake_minimum_required(VERSION 3.25)
project(WildSpark LANGUAGES CXX)

# ---- Global toolchain/setup ---------------------------------------------------
set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

set(CMAKE_RUNTIME_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/bin)

# Versions/tags (only where FetchContent is used)
set(IMGUI_TAG v1.91.9)
set(IMGUI_SFML_TAG v3.0)
set(GOOGLE_TEST_TAG v1.17.0)
set(NLOHMANN_JSON_TAG v3.12.0)
set(OPTIONAL_LITE_TAG v3.6.0)

include(FetchContent)

# ---- Dependencies -------------------------------------------------------------

# SFML 3 (use installed package; no duplicate FetchContent)
set(SFML_STATIC_LIBRARIES ON)
find_package(SFML 3 REQUIRED COMPONENTS System Window Graphics)

# nlohmann_json: prefer installed; otherwise fetch header-only
find_package(nlohmann_json QUIET CONFIG)
if (NOT nlohmann_json_FOUND)
  message(STATUS "nlohmann_json not found, fetching...")
  FetchContent_Declare(nlohmann_json
    GIT_REPOSITORY https://github.com/nlohmann/json.git
    GIT_TAG ${NLOHMANN_JSON_TAG}
  )
  FetchContent_MakeAvailable(nlohmann_json)
endif()

# Nakama
find_package(nakama-sdk CONFIG REQUIRED)

# Provide optional-lite target ONLY if it doesn't already exist.
if (NOT TARGET nonstd::optional-lite)
  find_package(optional-lite CONFIG QUIET)
  if (TARGET optional-lite::optional-lite AND NOT TARGET nonstd::optional-lite)
    add_library(nonstd::optional-lite ALIAS optional-lite::optional-lite)
  elseif (NOT TARGET nonstd::optional-lite)
    include(FetchContent)
    FetchContent_Declare(optional_lite
      GIT_REPOSITORY https://github.com/martinmoene/optional-lite.git
      GIT_TAG ${OPTIONAL_LITE_TAG}
      GIT_SHALLOW TRUE
    )
    FetchContent_MakeAvailable(optional_lite)

    add_library(nonstd_optional_lite INTERFACE)
    target_include_directories(nonstd_optional_lite INTERFACE
      ${optional_lite_SOURCE_DIR}/include
    )
    add_library(nonstd::optional-lite ALIAS nonstd_optional_lite)
  endif()
endif()

# Dear ImGui (Fetch)
FetchContent_Declare(
  imgui
  GIT_REPOSITORY https://github.com/ocornut/imgui.git
  GIT_TAG ${IMGUI_TAG}
)
FetchContent_MakeAvailable(imgui)

# ImGui-SFML (Fetch)
set(IMGUI_DIR ${imgui_SOURCE_DIR})
set(IMGUI_SFML_FIND_SFML OFF CACHE BOOL "" FORCE)
set(IMGUI_SFML_USE_SFML_PACKAGE OFF CACHE BOOL "" FORCE)
option(IMGUI_SFML_IMGUI_DEMO "Build imgui_demo.cpp" ON)
FetchContent_Declare(
  imgui-sfml
  GIT_REPOSITORY https://github.com/SFML/imgui-sfml.git
  GIT_TAG ${IMGUI_SFML_TAG}
)
FetchContent_MakeAvailable(imgui-sfml)

# Google Test (Fetch)
FetchContent_Declare(
  googletest
  GIT_REPOSITORY https://github.com/google/googletest.git
  GIT_TAG ${GOOGLE_TEST_TAG}
  GIT_SHALLOW TRUE
)
FetchContent_MakeAvailable(googletest)

# ---- Library target -----------------------------------------------------------

set(LIB_SOURCES
  src/auth/AuthManager.cpp
  src/auth/clients/NakamaClient.cpp
  src/account/AccountManager.cpp
  src/networking/Networking.cpp
  src/scenes/SceneManager.cpp
  src/scenes/LoginScene/LoginScene.cpp
  src/scenes/CharacterScene/CharacterSelectionScene.cpp
  src/scenes/CharacterScene/CharacterCreationScene.cpp
  src/scenes/GameScene/GameScene.cpp
  src/world/WorldMap.cpp
  src/world/WorldRenderer.cpp
  src/world/entities/Player.cpp
  src/graphics/Camera.cpp
  src/input/InputManager.cpp
)

add_library(WildSparkLib STATIC ${LIB_SOURCES})

target_include_directories(WildSparkLib
  PUBLIC
    ${CMAKE_CURRENT_SOURCE_DIR}/src
)

target_link_libraries(WildSparkLib
  PUBLIC
    SFML::Graphics SFML::Window SFML::System
    nakama-sdk
    ImGui-SFML::ImGui-SFML
    nlohmann_json::nlohmann_json
)

# ---- Executable ---------------------------------------------------------------

add_executable(wildspark_game src/main.cpp)
target_link_libraries(wildspark_game PRIVATE WildSparkLib)

# ---- Testing ------------------------------------------------------------------

enable_testing()
add_subdirectory(tests)

# ---- cpplint (optional) -------------------------------------------------------
option(ENABLE_CPPLINT "Enable cpplint linting target" ON)
if (ENABLE_CPPLINT)
  find_package(Python3 COMPONENTS Interpreter)
  if (Python3_Interpreter_FOUND)
    file(GLOB_RECURSE PROJECT_HEADERS CONFIGURE_DEPENDS ${CMAKE_CURRENT_SOURCE_DIR}/src/*.h)
    file(GLOB_RECURSE TEST_HEADERS    CONFIGURE_DEPENDS ${CMAKE_CURRENT_SOURCE_DIR}/tests/*.h)
    file(GLOB_RECURSE TEST_SOURCES    CONFIGURE_DEPENDS ${CMAKE_CURRENT_SOURCE_DIR}/tests/*.cpp)

    set(CPPLINT_FILES ${LIB_SOURCES} ${PROJECT_HEADERS} ${TEST_SOURCES} ${TEST_HEADERS})
    list(REMOVE_DUPLICATES CPPLINT_FILES)

    add_custom_target(cpplint
      COMMAND ${Python3_EXECUTABLE} -m cpplint --linelength=120 ${CPPLINT_FILES}
      WORKING_DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR}
      COMMENT "Running cpplint on project sources and tests"
      VERBATIM
    )

    # If you want the build to fail on lint, uncomment:
    add_dependencies(WildSparkLib cpplint)
    add_dependencies(wildspark_game cpplint)
  else()
    message(WARNING "Python3 interpreter not found; cpplint target disabled.")
  endif()
endif()
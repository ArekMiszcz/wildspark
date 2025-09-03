# WildSpark Game Client

WildSpark is a 2D game client built with SFML and modern C++. This project implements a scene-based architecture with authentication, character management, and a tile-based world system.

## Features

- **Scene Management**: Flexible scene-based architecture for different game states
- **Authentication System**: User login and session management using Nakama backend
- **Character Management**: Character selection and creation interfaces
- **Tile-Based World**: 2D world map with different tile types and rendering
- **Input System**: Robust input handling for keyboard and mouse interactions
- **Camera System**: Smooth camera controls with zooming and panning

## Project Structure

- **src/**: Main source code directory
  - **account/**: Account management functionality
  - **auth/**: Authentication system with Nakama integration
  - **graphics/**: Camera and rendering utilities
  - **input/**: Input management system
  - **scenes/**: Scene management and individual game scenes
    - **LoginScene/**: User authentication interface
    - **CharacterScene/**: Character selection and creation
    - **GameScene/**: Main gameplay scene
  - **world/**: World map, tile system, and rendering

## Dependencies

- **SFML 3.0.1**: Graphics, window management, and input handling
- **Dear ImGui 1.91.9**: User interface components
- **ImGui-SFML v3.0**: ImGui integration with SFML
- **Nakama SDK**: Backend authentication and multiplayer services
- **GoogleTest 1.17.0**: Testing framework

## Building and running

The project uses CMake for build configuration. From the repository root:

```bash
# Create a build directory and configure
mkdir -p build && cd build
cmake .. -DCMAKE_BUILD_TYPE=Debug

# Build the project (and tests)
cmake --build . --config Debug -j

# Run the game
./bin/wildspark_game
```

Notes:
- If you use a different generator or toolchain, adjust the `cmake` invocation accordingly.
- For release builds use `-DCMAKE_BUILD_TYPE=Release`.

Environment:
- The game loads map files using a MAPS_DIR environment variable read via dotenv. Create a `.env` file in the project root with a line like:

```text
MAPS_DIR=/absolute/path/to/maps/
```

This lets the code locate map JSON and tileset assets when running locally.

## Game Flow

1. **Login Scene**: User authentication via email and password
2. **Character Selection**: Choose an existing character or create a new one
3. **Character Creation**: Create and customize a new character
4. **Game Scene**: Main gameplay with world exploration and interaction

## Input Controls

The game uses a flexible input mapping system that allows binding actions to keyboard keys and mouse buttons:

- **WASD/Arrow Keys**: Camera movement
- **Mouse Wheel**: Zoom in/out
- **Mouse Drag**: Pan camera (when implemented)

## Development

### Scene System

The game uses a scene-based architecture where each scene represents a different state or screen in the game:

```cpp
// Adding scenes to the scene manager
sceneManager.addScene(SceneType::Login, std::make_unique<LoginScene>(window, authManager));
sceneManager.addScene(SceneType::CharacterSelection, std::make_unique<CharacterSelectionScene>(window, authManager));
sceneManager.addScene(SceneType::CharacterCreation, std::make_unique<CharacterCreationScene>(window, authManager));
sceneManager.addScene(SceneType::Game, std::make_unique<GameScene>(window, authManager, inputManager));
```

### World System

The game world is represented by a tile-based map system:

```cpp
// World map with different tile types
enum class TileType {
    Grass,
    Water,
    Wall,
    Dirt,
    Sand,
    Void // Represents an empty or non-existent tile
};
```

## Testing

Unit tests use Google Test and are built as part of the normal CMake build. From the project root:

```bash
cd build
# Run the test target (build first if needed)
cmake --build . --target run_tests -j

# Run the test binary directly
./bin/run_tests

# Or use ctest to run discovered tests with output on failure
ctest --output-on-failure
```

You can filter tests using Google Test flags, for example:

```bash
./bin/run_tests --gtest_filter=WorldMapUpdateObject.*
```

## Future Development

- Multiplayer functionality using Nakama real-time client
- Enhanced world generation and map editing
- Character progression and inventory systems
- Combat and interaction mechanics
- Quest and mission system

## License

See the `LICENSE` file in the project root for license details.
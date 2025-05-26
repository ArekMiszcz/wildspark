#include "WorldMap.h"
#include <stdexcept> // For std::out_of_range

WorldMap::WorldMap(int width, int height) : mapWidth(width), mapHeight(height) {
    if (width <= 0 || height <= 0) {
        throw std::invalid_argument("Map dimensions must be positive.");
    }
    initializeMap();
}

void WorldMap::initializeMap() {
    tiles.resize(mapHeight);
    for (int y = 0; y < mapHeight; ++y) {
        tiles[y].resize(mapWidth);
        for (int x = 0; x < mapWidth; ++x) {
            // Default to Grass tiles for now
            if ((x + y) % 5 == 0) { // Add some variety for testing
                 tiles[y][x] = Tile(TileType::Water, false, sf::Color::Blue);
            } else if ((x % 7 == 0 && y % 3 == 0)) {
                 tiles[y][x] = Tile(TileType::Sand, true, sf::Color::Yellow);
            }
            else {
                 tiles[y][x] = Tile(TileType::Grass, true, sf::Color::Green);
            }
        }
    }
}

const Tile& WorldMap::getTile(int x, int y) const {
    if (x < 0 || x >= mapWidth || y < 0 || y >= mapHeight) {
        // Consider returning a special 'Void' tile or throwing an error
        // For now, throwing an error is safer to catch out-of-bounds access.
        throw std::out_of_range("Tile coordinates out of bounds.");
    }
    return tiles[y][x];
}

Tile& WorldMap::getTile(int x, int y) {
    if (x < 0 || x >= mapWidth || y < 0 || y >= mapHeight) {
        throw std::out_of_range("Tile coordinates out of bounds.");
    }
    return tiles[y][x];
}

void WorldMap::setTile(int x, int y, const Tile& tile) {
    if (x < 0 || x >= mapWidth || y < 0 || y >= mapHeight) {
        // Optionally throw or log an error
        return; 
    }
    tiles[y][x] = tile;
}

int WorldMap::getWidth() const {
    return mapWidth;
}

int WorldMap::getHeight() const {
    return mapHeight;
}

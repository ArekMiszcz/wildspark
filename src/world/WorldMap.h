#pragma once

#include <vector>
#include "Tile.h" // Make sure Tile.h is included

class WorldMap {
public:
    WorldMap(int width, int height);

    // Get the tile at a specific coordinate
    const Tile& getTile(int x, int y) const;
    Tile& getTile(int x, int y); // Non-const version to modify tiles

    // Set the tile at a specific coordinate
    void setTile(int x, int y, const Tile& tile);

    int getWidth() const;
    int getHeight() const;

private:
    int mapWidth;
    int mapHeight;
    std::vector<std::vector<Tile>> tiles;

    void initializeMap(); // Helper to fill the map with default tiles
};

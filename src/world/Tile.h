#pragma once

#include <SFML/Graphics.hpp> // For sf::Color or sf::IntRect if using a spritesheet

enum class TileType {
    Grass,
    Water,
    Wall,
    Dirt,
    Sand,
    Void // Represents an empty or non-existent tile
};

struct Tile {
    TileType type;
    bool isWalkable;
    // For now, we can use a color for simple rendering. Later, this could be a texture ID or IntRect.
    sf::Color debugColor; 
    // int textureId; // Example for later texture atlas use

    Tile(TileType t = TileType::Void, bool walkable = false, sf::Color color = sf::Color::Black)
        : type(t), isWalkable(walkable), debugColor(color) {}
};

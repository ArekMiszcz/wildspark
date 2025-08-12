// Copyright 2025 WildSpark Authors

#pragma once

#include <SFML/Graphics.hpp>

enum class TileType {
  Grass,
  Water,
  Wall,
  Dirt,
  Sand,
  Void  // Represents an empty or non-existent tile
};

struct Tile {
  TileType type;
  bool isWalkable;
  sf::Color debugColor;  // For now, we can use a color for simple rendering. Later, could be a texture ID.
  explicit Tile(TileType t = TileType::Void, bool walkable = false, sf::Color color = sf::Color::Black)
      : type(t), isWalkable(walkable), debugColor(color) {}
};

// Copyright 2025 WildSpark Authors

#ifndef WORLD_WORLDRENDERER_H_
#define WORLD_WORLDRENDERER_H_

#include <SFML/Graphics/RenderTarget.hpp>

#include "WorldMap.h"  // Assuming WorldMap is in the same directory or include paths are set

class WorldRenderer {
 public:
  static const int TILE_WIDTH_PX =
      32;  // Example for square tiles or base width for isometric
  static const int TILE_HEIGHT_PX =
      32;  // Example for square tiles, or half height for common isometric

  explicit WorldRenderer(
      WorldMap& worldMap);  // Takes a reference to the map it will render

  void render(sf::RenderTarget& target);

 private:
  WorldMap& m_worldMapRef;
};

#endif  // WORLD_WORLDRENDERER_H_

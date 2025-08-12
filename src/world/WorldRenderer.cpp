// Copyright 2025 WildSpark Authors

#include "WorldRenderer.h"

#include <SFML/Graphics/ConvexShape.hpp>
#include <SFML/Graphics/RectangleShape.hpp>

WorldRenderer::WorldRenderer(WorldMap& worldMap) : m_worldMapRef(worldMap) {}

void WorldRenderer::render(sf::RenderTarget& target) {
  // For now, simple grid rendering. Isometric projection will be added here.
  for (int y = 0; y < m_worldMapRef.getHeight(); ++y) {
    for (int x = 0; x < m_worldMapRef.getWidth(); ++x) {
      const Tile& tile = m_worldMapRef.getTile(x, y);
      float isoX = (static_cast<float>(x) - static_cast<float>(y)) *
                   (TILE_WIDTH_PX / 2.0f);
      float isoY = (static_cast<float>(x) + static_cast<float>(y)) *
                   (TILE_HEIGHT_PX / 2.0f);

      sf::ConvexShape isoTile;
      isoTile.setPointCount(4);
      isoTile.setPoint(0, sf::Vector2f(isoX, isoY + TILE_HEIGHT_PX / 2.0f));
      isoTile.setPoint(1, sf::Vector2f(isoX + TILE_WIDTH_PX / 2.0f, isoY));
      isoTile.setPoint(2, sf::Vector2f(isoX, isoY - TILE_HEIGHT_PX / 2.0f));
      isoTile.setPoint(3, sf::Vector2f(isoX - TILE_WIDTH_PX / 2.0f, isoY));
      isoTile.setFillColor(tile.debugColor);
      isoTile.setOutlineColor(sf::Color(50, 50, 50));
      isoTile.setOutlineThickness(1.f);
      target.draw(isoTile);
    }
  }
}

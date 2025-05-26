\
#include "WorldRenderer.h"
#include <SFML/Graphics/RectangleShape.hpp>
#include <SFML/Graphics/ConvexShape.hpp>

WorldRenderer::WorldRenderer(WorldMap& worldMap)
    : m_worldMapRef(worldMap) {
    // Initialization, if any, e.g., loading tile textures
}

void WorldRenderer::render(sf::RenderTarget& target) {
    // For now, simple grid rendering. Isometric projection will be added here.
    for (int y = 0; y < m_worldMapRef.getHeight(); ++y) {
        for (int x = 0; x < m_worldMapRef.getWidth(); ++x) {
            const Tile& tile = m_worldMapRef.getTile(x, y);
            
            // Isometric projection calculations
            // Assuming TILE_WIDTH_PX is the width of the diamond, and TILE_HEIGHT_PX is the height of the diamond
            // The actual tile texture might be, for example, 64x32 for a 2:1 isometric style
            float isoX = (static_cast<float>(x) - static_cast<float>(y)) * (TILE_WIDTH_PX / 2.0f);
            float isoY = (static_cast<float>(x) + static_cast<float>(y)) * (TILE_HEIGHT_PX / 2.0f); // For 2:1 ratio, this would be TILE_HEIGHT_PX / 4.0f if TILE_HEIGHT_PX is the full texture height

            // Adjust for a common isometric style where TILE_HEIGHT_PX is half of TILE_WIDTH_PX for the diamond
            // For example, if TILE_WIDTH_PX = 64, TILE_HEIGHT_PX = 32 (for the diamond shape)
            // float isoY = (static_cast<float>(x) + static_cast<float>(y)) * (TILE_HEIGHT_PX / 2.0f); 

            // To center the map or provide an origin offset:
            // float originX = target.getSize().x / 2.0f; // Example: center of the screen
            // float originY = 50.0f; // Example: some offset from the top
            // isoX += originX;
            // isoY += originY;

            sf::ConvexShape isoTile;
            isoTile.setPointCount(4);
            isoTile.setPoint(0, sf::Vector2f(isoX, isoY + TILE_HEIGHT_PX / 2.0f)); // Bottom point
            isoTile.setPoint(1, sf::Vector2f(isoX + TILE_WIDTH_PX / 2.0f, isoY)); // Right point
            isoTile.setPoint(2, sf::Vector2f(isoX, isoY - TILE_HEIGHT_PX / 2.0f)); // Top point
            isoTile.setPoint(3, sf::Vector2f(isoX - TILE_WIDTH_PX / 2.0f, isoY)); // Left point

            isoTile.setFillColor(tile.debugColor);
            isoTile.setOutlineColor(sf::Color(50, 50, 50));
            isoTile.setOutlineThickness(1.f);
            target.draw(isoTile);
        }
    }
}

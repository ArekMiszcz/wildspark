\
#ifndef WORLD_RENDERER_H
#define WORLD_RENDERER_H

#include <SFML/Graphics/RenderTarget.hpp>
#include "WorldMap.h" // Assuming WorldMap is in the same directory or include paths are set

// Forward declaration if needed, but direct include is fine if they are tightly coupled
// class WorldMap; 

class WorldRenderer {
public:
    // Tile dimensions - these might become configurable later
    static const int TILE_WIDTH_PX = 32;  // Example for square tiles or base width for isometric
    static const int TILE_HEIGHT_PX = 32; // Example for square tiles, or half height for common isometric

    WorldRenderer(WorldMap& worldMap); // Takes a reference to the map it will render

    void render(sf::RenderTarget& target);

private:
    WorldMap& m_worldMapRef;
    // Potentially add camera reference or view parameters here later
};

#endif // WORLD_RENDERER_H

// Copyright 2025 WildSpark Authors
#ifndef WORLD_WORLDRENDERER_H_
#define WORLD_WORLDRENDERER_H_

#include <string>
#include <string_view>
#include <unordered_map>
#include <vector>

#include "WorldMap.h"

#include <SFML/Graphics.hpp>

// WorldRenderer can draw the whole map, or split draw into:
//  - ground layers
//  - overlay/occluder layers
// This allows a scene to render: ground -> actors (player, NPCs) -> overlays
class WorldRenderer : public sf::Drawable, public sf::Transformable {
 public:
  explicit WorldRenderer(const WorldMap& map) : map_(map) {}

  void setCulling(bool enabled) { cull_ = enabled; }
  void setDebugGrid(bool enabled) { debugGrid_ = enabled; }
  void setDebugGridColor(const sf::Color& c) { debugGridColor_ = c; }
  void setDebugObjectAreas(bool enabled) { debugObjectAreas_ = enabled; }
  void setDebugObjectAreasColor(const sf::Color& c) { debugObjectAreasColor_ = c; }

  // convenience: legacy "draw everything" (no actor interleave)
  void render(sf::RenderTarget& target) const { target.draw(*this); }
  void render(sf::RenderTarget& target, sf::RenderStates states) const {
    target.draw(*this, states);
  }

  // New split rendering for actor interleave.
  void renderGround(sf::RenderTarget& target) const;
  void renderGround(sf::RenderTarget& target, sf::RenderStates states) const;

  void renderOverlays(sf::RenderTarget& target) const;
  void renderOverlays(sf::RenderTarget& target, sf::RenderStates states) const;

  // Compute bounds for a vertex array (cached). Made public so callers
  // like WorldMap::getObjectIdAtPosition can reuse the same logic.
  sf::FloatRect boundsFor(const sf::VertexArray& va) const;

  // Invalidate internal caches (bounds cache and optionally object draw orders).
  // If rebuildObjectDrawOrder is true, the renderer will rebuild object_draw_order
  // arrays for object layers by re-scanning map layers. This is useful after
  // runtime mutations to map chunks.
  void invalidateCache(bool rebuildObjectDrawOrder = false);

  // Invalidate cache and rebuild object draw order only for the specified
  // layer indices. This avoids re-scanning unaffected layers.
  void invalidateCache(const std::vector<int>& affectedLayers);

 private:
  void draw(sf::RenderTarget& target, sf::RenderStates states) const override;

  // Layer name helpers
  static std::string toLower(std::string s);
  static bool isGroundName(std::string_view n);
  static bool isOverlayName(std::string_view n);
  static bool isObjectLayerName(std::string_view n);

  // Drawing helpers
  void drawLayerMesh(sf::RenderTarget& target, sf::RenderStates states,
                     const WorldMap::LayerMesh& layer) const;

  void drawDebugGrid(sf::RenderTarget& target, sf::RenderStates states,
                     const sf::FloatRect& visibleWorld) const;
  void drawDebugObjectAreas(sf::RenderTarget& target, sf::RenderStates states,
                          const WorldMap::LayerMesh& layer) const;

  // cached bounds for vertex arrays
  struct BoundsKey {
    const void* ptr;
    size_t count;
    bool operator==(const BoundsKey& o) const noexcept {
      return ptr == o.ptr && count == o.count;
    }
  };
  struct BoundsKeyHash {
    size_t operator()(const BoundsKey& k) const noexcept {
      return std::hash<const void*>()(k.ptr) ^
             (std::hash<size_t>()(k.count) << 1);
    }
  };

  const WorldMap& map_;
  bool cull_ = true;
  bool debugGrid_ = false;
  bool debugObjectAreas_ = true;
  sf::Color debugGridColor_ = sf::Color::Red;
  sf::Color debugObjectAreasColor_ = sf::Color(0, 255, 255, 128);  // Cyan with transparency
  mutable std::unordered_map<BoundsKey, sf::FloatRect, BoundsKeyHash> cache_;
};

#endif  // WORLD_WORLDRENDERER_H_

// Copyright 2025 WildSpark Authors

#ifndef WORLD_WORLDRENDERER_H_
#define WORLD_WORLDRENDERER_H_

#include <unordered_map>

#include "WorldMap.h"

#include <SFML/Graphics.hpp>

class WorldRenderer : public sf::Drawable, public sf::Transformable {
 public:
  explicit WorldRenderer(const WorldMap& map) : map_(map) {}
  void setCulling(bool enabled) { cull_ = enabled; }
  void invalidateCache() { cache_.clear(); }

  void setDebugGrid(bool enabled) { debugGrid_ = enabled; }
  void setDebugGridColor(const sf::Color& c) { debugGridColor_ = c; }

  void render(sf::RenderTarget& target) const {
    target.draw(*this);
  }

 private:
  struct BoundsKey {
    const sf::VertexArray* ptr{};
    size_t count{};
    bool operator==(const BoundsKey& o) const {
      return ptr == o.ptr && count == o.count;
    }
  };

  struct BoundsKeyHash {
    std::size_t operator()(const BoundsKey& k) const noexcept {
      return std::hash<const void*>()(k.ptr) ^
             (std::hash<size_t>()(k.count) << 1);
    }
  };

  sf::FloatRect boundsFor(const sf::VertexArray& va) const;

  void draw(sf::RenderTarget& target, sf::RenderStates states) const override;

  void drawDebugGrid(sf::RenderTarget& target, sf::RenderStates states,
                     const sf::FloatRect& localViewRect) const;

  const WorldMap& map_;
  bool cull_ = true;
  bool debugGrid_ = false;
  sf::Color debugGridColor_ = sf::Color::Red;

  mutable std::unordered_map<BoundsKey, sf::FloatRect, BoundsKeyHash> cache_;
};

#endif  // WORLD_WORLDRENDERER_H_

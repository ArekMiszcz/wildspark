// Copyright 2025 WildSpark Authors

#include "WorldRenderer.h"

#include <algorithm>
#include <limits>
#include <string>

#include <SFML/Graphics/ConvexShape.hpp>
#include <SFML/Graphics/RectangleShape.hpp>

using LM = WorldMap::LayerMesh;

std::string WorldRenderer::toLower(std::string s) {
  for (char& c : s) c = static_cast<char>(std::tolower(c));
  return s;
}

bool WorldRenderer::isGroundName(std::string_view name) {
  std::string n(name);
  n = toLower(n);
  return n.find("world") != std::string::npos ||
         n.find("decals") != std::string::npos ||
         n.find("level_0_0") != std::string::npos;
}

bool WorldRenderer::isOverlayName(std::string_view name) {
  std::string n(name);
  n = toLower(n);
  // Common occluder-ish names from Tiled projects
  return n.find("level_0_1") != std::string::npos ||
         n.find("level_1_0") != std::string::npos ||
         n.find("level_1_1") != std::string::npos;
}

sf::FloatRect WorldRenderer::boundsFor(const sf::VertexArray& va) const {
  BoundsKey key{&va, va.getVertexCount()};
  if (auto it = cache_.find(key); it != cache_.end()) {
    return it->second;
  }
  float minX = std::numeric_limits<float>::max();
  float minY = std::numeric_limits<float>::max();
  float maxX = std::numeric_limits<float>::lowest();
  float maxY = std::numeric_limits<float>::lowest();
  const size_t n = va.getVertexCount();
  for (size_t i = 0; i < n; ++i) {
    const sf::Vector2f p = va[i].position;
    if (p.x < minX) minX = p.x;
    if (p.y < minY) minY = p.y;
    if (p.x > maxX) maxX = p.x;
    if (p.y > maxY) maxY = p.y;
  }
  sf::FloatRect r({minX, minY}, {maxX - minX, maxY - minY});
  cache_.emplace(key, r);
  return r;
}

void WorldRenderer::drawLayerMesh(sf::RenderTarget& target,
                                  sf::RenderStates states,
                                  const LM& layer) const {
  if (!layer.visible || layer.opacity <= 0.f) return;

  sf::RenderStates s = states;
  s.transform *= getTransform();

  if (cull_) {
    const auto& v = target.getView();
    const sf::Vector2f c = v.getCenter();
    const sf::Vector2f sz = v.getSize();
    const sf::FloatRect worldView({c.x - sz.x * 0.5f, c.y - sz.y * 0.5f},
                                  {sz.x, sz.y});
    const sf::Transform inv = s.transform.getInverse();
    const sf::Vector2f pos = worldView.position, size = worldView.size;
    const sf::Vector2f tl = inv.transformPoint(pos);
    const sf::Vector2f tr = inv.transformPoint({pos.x + size.x, pos.y});
    const sf::Vector2f br =
        inv.transformPoint({pos.x + size.x, pos.y + size.y});
    const sf::Vector2f bl = inv.transformPoint({pos.x, pos.y + size.y});
    const float minX = std::min(std::min(tl.x, tr.x), std::min(br.x, bl.x));
    const float minY = std::min(std::min(tl.y, tr.y), std::min(br.y, bl.y));
    const float maxX = std::max(std::max(tl.x, tr.x), std::max(br.x, bl.x));
    const float maxY = std::max(std::max(tl.y, tr.y), std::max(br.y, bl.y));
    const sf::FloatRect localVisible({minX, minY}, {maxX - minX, maxY - minY});
    const sf::Vector2i sourceTileSize = {
      static_cast<int>(map_.tileWidth()),
      static_cast<int>(map_.tileHeight())
    };

    for (const auto& ch : layer.chunks) {
      if (!ch.visible || ch.opacity <= 0.f || ch.vertices.getVertexCount() == 0) {
        continue;
      }

      const sf::FloatRect b = boundsFor(ch.vertices);

      if (!b.findIntersection(localVisible)) {
        continue;
      }

      sf::RenderStates cs = states;
      cs.texture = ch.texture;

      // Scale geometry from tileset pixels -> world grid pixels
      if (sourceTileSize.x > 0 && sourceTileSize.y > 0) {
        const float sx = static_cast<float>(map_.tileWidth()) /
                        static_cast<float>(sourceTileSize.x);
        const float sy = static_cast<float>(map_.tileHeight()) /
                        static_cast<float>(sourceTileSize.y);
        cs.transform.scale(sf::Vector2f(sx, sy));
      }

      target.draw(ch.vertices, cs);
    }

    return;
  }

  for (const auto& ch : layer.chunks) {
    if (!ch.visible || ch.opacity <= 0.f) continue;
    sf::RenderStates cs = s;
    cs.texture = ch.texture;
    target.draw(ch.vertices, cs);
  }
}

void WorldRenderer::renderGround(sf::RenderTarget& target) const {
  renderGround(target, sf::RenderStates{});
}

void WorldRenderer::renderGround(sf::RenderTarget& target,
                                 sf::RenderStates states) const {
  for (const auto& l : map_.layers()) {
    if (isGroundName(l.name)) {
      drawLayerMesh(target, states, l);
    }
  }

  if (debugGrid_) {
    sf::RenderStates s = states;
    s.transform *= getTransform();

    const auto& v = target.getView();
    const sf::Vector2f c = v.getCenter();
    const sf::Vector2f sz = v.getSize();
    const sf::FloatRect worldView({c.x - sz.x * 0.5f, c.y - sz.y * 0.5f},
                                  {sz.x, sz.y});

    drawDebugGrid(target, s, worldView);
  }
}

void WorldRenderer::renderOverlays(sf::RenderTarget& target) const {
  renderOverlays(target, sf::RenderStates{});
}

void WorldRenderer::renderOverlays(sf::RenderTarget& target,
                                   sf::RenderStates states) const {
  for (const auto& l : map_.layers()) {
    if (isOverlayName(l.name)) {
      drawLayerMesh(target, states, l);
    }
  }
}

void WorldRenderer::draw(sf::RenderTarget& target,
                         sf::RenderStates states) const {
  // Legacy: draw all layers in map order (useful for quick debugging)
  for (const auto& l : map_.layers()) {
    drawLayerMesh(target, states, l);
  }
}

void WorldRenderer::drawDebugGrid(sf::RenderTarget& target,
                                  sf::RenderStates states,
                                  const sf::FloatRect& visibleWorld) const {
  const int tw = map_.tileWidth();
  const int th = map_.tileHeight();
  if (tw <= 0 || th <= 0) return;

  const int tx0 =
      static_cast<int>(std::floor(visibleWorld.position.x / tw)) - 1;
  const int ty0 =
      static_cast<int>(std::floor(visibleWorld.position.y / th)) - 1;
  const int tx1 = static_cast<int>(std::ceil(
                      (visibleWorld.position.x + visibleWorld.size.x) / tw)) +
                  1;
  const int ty1 = static_cast<int>(std::ceil(
                      (visibleWorld.position.y + visibleWorld.size.y) / th)) +
                  1;

  sf::VertexArray lines(sf::PrimitiveType::Lines);
  auto push = [&](sf::Vector2f a, sf::Vector2f b) {
    sf::Vertex va(a, debugGridColor_);
    sf::Vertex vb(b, debugGridColor_);
    lines.append(va);
    lines.append(vb);
  };

  for (int ty = ty0; ty < ty1; ++ty) {
    for (int tx = tx0; tx < tx1; ++tx) {
      const float x = static_cast<float>(tx * tw);
      const float y = static_cast<float>(ty * th);
      const float x2 = x + static_cast<float>(tw);
      const float y2 = y + static_cast<float>(th);
      push({x, y}, {x2, y});
      push({x2, y}, {x2, y2});
      push({x2, y2}, {x, y2});
      push({x, y2}, {x, y});
    }
  }
  target.draw(lines, states);
}

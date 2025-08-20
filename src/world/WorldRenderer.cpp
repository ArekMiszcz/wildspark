// Copyright 2025 WildSpark Authors

#include "WorldRenderer.h"

#include <algorithm>
#include <limits>

#include <SFML/Graphics/ConvexShape.hpp>
#include <SFML/Graphics/RectangleShape.hpp>

sf::FloatRect WorldRenderer::boundsFor(const sf::VertexArray& va) const {
  BoundsKey key{&va, va.getVertexCount()};

  if (auto it = cache_.find(key); it != cache_.end()) {
    return it->second;
  }

  const std::size_t n = va.getVertexCount();
  if (n == 0) {
    sf::FloatRect empty{sf::Vector2f(0.f, 0.f), sf::Vector2f(0.f, 0.f)};
    cache_.emplace(key, empty);
    return empty;
  }

  float minX = std::numeric_limits<float>::infinity();
  float minY = std::numeric_limits<float>::infinity();
  float maxX = -std::numeric_limits<float>::infinity();
  float maxY = -std::numeric_limits<float>::infinity();

  for (std::size_t i = 0; i < n; ++i) {
    const sf::Vector2f p = va[i].position;
    if (p.x < minX) minX = p.x;
    if (p.y < minY) minY = p.y;
    if (p.x > maxX) maxX = p.x;
    if (p.y > maxY) maxY = p.y;
  }

  sf::FloatRect r{sf::Vector2f(minX, minY),
                  sf::Vector2f(maxX - minX, maxY - minY)};
  cache_.emplace(key, r);
  return r;
}

void WorldRenderer::drawDebugGrid(sf::RenderTarget& target,
                                  sf::RenderStates states,
                                  const sf::FloatRect& localViewRect) const {
  // Compute visible tile range in local space (orthogonal layout)
  const int tw = map_.tileWidth();
  const int th = map_.tileHeight();
  if (tw <= 0 || th <= 0) return;

  // Clamp to map bounds
  const sf::FloatRect wb = map_.worldBounds();
  const float vx0 = std::max(localViewRect.position.x, wb.position.x);
  const float vy0 = std::max(localViewRect.position.y, wb.position.y);
  const float vx1 = std::min(localViewRect.position.x + localViewRect.size.x,
                             wb.position.x + wb.size.x);
  const float vy1 = std::min(localViewRect.position.y + localViewRect.size.y,
                             wb.position.y + wb.size.y);
  if (vx1 <= vx0 || vy1 <= vy0) return;

  const int tx0 = std::max(0, static_cast<int>(std::floor(vx0 / tw)));
  const int ty0 = std::max(0, static_cast<int>(std::floor(vy0 / th)));
  const int tx1 = std::min(map_.width(), static_cast<int>(std::ceil(vx1 / tw)));
  const int ty1 =
      std::min(map_.height(), static_cast<int>(std::ceil(vy1 / th)));

  // Build a single vertex array for all tile borders (Lines)
  sf::VertexArray lines(sf::PrimitiveType::Lines);

  auto pushLine = [&](sf::Vector2f a, sf::Vector2f b) {
    sf::Vertex va(a, debugGridColor_);
    sf::Vertex vb(b, debugGridColor_);
    lines.append(va);
    lines.append(vb);
  };

  // Draw rectangle per tile (top, right, bottom, left)
  for (int ty = ty0; ty < ty1; ++ty) {
    for (int tx = tx0; tx < tx1; ++tx) {
      const float x = static_cast<float>(tx * tw);
      const float y = static_cast<float>(ty * th);
      const float x2 = x + static_cast<float>(tw);
      const float y2 = y + static_cast<float>(th);

      // top
      pushLine({x, y}, {x2, y});
      // right
      pushLine({x2, y}, {x2, y2});
      // bottom
      pushLine({x2, y2}, {x, y2});
      // left
      pushLine({x, y2}, {x, y});
    }
  }

  // Draw with same transform/states as the map
  target.draw(lines, states);
}

void WorldRenderer::draw(sf::RenderTarget& target,
                         sf::RenderStates states) const {
  sf::RenderStates s = states;
  sf::FloatRect localViewRect;

  s.transform *= getTransform();

  if (cull_) {
    const sf::View& v = target.getView();
    const sf::Vector2f c = v.getCenter();
    const sf::Vector2f sz = v.getSize();

    sf::FloatRect worldView({c.x - sz.x * 0.5f, c.y - sz.y * 0.5f},
                            {sz.x, sz.y});

    const sf::Transform inv = s.transform.getInverse();
    const sf::Vector2f pos = worldView.position;
    const sf::Vector2f size = worldView.size;

    const sf::Vector2f tl = inv.transformPoint(pos);
    const sf::Vector2f tr = inv.transformPoint({pos.x + size.x, pos.y});
    const sf::Vector2f br =
        inv.transformPoint({pos.x + size.x, pos.y + size.y});
    const sf::Vector2f bl = inv.transformPoint({pos.x, pos.y + size.y});

    float minX = std::min(std::min(tl.x, tr.x), std::min(br.x, bl.x));
    float minY = std::min(std::min(tl.y, tr.y), std::min(br.y, bl.y));
    float maxX = std::max(std::max(tl.x, tr.x), std::max(br.x, bl.x));
    float maxY = std::max(std::max(tl.y, tr.y), std::max(br.y, bl.y));
    localViewRect = {{minX, minY}, {maxX - minX, maxY - minY}};
  }

  if (cull_) {
    const sf::FloatRect mapBoundsLocal = map_.worldBounds();
    if (!mapBoundsLocal.findIntersection(localViewRect)) {
      return;
    }
  }

  for (const auto& layer : map_.layers()) {
    if (!layer.visible) {
      continue;
    }

    for (const auto& chunk : layer.chunks) {
      if (!chunk.visible || !chunk.ts || !chunk.ts->texture) {
        continue;
      }

      if (cull_) {
        const sf::FloatRect chunkLocal = boundsFor(chunk.vertices);
        if (!chunkLocal.findIntersection(localViewRect)) {
          continue;
        }
      }

      s.texture = chunk.ts->texture.get();
      target.draw(chunk.vertices, s);
    }
  }

  if (debugGrid_) {
    drawDebugGrid(target, s, localViewRect);
  }
}

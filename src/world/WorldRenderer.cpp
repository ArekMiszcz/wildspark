// Copyright 2025 WildSpark Authors

#include "WorldRenderer.h"

#include <algorithm>
#include <limits>
#include <string>
#include <unordered_set>
#include <vector>

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

bool WorldRenderer::isObjectLayerName(std::string_view name) {
  std::string n(name);
  n = toLower(n);
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

  // Build a flat draw list: prefer object_draw_order for object layers to
  // preserve global Y ordering; otherwise iterate bucket order.
  std::vector<const LM::Chunk*> drawList;
  drawList.reserve(128);
  if (isObjectLayerName(layer.name) && !layer.object_draw_order.empty()) {
    for (const auto* p : layer.object_draw_order) if (p) drawList.push_back(p);
  } else {
    for (const auto& key : layer.chunk_bucket_order) {
      auto itBucket = layer.chunk_buckets.find(key);
      if (itBucket == layer.chunk_buckets.end()) continue;
      for (const auto& ch : itBucket->second.chunks) drawList.push_back(&ch);
    }
  }

  // If culling is enabled compute the local-visible rect once
  sf::FloatRect localVisible;
  if (cull_) {
    const auto& v = target.getView();
    const sf::Vector2f c = v.getCenter();
    const sf::Vector2f sz = v.getSize();
    const sf::FloatRect worldView({c.x - sz.x * 0.5f, c.y - sz.y * 0.5f}, {sz.x, sz.y});
    const sf::Transform inv = s.transform.getInverse();
    const sf::Vector2f tl = inv.transformPoint(worldView.position);
    const sf::Vector2f tr = inv.transformPoint({worldView.position.x + worldView.size.x, worldView.position.y});
    const sf::Vector2f br = inv.transformPoint({
        worldView.position.x + worldView.size.x,
        worldView.position.y + worldView.size.y
    });
    const sf::Vector2f bl = inv.transformPoint({worldView.position.x, worldView.position.y + worldView.size.y});
    const float minX = std::min(std::min(tl.x, tr.x), std::min(br.x, bl.x));
    const float minY = std::min(std::min(tl.y, tr.y), std::min(br.y, bl.y));
    const float maxX = std::max(std::max(tl.x, tr.x), std::max(br.x, bl.x));
    const float maxY = std::max(std::max(tl.y, tr.y), std::max(br.y, bl.y));
    localVisible = sf::FloatRect({minX, minY}, {maxX - minX, maxY - minY});
  }

  // Draw all chunks from the draw list, applying culling when active
  for (const auto* chptr : drawList) {
    if (!chptr) continue;
    const auto& ch = *chptr;
    if (!ch.visible || ch.opacity <= 0.f || ch.vertices.getVertexCount() == 0) continue;

    if (cull_) {
      const sf::FloatRect b = boundsFor(ch.vertices);
      if (!b.findIntersection(localVisible)) continue;
    }

    sf::RenderStates cs = s;
    cs.texture = ch.texture;
    target.draw(ch.vertices, cs);
  }

  if (debugObjectAreas_ && isObjectLayerName(layer.name)) {
    drawDebugObjectAreas(target, states, layer);
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

void WorldRenderer::drawDebugObjectAreas(
    sf::RenderTarget& target, sf::RenderStates states,
    const WorldMap::LayerMesh& layer) const {
  if (!layer.visible || layer.opacity <= 0.f || !debugObjectAreas_) {
    return;
  }

  sf::RenderStates s = states;
  s.transform *= getTransform();

  // For each chunk in the layer, draw both chunk bounds and their associated
  // object groups
  for (const auto& [_, bucket] : layer.chunk_buckets) {
    for (const auto& ch : bucket.chunks) {
      if (!ch.visible || ch.opacity <= 0.f || ch.vertices.getVertexCount() < 6) {
        continue;
      }

      // Extract vertices to find world coordinates of the tiles
      // We're assuming triangles (2 triangles per tile)
      const auto& vertices = ch.vertices;
      const size_t vertCount = vertices.getVertexCount();

      // Only process complete triangles
      if (vertCount % 3 != 0) {
        continue;
      }

      // Track processed tiles to avoid duplicates
      std::unordered_set<uint32_t> processedTiles;

      // Process each triangle in the chunk
      for (size_t i = 0; i < vertCount; i += 3) {
        // Skip if incomplete triangle
        if (i + 2 >= vertCount) {
          continue;
        }

        // Use first vertex of each triangle to get tile position
        const sf::Vector2f& pos = vertices[i].position;
        const uint32_t gid = static_cast<uint32_t>(ch.gid);

        // Skip if we've already processed this tile
        if (processedTiles.find(gid) != processedTiles.end()) {
          continue;
        }
        processedTiles.insert(gid);

        // Skip empty tiles (gid == 0)
        if (gid == 0) {
          continue;
        }

        // Find the tileset for this GID
        const WorldMap::Tileset* tileset = map_.findTilesetForGid(gid);
        if (!tileset) {
          continue;  // No tileset found for this gid
        }

        const int localId = gid - tileset->firstGid;
        if (tileset->objectGroups.find(localId) == tileset->objectGroups.end()) {
          continue;  // No object group for this tile
        }

        // For each object in the group
        for (const auto& obj : tileset->objectGroups.at(localId).objects) {
            if (!obj.visible) continue;

            // Different colors for different object types
            sf::Color objColor;
            if (obj.type == "collider") {
              objColor = sf::Color::Red;
            } else if (obj.type == "clickable") {
              objColor = sf::Color::Green;
            } else {
              continue;
            }

            // Adjust alpha for better visibility
            objColor.a = 128;

            // Draw polygon objects
            if (!obj.polygon.empty()) {
              sf::ConvexShape polygon;
              polygon.setPointCount(obj.polygon.size());

              // Set the position of each point
              for (size_t i = 0; i < obj.polygon.size(); ++i) {
                // Map object local coordinates to world coordinates
                sf::Vector2f worldPos = map_.objectToWorld(
                    gid, pos.x, pos.y, obj.x + obj.polygon[i].x,
                    obj.y + obj.polygon[i].y);

                polygon.setPoint(i, worldPos);
              }

              // Draw outline
              polygon.setFillColor(sf::Color::Transparent);
              polygon.setOutlineColor(objColor);
              polygon.setOutlineThickness(2.0f);
              target.draw(polygon, s);

              // Draw semi-transparent fill
              sf::ConvexShape fillPolygon = polygon;
              objColor.a = 64;
              fillPolygon.setFillColor(objColor);
              fillPolygon.setOutlineThickness(0);
              target.draw(fillPolygon, s);
            } else if (obj.width > 0 &&
                      obj.height > 0) {  // Draw rectangle objects
              // Map object's rectangle to world coordinates
              sf::Vector2f worldPos =
                  map_.objectToWorld(gid, pos.x, pos.y, obj.x, obj.y);

              sf::RectangleShape rect;
              rect.setPosition(worldPos);
              rect.setSize(sf::Vector2f(obj.width, obj.height));
              rect.setFillColor(sf::Color::Transparent);
              rect.setOutlineColor(objColor);
              rect.setOutlineThickness(2.0f);

              // Apply rotation if needed
              if (obj.rotation != 0) {
                rect.setRotation(sf::degrees(obj.rotation));
              }

              target.draw(rect, s);

              // Draw filled rectangle with transparency
              sf::RectangleShape fillRect = rect;
              objColor.a = 64;
              fillRect.setFillColor(objColor);
              fillRect.setOutlineThickness(0);
              target.draw(fillRect, s);
            }
          }
      }

      // Draw the chunk bounds for debugging
      sf::FloatRect bounds = boundsFor(ch.vertices);
      sf::RectangleShape rect;
      rect.setPosition(sf::Vector2f(bounds.position.x, bounds.position.y));
      rect.setSize(sf::Vector2f(bounds.size.x, bounds.size.y));
      rect.setFillColor(sf::Color::Transparent);
      rect.setOutlineColor(
          sf::Color(128, 128, 128, 128));  // Gray with transparency
      rect.setOutlineThickness(1.0f);
      target.draw(rect, s);
    }
  }
}

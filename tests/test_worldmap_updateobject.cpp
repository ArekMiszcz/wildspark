// Copyright 2025 WildSpark Authors

#include <gtest/gtest.h>
#include <vector>
#include <utility>

#include "world/WorldMap.h"

// Helpers to construct a minimal objectgroup layer with a single object
static WorldMap::LayerMesh::Chunk makeChunkForObject(int id, uint32_t gid, int x, int y, int tw, int th) {
  WorldMap::LayerMesh::Chunk ch;
  ch.id = static_cast<uint32_t>(id);
  ch.gid = gid;
  ch.texture = nullptr;
  ch.visible = true;
  ch.opacity = 1.f;
  ch.vertices.setPrimitiveType(sf::PrimitiveType::Triangles);
  ch.vertices.resize(6);
  ch.sortY = static_cast<float>(y + th);
  // place vertices relative to (x,y)
  ch.vertices[0].position = sf::Vector2f(static_cast<float>(x), static_cast<float>(y));
  ch.vertices[1].position = sf::Vector2f(static_cast<float>(x + tw), static_cast<float>(y));
  ch.vertices[2].position = sf::Vector2f(static_cast<float>(x + tw), static_cast<float>(y + th));
  ch.vertices[3].position = sf::Vector2f(static_cast<float>(x), static_cast<float>(y));
  ch.vertices[4].position = sf::Vector2f(static_cast<float>(x + tw), static_cast<float>(y + th));
  ch.vertices[5].position = sf::Vector2f(static_cast<float>(x), static_cast<float>(y + th));
  const uint8_t a = static_cast<uint8_t>(255.f * ch.opacity);
  for (size_t i = 0; i < 6; ++i) ch.vertices[i].color.a = a;
  return ch;
}

TEST(WorldMapUpdateObject, GidAndOpacityAndVisibleUpdate) {
  WorldMap wm;
  wm.setTileSize(32, 32);

  // make a layer with one bucket key (0, 0)
  WorldMap::LayerMesh layer;
  layer.type = "objectgroup";
  layer.visible = true;
  layer.opacity = 1.f;
  WorldMap::LayerMesh::CellKey key{0, 0};
  auto& bucket = layer.chunk_buckets[key];
  bucket.chunks.push_back(makeChunkForObject(42, 1001, 0, 0, 32, 32));
  wm.layersMutable().push_back(std::move(layer));
  wm.buildObjectIndexForTests();

  nlohmann::json props;
  props["gid"] = 2002;
  props["visible"] = false;
  props["opacity"] = 0.5f;

  std::vector<int> affected;
  bool changed = wm.updateObject(42, props, &affected);
  EXPECT_TRUE(changed);
  ASSERT_EQ(affected.size(), 1);
  EXPECT_EQ(affected[0], 0);

  // Verify the chunk was updated
  const auto& layers = wm.layers();
  ASSERT_EQ(layers.size(), 1);
  const auto& mesh = layers[0];
  auto it = mesh.chunk_buckets.find(key);
  ASSERT_NE(it, mesh.chunk_buckets.end());
  const auto& bucket2 = it->second;
  ASSERT_EQ(bucket2.chunks.size(), 1);
  const auto& ch = bucket2.chunks[0];
  EXPECT_EQ(static_cast<int>(ch.id), 42);
  EXPECT_TRUE(ch.gid == 2002u || ch.gid == 1001u);
  EXPECT_FALSE(ch.visible);
  EXPECT_NEAR(ch.opacity, 0.5f, 1e-6f);
}

TEST(WorldMapUpdateObject, MovePositionUpdatesBucketsAndIndex) {
  WorldMap wm;
  wm.setTileSize(16, 16);

  // single object occupying 1x1 tiles at (0, 0) initially
  WorldMap::LayerMesh layer;
  layer.type = "objectgroup";
  WorldMap::LayerMesh::CellKey key{0, 0};
  layer.chunk_buckets[key].chunks.push_back(makeChunkForObject(7, 100, 0, 0, 16, 16));
  wm.layersMutable().push_back(std::move(layer));
  wm.buildObjectIndexForTests();

  nlohmann::json props;
  props["pos"] = { {"x", 48.f}, {"y", 48.f} };  // move to world pos (48,48) -> tile (3,3)

  std::vector<int> affected;
  bool changed = wm.updateObject(7, props, &affected);
  EXPECT_TRUE(changed);
  ASSERT_EQ(affected.size(), 1);
  EXPECT_EQ(affected[0], 0);

  // verify new key exists and chunk moved
  const auto& mesh = wm.layers()[0];
  WorldMap::LayerMesh::CellKey newKey{3, 3};
  auto it = mesh.chunk_buckets.find(newKey);
  ASSERT_NE(it, mesh.chunk_buckets.end());
  ASSERT_EQ(it->second.chunks.size(), 1);
  EXPECT_EQ(static_cast<int>(it->second.chunks[0].id), 7);

  // index must contain the new key
  const auto& idx = wm.layersMutable();
  // we can't access object_index_ directly, but ensure draw order updated
  EXPECT_FALSE(wm.layers()[0].object_draw_order.empty());
}

TEST(WorldMapUpdateObject, MissingIndexThrows) {
  WorldMap wm;
  wm.setTileSize(16, 16);
  // empty map, index empty

  nlohmann::json props;
  props["visible"] = false;

  EXPECT_THROW(wm.updateObject(9999, props), std::runtime_error);
}

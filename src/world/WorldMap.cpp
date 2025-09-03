// Copyright 2025 WildSpark Authors

#include "WorldMap.h"

#include <algorithm>
#include <cstdio>
#include <filesystem>
#include <fstream>
#include <iostream>
#include <memory>
#include <stdexcept>
#include <string>
#include <unordered_map>
#include <utility>
#include <vector>

#include "../vendor/dotenv-cpp/dotenv.h"

#include <nlohmann/json.hpp>

using nlohmann::json;
namespace fs = std::filesystem;

static std::string readFile(const fs::path& p) {
  std::ifstream ifs(p, std::ios::binary);
  if (!ifs) throw std::runtime_error("Failed to open: " + p.string());
  return std::string(std::istreambuf_iterator<char>(ifs), {});
}

WorldMap::WorldMap(const std::string& mapJsonPath) {
  loadFromJson(mapJsonPath);
}

void WorldMap::loadFromJson(const std::string& mapPathStr) {
  fs::path mapPath = dotenv::getenv("MAPS_DIR") + mapPathStr;
  fs::path mapDir = mapPath.parent_path();

  printf("Loading map from: %s\n", mapPath.c_str());

  const json j = json::parse(readFile(mapPath));
  const std::string orientation = j.value("orientation", "orthogonal");

  if (orientation != "orthogonal")
    throw std::runtime_error("Only orthogonal maps are supported. Got: " +
                             orientation);

  mapWidth_ = j.at("width").get<int>();
  mapHeight_ = j.at("height").get<int>();
  tileWidth_ = j.at("tilewidth").get<int>();
  tileHeight_ = j.at("tileheight").get<int>();

  // tilesets (external or embedded)
  for (const auto& tsj : j.at("tilesets")) {
    if (tsj.contains("source")) {
      loadTilesetExternal(mapDir, tsj.at("source").get<std::string>(),
                          tsj.at("firstgid").get<int>());
    } else {
      loadTilesetInline(mapDir, tsj);
    }
  }
  std::sort(tilesets_.begin(), tilesets_.end(),
            [](const Tileset& a, const Tileset& b) {
              return a.firstGid < b.firstGid;
            });

  // layers
  buildLayers(j);
}

void WorldMap::loadTilesetInline(const fs::path& mapDir, const json& tsj) {
  Tileset ts;
  ts.firstGid = tsj.at("firstgid").get<int>();
  ts.name = tsj.value("name", "");
  ts.tileWidth = tsj.at("tilewidth").get<int>();
  ts.tileHeight = tsj.at("tileheight").get<int>();
  ts.margin = tsj.value("margin", 0);
  ts.spacing = tsj.value("spacing", 0);
  ts.columns = tsj.value("columns", 0);

  if (tsj.contains("image")) {
    ts.imageCollection = false;
    ts.imagePath = (mapDir / tsj.at("image").get<std::string>()).string();
    ts.imageWidth = tsj.value("imagewidth", 0);
    ts.imageHeight = tsj.value("imageheight", 0);

    ts.texture = std::make_shared<sf::Texture>();
    if (!ts.texture->loadFromFile(ts.imagePath))
      throw std::runtime_error("Tileset image load failed: " + ts.imagePath);
    if (ts.imageWidth == 0 || ts.imageHeight == 0) {
      auto s = ts.texture->getSize();
      ts.imageWidth = static_cast<int>(s.x);
      ts.imageHeight = static_cast<int>(s.y);
    }
    if (ts.columns <= 0) {
      int denom = ts.tileWidth + ts.spacing;
      if (denom <= 0) throw std::runtime_error("Invalid tileset denom");
      ts.columns = (ts.imageWidth - 2 * ts.margin + ts.spacing) / denom;
      if (ts.columns <= 0) throw std::runtime_error("Computed columns <= 0");
    }
  } else if (tsj.contains("tiles") && tsj.at("tiles").is_array()) {
    ts.imageCollection = true;
    for (const auto& tile : tsj.at("tiles")) {
      if (!tile.contains("id") || !tile.contains("image")) continue;
      Tileset::PerTile pt;
      pt.localId = tile.at("id").get<int>();
      fs::path img = mapDir / tile.at("image").get<std::string>();
      pt.texture = std::make_shared<sf::Texture>();
      if (!pt.texture->loadFromFile(img.string()))
        throw std::runtime_error("Tile image load failed: " + img.string());
      pt.width = tile.value("imagewidth", 0);
      pt.height = tile.value("imageheight", 0);
      if (pt.width == 0 || pt.height == 0) {
        auto s = pt.texture->getSize();
        pt.width = static_cast<int>(s.x);
        pt.height = static_cast<int>(s.y);
      }

      // Parse object groups for this tile if they exist
      if (tile.contains("objectgroup") && tile["objectgroup"].is_object()) {
        const auto& objGroup = tile["objectgroup"];
        Tileset::ObjectGroup group;
        group.id = objGroup.value("id", 0);
        group.name = objGroup.value("name", "");
        group.draworder = objGroup.value("draworder", "index");
        group.opacity = objGroup.value("opacity", 1.0f);
        group.visible = objGroup.value("visible", true);

        // Parse objects
        if (objGroup.contains("objects") && objGroup["objects"].is_array()) {
          for (const auto& obj : objGroup["objects"]) {
            Tileset::Object object;
            object.id = obj.value("id", 0);
            object.name = obj.value("name", "");
            object.type = obj.value("type", "");
            object.x = obj.value("x", 0.0f);
            object.y = obj.value("y", 0.0f);
            object.width = obj.value("width", 0.0f);
            object.height = obj.value("height", 0.0f);
            object.rotation = obj.value("rotation", 0.0f);
            object.visible = obj.value("visible", true);

            // Parse polygon points if they exist
            if (obj.contains("polygon") && obj["polygon"].is_array()) {
              for (const auto& point : obj["polygon"]) {
                Tileset::Point p;
                p.x = point.value("x", 0.0f);
                p.y = point.value("y", 0.0f);
                object.polygon.push_back(p);
              }
            }

            group.objects.push_back(object);
          }
        }

        ts.objectGroups[pt.localId] = std::move(group);
      }

      ts.perTile.emplace(pt.localId, std::move(pt));
    }
    if (ts.perTile.empty())
      throw std::runtime_error("Image-collection tileset has no tiles.");
  } else {
    throw std::runtime_error(
        "Unsupported embedded tileset format (need 'image' or 'tiles[]').");
  }

  tilesets_.push_back(std::move(ts));
}

void WorldMap::loadTilesetExternal(const fs::path& mapDir,
                                   const std::string& source, int firstGid) {
  fs::path src = mapDir / source;
  if (src.extension() == ".tsx")
    throw std::runtime_error(
        "TSX (XML) not supported; export tileset as JSON (.tsj/.json): " +
        src.string());

  const json tj = json::parse(readFile(src));

  Tileset ts;
  ts.firstGid = firstGid;
  ts.name = tj.value("name", "");
  ts.tileWidth = tj.value("tilewidth", 0);
  ts.tileHeight = tj.value("tileheight", 0);
  ts.margin = tj.value("margin", 0);
  ts.spacing = tj.value("spacing", 0);
  ts.columns = tj.value("columns", 0);

  if (tj.contains("image")) {
    ts.imageCollection = false;
    ts.imagePath =
        (src.parent_path() / tj.at("image").get<std::string>()).string();
    ts.imageWidth = tj.value("imagewidth", 0);
    ts.imageHeight = tj.value("imageheight", 0);

    ts.texture = std::make_shared<sf::Texture>();
    if (!ts.texture->loadFromFile(ts.imagePath))
      throw std::runtime_error("Tileset image load failed: " + ts.imagePath);
    if (ts.imageWidth == 0 || ts.imageHeight == 0) {
      auto s = ts.texture->getSize();
      ts.imageWidth = static_cast<int>(s.x);
      ts.imageHeight = static_cast<int>(s.y);
    }
    if (ts.columns <= 0) {
      int denom = ts.tileWidth + ts.spacing;
      if (denom <= 0) throw std::runtime_error("Invalid tileset denom");
      ts.columns = (ts.imageWidth - 2 * ts.margin + ts.spacing) / denom;
      if (ts.columns <= 0) throw std::runtime_error("Computed columns <= 0");
    }
  } else if (tj.contains("tiles") && tj.at("tiles").is_array()) {
    ts.imageCollection = true;
    for (const auto& tile : tj.at("tiles")) {
      if (!tile.contains("id") || !tile.contains("image")) continue;
      Tileset::PerTile pt;
      pt.localId = tile.at("id").get<int>();
      fs::path img = src.parent_path() / tile.at("image").get<std::string>();
      pt.texture = std::make_shared<sf::Texture>();
      if (!pt.texture->loadFromFile(img.string()))
        throw std::runtime_error("Tile image load failed: " + img.string());
      pt.width = tile.value("imagewidth", 0);
      pt.height = tile.value("imageheight", 0);
      if (pt.width == 0 || pt.height == 0) {
        auto s = pt.texture->getSize();
        pt.width = static_cast<int>(s.x);
        pt.height = static_cast<int>(s.y);
      }

      // Parse object groups for this tile if they exist
      if (tile.contains("objectgroup") && tile["objectgroup"].is_object()) {
        const auto& objGroup = tile["objectgroup"];
        Tileset::ObjectGroup group;
        group.id = objGroup.value("id", 0);
        group.name = objGroup.value("name", "");
        group.draworder = objGroup.value("draworder", "index");
        group.opacity = objGroup.value("opacity", 1.0f);
        group.visible = objGroup.value("visible", true);

        // Parse objects
        if (objGroup.contains("objects") && objGroup["objects"].is_array()) {
          for (const auto& obj : objGroup["objects"]) {
            Tileset::Object object;
            object.id = obj.value("id", 0);
            object.name = obj.value("name", "");
            object.type = obj.value("type", "");
            object.x = obj.value("x", 0.0f);
            object.y = obj.value("y", 0.0f);
            object.width = obj.value("width", 0.0f);
            object.height = obj.value("height", 0.0f);
            object.rotation = obj.value("rotation", 0.0f);
            object.visible = obj.value("visible", true);

            // Parse polygon points if they exist
            if (obj.contains("polygon") && obj["polygon"].is_array()) {
              for (const auto& point : obj["polygon"]) {
                Tileset::Point p;
                p.x = point.value("x", 0.0f);
                p.y = point.value("y", 0.0f);
                object.polygon.push_back(p);
              }
            }

            group.objects.push_back(object);
          }
        }

        ts.objectGroups[pt.localId] = std::move(group);
      }

      ts.perTile.emplace(pt.localId, std::move(pt));
    }
    if (ts.perTile.empty())
      throw std::runtime_error("Image-collection tileset has no tiles.");
  } else {
    throw std::runtime_error("Unsupported external tileset format: " +
                             src.string());
  }

  tilesets_.push_back(std::move(ts));
}

const WorldMap::Tileset* WorldMap::findTilesetForGid(uint32_t raw) const {
  if (raw == 0) return nullptr;
  const uint32_t id = clearFlipFlags(raw);
  const Tileset* best = nullptr;
  for (const auto& ts : tilesets_) {
    if (static_cast<uint32_t>(ts.firstGid) <= id)
      best = &ts;
    else
      break;
  }
  return best;
}

void WorldMap::applyFlipTexcoords(bool h, bool v, bool d, sf::Vector2f tc[4]) {
  if (h) {
    std::swap(tc[0], tc[1]);
    std::swap(tc[3], tc[2]);
  }
  if (v) {
    std::swap(tc[0], tc[3]);
    std::swap(tc[1], tc[2]);
  }
  if (d) {
    std::swap(tc[1], tc[3]);
  }
}

void WorldMap::buildLayers(const json& j) {
  layers_.clear();
  for (const auto& lj : j.at("layers")) {
    const std::string type = lj.at("type").get<std::string>();

    // batch by texture
    auto makeLayer = [&](LayerMesh& mesh) {
      mesh.type = type;
      mesh.name = lj.value("name", "");
      mesh.visible = lj.value("visible", true);
      mesh.opacity = lj.value("opacity", 1.f);
    };

    // helper to append one tile
    auto appendTile = [&](LayerMesh& mesh, const Tileset* ts, uint32_t localId,
                          const sf::Vector2f& pos, bool h, bool v, bool d) {
      sf::Vector2f uv[4];
      int tw = 0, th = 0;
      const sf::Texture* tex = nullptr;

      if (!ts->imageCollection) {
        const int cols = ts->columns;
        if (cols <= 0)
          throw std::runtime_error("Tileset '" + ts->name + "' columns <= 0");

        const int tu = static_cast<int>(localId % cols);
        const int tv = static_cast<int>(localId / cols);
        tw = ts->tileWidth;
        th = ts->tileHeight;

        const int margin = ts->margin, spacing = ts->spacing;
        const float left = static_cast<float>(margin + tu * (tw + spacing));
        const float top = static_cast<float>(margin + tv * (th + spacing));
        const float right = left + tw;
        const float bottom = top + th;
        uv[0] = {left, top};
        uv[1] = {right, top};
        uv[2] = {right, bottom};
        uv[3] = {left, bottom};
        tex = ts->texture.get();
      } else {
        auto it = ts->perTile.find(static_cast<int>(localId));
        if (it == ts->perTile.end()) return;  // missing tile, skip
        const auto& pt = it->second;
        tw = pt.width;
        th = pt.height;
        uv[0] = {0.f, 0.f};
        uv[1] = {static_cast<float>(tw), 0.f};
        uv[2] = {static_cast<float>(tw), static_cast<float>(th)};
        uv[3] = {0.f, static_cast<float>(th)};
        tex = pt.texture.get();
      }

      applyFlipTexcoords(h, v, d, uv);

      // find or make chunk by texture
      LayerMesh::Chunk* chunk = nullptr;

      for (auto& c : mesh.chunk_buckets) {
        std::vector<LayerMesh::Chunk>& cl = c.second.chunks;
        for (auto& c : cl) {
          if (c.texture == tex) {
            chunk = &c;
            break;
          }
        }
        if (chunk) break;
      }

      if (!chunk) {
        LayerMesh::CellKey key{
            static_cast<int>(std::floor(pos.x / tileWidth_)),
            static_cast<int>(std::floor(pos.y / tileHeight_))};
        auto [it, _] =
            mesh.chunk_buckets.emplace(key, LayerMesh::ChunkBucket{});
        it->second.chunks.push_back(LayerMesh::Chunk{});
        chunk = &(it->second.chunks.back());
        chunk->gid = ts->firstGid + localId;
        chunk->texture = tex;
        chunk->visible = mesh.visible;
        chunk->opacity = mesh.opacity;
        chunk->vertices.setPrimitiveType(sf::PrimitiveType::Triangles);
      }

      const size_t base = chunk->vertices.getVertexCount();
      chunk->vertices.resize(base + 6);
      sf::Vertex* t = &chunk->vertices[base];

      // 2 triangles
      t[0].position = pos;
      t[0].texCoords = uv[0];
      t[1].position = {pos.x + tw, pos.y};
      t[1].texCoords = uv[1];
      t[2].position = {pos.x + tw, pos.y + th};
      t[2].texCoords = uv[2];
      t[3].position = pos;
      t[3].texCoords = uv[0];
      t[4].position = {pos.x + tw, pos.y + th};
      t[4].texCoords = uv[2];
      t[5].position = {pos.x, pos.y + th};
      t[5].texCoords = uv[3];

      const std::uint8_t a = static_cast<std::uint8_t>(255.f * mesh.opacity);
      for (int i = 0; i < 6; ++i) {
        t[i].color.a = a;
      }
    };

    // tile layers (grid-aligned)
    if (type == "tilelayer") {
      if (!lj.contains("data") || !lj.at("data").is_array())
        throw std::runtime_error(
            "Only finite maps with 'data' arrays are supported.");

      LayerMesh mesh;
      makeLayer(mesh);
      const auto data = lj.at("data").get<std::vector<uint32_t>>();
      if (static_cast<int>(data.size()) != mapWidth_ * mapHeight_)
        throw std::runtime_error("Layer size mismatch");

      for (int ty = 0; ty < mapHeight_; ++ty) {
        for (int tx = 0; tx < mapWidth_; ++tx) {
          const uint32_t raw = data[ty * mapWidth_ + tx];
          if (raw == 0) continue;
          const bool h = (raw & 0x80000000u) != 0, v = (raw & 0x40000000u) != 0,
                     d = (raw & 0x20000000u) != 0;
          const Tileset* ts = findTilesetForGid(raw);
          if (!ts) continue;
          const uint32_t localId =
              clearFlipFlags(raw) - static_cast<uint32_t>(ts->firstGid);
          appendTile(mesh, ts, localId, tileToWorld(tx, ty), h, v, d);
        }
      }

      if (!mesh.chunk_buckets.empty()) {
        mesh.chunk_bucket_order.reserve(mesh.chunk_buckets.size());

        for (const auto& kv : mesh.chunk_buckets)
          mesh.chunk_bucket_order.push_back(kv.first);

        std::sort(mesh.chunk_bucket_order.begin(),
                  mesh.chunk_bucket_order.end(),
                  [](const auto& a, const auto& b) {
                    if (a.y != b.y) return a.y < b.y;
                    return a.x < b.x;
                  });

        for (const auto& key : mesh.chunk_bucket_order) {
          auto it = mesh.chunk_buckets.find(key);
          if (it == mesh.chunk_buckets.end()) continue;
          auto& chunks = it->second.chunks;
          std::stable_sort(
              chunks.begin(), chunks.end(),
              [](const LayerMesh::Chunk& a, const LayerMesh::Chunk& b) {
                return a.sortY < b.sortY;
              });
        }

        layers_.push_back(std::move(mesh));
      }

      continue;
    }

    // object layers (tile objects with gid)
    if (type == "objectgroup") {
      LayerMesh mesh;

      makeLayer(mesh);

      struct ObjDraw {
        uint32_t id = 0;
        uint32_t gid = 0;
        sf::Vector2f pos{};
        const sf::Texture* tex{};
        std::array<sf::Vertex, 6> tri{};
        float sortY = 0.f;
      };

      std::vector<ObjDraw> drawables;

      if (lj.contains("objects")) {
        for (const auto& obj : lj.at("objects")) {
          if (!obj.contains("gid")) continue;

          const uint32_t raw = obj.at("gid").get<uint32_t>();
          const bool h = (raw & 0x80000000u) != 0, v = (raw & 0x40000000u) != 0,
                     d = (raw & 0x20000000u) != 0;
          const Tileset* ts = findTilesetForGid(raw);
          if (!ts) continue;

          const uint32_t localId =
              clearFlipFlags(raw) - static_cast<uint32_t>(ts->firstGid);

          // Build UVs + texture
          sf::Vector2f uv[4];
          int tw = 0, th = 0;
          const sf::Texture* tex = nullptr;
          if (!ts->imageCollection) {
            const int cols = ts->columns;
            if (cols <= 0)
              throw std::runtime_error("Tileset '" + ts->name +
                                       "' invalid columns (<=0).");
            const int tu = static_cast<int>(localId % cols),
                      tv = static_cast<int>(localId / cols);
            tw = ts->tileWidth;
            th = ts->tileHeight;
            const int margin = ts->margin, spacing = ts->spacing;
            const float left = static_cast<float>(margin + tu * (tw + spacing));
            const float top = static_cast<float>(margin + tv * (th + spacing));
            const float right = left + tw, bottom = top + th;
            uv[0] = {left, top};
            uv[1] = {right, top};
            uv[2] = {right, bottom};
            uv[3] = {left, bottom};
            tex = ts->texture.get();
          } else {
            auto it = ts->perTile.find(static_cast<int>(localId));
            if (it == ts->perTile.end()) continue;
            const auto& pt = it->second;
            tw = pt.width;
            th = pt.height;
            uv[0] = {0, 0};
            uv[1] = {static_cast<float>(tw), 0};
            uv[2] = {static_cast<float>(tw), static_cast<float>(th)};
            uv[3] = {0, static_cast<float>(th)};
            tex = pt.texture.get();
          }
          applyFlipTexcoords(h, v, d, uv);

          // Tiled tile-objects: y is the bottom ("feet")
          const float footX = obj.value("x", 0.0f);
          const float footY = obj.value("y", 0.0f);
          const sf::Vector2f pos{footX, footY - static_cast<float>(th)};

          ObjDraw od;
          od.id = obj.at("id");
          od.gid = raw;
          od.pos = pos;
          od.tex = tex;
          od.sortY = footY;

          auto& t = od.tri;
          t[0].position = pos;
          t[0].texCoords = uv[0];
          t[1].position = {pos.x + tw, pos.y};
          t[1].texCoords = uv[1];
          t[2].position = {pos.x + tw, pos.y + th};
          t[2].texCoords = uv[2];
          t[3].position = pos;
          t[3].texCoords = uv[0];
          t[4].position = {pos.x + tw, pos.y + th};
          t[4].texCoords = uv[2];
          t[5].position = {pos.x, pos.y + th};
          t[5].texCoords = uv[3];

          const std::uint8_t a =
              static_cast<std::uint8_t>(255.f * mesh.opacity);
          for (int i = 0; i < 6; ++i) t[i].color.a = a;

          drawables.push_back(std::move(od));
        }
      }

      for (const auto& od : drawables) {
        // Create keys based on tile size
        bool visible = mesh.visible;
        for (int dy = 0;
             dy <
             (od.tri[5].position.y - od.tri[0].position.y) / tileHeight_ + 1;
             ++dy) {
          for (int dx = 0;
               dx <
               (od.tri[1].position.x - od.tri[0].position.x) / tileWidth_ + 1;
               ++dx) {
            LayerMesh::CellKey key{
                static_cast<int>(
                    std::floor((od.pos.x + dx * tileWidth_) / tileWidth_)),
                static_cast<int>(
                    std::floor((od.pos.y + dy * tileHeight_) / tileHeight_))};
            LayerMesh::Chunk ch;

            ch.id = od.id;
            ch.gid = od.gid;
            ch.texture = od.tex;
            ch.visible = visible;
            ch.opacity = mesh.opacity;
            ch.vertices.setPrimitiveType(sf::PrimitiveType::Triangles);
            ch.vertices.resize(6);
            ch.sortY = od.sortY;

            for (int i = 0; i < 6; ++i) {
              ch.vertices[i] = od.tri[i];
            }

            mesh.chunk_buckets[key].chunks.push_back(std::move(ch));

            // only the first chunk is visible to not render the same object
            // multiple times
            visible = false;
          }
        }
      }

      if (!mesh.chunk_buckets.empty()) {
        mesh.chunk_bucket_order.reserve(mesh.chunk_buckets.size());

        for (const auto& kv : mesh.chunk_buckets) {
          mesh.chunk_bucket_order.push_back(kv.first);
        }

        std::sort(mesh.chunk_bucket_order.begin(),
                  mesh.chunk_bucket_order.end(),
                  [](const auto& a, const auto& b) {
                    if (a.y != b.y) {
                      return a.y < b.y;
                    }

                    return a.x < b.x;
                  });

        for (const auto& key : mesh.chunk_bucket_order) {
          auto it = mesh.chunk_buckets.find(key);

          if (it == mesh.chunk_buckets.end()) {
            continue;
          }

          auto& chunks = it->second.chunks;
          std::stable_sort(
              chunks.begin(), chunks.end(),
              [](const LayerMesh::Chunk& a, const LayerMesh::Chunk& b) {
                return a.sortY < b.sortY;
              });
        }

        // Build global draw order for object layers: gather pointers to all
        // chunks and sort by chunk->sortY. Reserve exact size for efficiency.
        size_t totalChunks = 0;
        for (const auto& kv : mesh.chunk_buckets)
          totalChunks += kv.second.chunks.size();
        mesh.object_draw_order.clear();
        mesh.object_draw_order.reserve(totalChunks);
        for (auto& kv : mesh.chunk_buckets) {
          for (auto& c : kv.second.chunks) mesh.object_draw_order.push_back(&c);
        }
        std::stable_sort(
            mesh.object_draw_order.begin(), mesh.object_draw_order.end(),
            [](const LayerMesh::Chunk* a, const LayerMesh::Chunk* b) {
              if (a->sortY != b->sortY) return a->sortY < b->sortY;
              // tie-breaker: by x then id to stabilize ordering
              const float ax = a->vertices.getVertexCount()
                                   ? a->vertices[0].position.x
                                   : 0.f;
              const float bx = b->vertices.getVertexCount()
                                   ? b->vertices[0].position.x
                                   : 0.f;
              if (ax != bx) return ax < bx;
              return a->id < b->id;
            });

        layers_.push_back(std::move(mesh));
      }

      continue;
    }
  }

  // Build fast lookup index for object layers
  buildObjectIndex();
}

void WorldMap::buildObjectIndex() {
  object_index_.clear();
  for (size_t li = 0; li < layers_.size(); ++li) {
    const auto& layer = layers_[li];
    if (layer.type != "objectgroup") continue;
    for (const auto& kv : layer.chunk_buckets) {
      const LayerMesh::CellKey key = kv.first;
      for (const auto& c : kv.second.chunks) {
        object_index_[static_cast<int>(c.id)].push_back(
            {static_cast<int>(li), key});
      }
    }
  }
}

int WorldMap::getObjectIdAtPosition(const sf::Vector2f& worldPos) const {
  for (auto it = layers_.rbegin(); it != layers_.rend(); ++it) {
    const LayerMesh& mesh = *it;

    if (mesh.type != "objectgroup") {
      continue;
    }

    LayerMesh::CellKey key{
        static_cast<int>(std::floor(worldPos.x / tileWidth_)),
        static_cast<int>(std::floor(worldPos.y / tileHeight_))};

    std::cout << "getObjectIdAtPosition: worldPos=(" << worldPos.x << ","
              << worldPos.y << ") tileW=" << tileWidth_
              << " tileH=" << tileHeight_ << " key=(" << key.x << "," << key.y
              << ")\n";

    auto bit = mesh.chunk_buckets.find(key);

    if (bit == mesh.chunk_buckets.end()) {
      std::cout << "getObjectIdAtPosition: no bucket for key\n";
      continue;
    }

    const auto& bucket = bit->second;

    std::cout << "getObjectIdAtPosition: found bucket with "
              << bucket.chunks.size() << " chunks\n";

    if (bucket.chunks.empty()) {
      continue;
    }

    for (const auto& chunk : bucket.chunks) {
      std::cout << "  chunk id=" << chunk.id << " gid=" << chunk.gid
                << " vertCount=" << chunk.vertices.getVertexCount()
                << " visible=" << chunk.visible << "\n";
      if (chunk.vertices.getVertexCount() >= 1) {
        std::cout << "    vertex0=(" << chunk.vertices[0].position.x << ","
                  << chunk.vertices[0].position.y << ")\n";
      }

      if (chunk.vertices.getVertexCount() < 6) {
        continue;
      }

      const sf::FloatRect bbox = chunk.vertices.getBounds();
      float bx = 0.f, by = 0.f, bw = 0.f, bh = 0.f;
      if (chunk.vertices.getVertexCount() >= 1) {
        float minx = chunk.vertices[0].position.x;
        float miny = chunk.vertices[0].position.y;
        float maxx = minx;
        float maxy = miny;
        for (size_t vi = 1; vi < chunk.vertices.getVertexCount(); ++vi) {
          const auto& p = chunk.vertices[vi].position;
          if (p.x < minx) minx = p.x;
          if (p.y < miny) miny = p.y;
          if (p.x > maxx) maxx = p.x;
          if (p.y > maxy) maxy = p.y;
        }
        bx = minx;
        by = miny;
        bw = maxx - minx;
        bh = maxy - miny;
      }
      std::cout << "    bbox=(l=" << bx << ", t=" << by << ", w=" << bw
                << ", h=" << bh << ")\n";

      if (bbox.contains(worldPos)) {
        std::cout << "    worldPos inside bbox\n";
        const WorldMap::Tileset* tileset = findTilesetForGid(chunk.gid);

        if (!tileset) {
          std::cout << "    no tileset for gid=" << chunk.gid << "\n";
          continue;
        }

        const int localId = chunk.gid - tileset->firstGid;
        if (tileset->objectGroups.find(localId) ==
            tileset->objectGroups.end()) {
          std::cout << "    tileset has no object group for localId=" << localId
                    << "\n";
          continue;
        }

        for (const auto& obj : tileset->objectGroups.at(localId).objects) {
          if (obj.type != "clickable") {
            continue;
          }

          sf::ConvexShape polygon{
              static_cast<unsigned int>(obj.polygon.size())};

          for (size_t i = 0; i < obj.polygon.size(); ++i) {
            polygon.setPoint(
                i, {obj.x + obj.polygon[i].x, obj.y + obj.polygon[i].y});
          }

          polygon.setPosition(chunk.vertices[0].position);

          std::cout << "    testing polygon at chunk origin ("
                    << chunk.vertices[0].position.x << ","
                    << chunk.vertices[0].position.y << ")\n";

          if (!polygon.getGlobalBounds().contains(worldPos)) {
            std::cout << "    worldPos not in polygon bbox\n";
            continue;
          }

          // More precise point-in-polygon test
          if (polygon.getPointCount() >= 3) {
            // Ray-casting algorithm
            bool inside = false;
            for (size_t i = 0, j = polygon.getPointCount() - 1;
                 i < polygon.getPointCount(); j = i++) {
              sf::Vector2f pi = polygon.getPoint(i) + polygon.getPosition();
              sf::Vector2f pj = polygon.getPoint(j) + polygon.getPosition();
              if (((pi.y > worldPos.y) != (pj.y > worldPos.y)) &&
                  (worldPos.x <
                   (pj.x - pi.x) * (worldPos.y - pi.y) / (pj.y - pi.y) +
                       pi.x)) {
                inside = !inside;
              }
            }

            if (inside) {
              std::cout << "    point-in-polygon: inside -> returning id="
                        << chunk.id << "\n";
              return static_cast<int>(chunk.id);
            } else {
              std::cout << "    point-in-polygon: not inside\n";
            }
          }
        }
      }
    }
  }

  return -1;
}

bool WorldMap::updateObject(int objectId, const nlohmann::json& props,
                            std::vector<int>* outAffectedLayers) {
  bool changed = false;

  if (outAffectedLayers) outAffectedLayers->clear();

  // Supported props: gid (number), visible (bool), opacity (number)
  const bool hasGid = props.contains("gid") && props["gid"].is_number();
  const bool hasVisible =
      props.contains("visible") && props["visible"].is_boolean();
  const bool hasOpacity =
      props.contains("opacity") &&
      (props["opacity"].is_number() || props["opacity"].is_number_float());
  const bool hasPos =
      props.contains("pos") && props["pos"].is_object() &&
      props["pos"].contains("x") && props["pos"].contains("y") &&
      (props["pos"]["x"].is_number() || props["pos"]["x"].is_number_float()) &&
      (props["pos"]["y"].is_number() || props["pos"]["y"].is_number_float());

  uint32_t newGid = 0;
  if (hasGid) newGid = props.value("gid", 0u);
  bool newVisible = false;
  float newOpacity = 1.0f;
  if (hasVisible) newVisible = props.value("visible", true);
  if (hasOpacity) newOpacity = props.value("opacity", 1.0f);

  // Helper to mark affected layer index
  auto markLayer = [&](int li) {
    if (outAffectedLayers) outAffectedLayers->push_back(li);
  };

  // Ensure object_index_ is up-to-date and contains entries for this
  // object. We do NOT fallback to scanning buckets; instead we rebuild the
  // index if necessary and fail-fast when the object is still not present.
  auto itIndex = object_index_.find(objectId);
  if (itIndex == object_index_.end()) {
    buildObjectIndex();
    itIndex = object_index_.find(objectId);
  }
  if (itIndex == object_index_.end()) {
    throw std::runtime_error("Object index missing entries for objectId: " +
                             std::to_string(objectId));
  }

  // Update gid/visible/opacity using indexed keys only
  for (const auto& loc : itIndex->second) {
    const int li = loc.first;
    const LayerMesh::CellKey key = loc.second;
    if (li < 0 || li >= static_cast<int>(layers_.size())) continue;
    auto& mesh = layers_[li];
    if (mesh.type != "objectgroup") continue;

    auto bit = mesh.chunk_buckets.find(key);
    if (bit == mesh.chunk_buckets.end()) continue;
    auto& bucket = bit->second;

    for (size_t ci = 0; ci < bucket.chunks.size(); ++ci) {
      auto& chunk = bucket.chunks[ci];
      if (static_cast<int>(chunk.id) != objectId) continue;

      bool thisChanged = false;

      if (hasVisible) {
        if (chunk.visible != newVisible) {
          chunk.visible = newVisible;
          thisChanged = true;
        }
      }

      if (hasOpacity) {
        if (std::abs(chunk.opacity - newOpacity) > 1e-6f) {
          chunk.opacity = newOpacity;
          const std::uint8_t a =
              static_cast<std::uint8_t>(255.f * chunk.opacity);
          for (size_t vi = 0; vi < chunk.vertices.getVertexCount(); ++vi) {
            chunk.vertices[vi].color.a = a;
          }
          thisChanged = true;
        }
      }

      if (hasGid) {
        if (chunk.gid != newGid) {
          const bool h = (newGid & 0x80000000u) != 0;
          const bool v = (newGid & 0x40000000u) != 0;
          const bool d = (newGid & 0x20000000u) != 0;

          const Tileset* ts = findTilesetForGid(newGid);
          if (ts) {
            chunk.gid = newGid;

            const size_t vertCount = chunk.vertices.getVertexCount();
            if (vertCount >= 6) {
              const uint32_t cleared = clearFlipFlags(newGid);
              const uint32_t localId =
                  cleared - static_cast<uint32_t>(ts->firstGid);
              sf::Vector2f uv[4];
              int tw = 0, th = 0;
              const sf::Texture* tex = nullptr;

              if (!ts->imageCollection) {
                const int cols = ts->columns;
                if (cols > 0) {
                  const int tu = static_cast<int>(localId % cols);
                  const int tv = static_cast<int>(localId / cols);
                  tw = ts->tileWidth;
                  th = ts->tileHeight;
                  const int margin = ts->margin, spacing = ts->spacing;
                  const float left =
                      static_cast<float>(margin + tu * (tw + spacing));
                  const float top =
                      static_cast<float>(margin + tv * (th + spacing));
                  const float right = left + tw;
                  const float bottom = top + th;
                  uv[0] = {left, top};
                  uv[1] = {right, top};
                  uv[2] = {right, bottom};
                  uv[3] = {left, bottom};
                  tex = ts->texture.get();
                }
              } else {
                auto it = ts->perTile.find(static_cast<int>(localId));
                if (it != ts->perTile.end()) {
                  const auto& pt = it->second;
                  tw = pt.width;
                  th = pt.height;
                  uv[0] = {0.f, 0.f};
                  uv[1] = {static_cast<float>(tw), 0.f};
                  uv[2] = {static_cast<float>(tw), static_cast<float>(th)};
                  uv[3] = {0.f, static_cast<float>(th)};
                  tex = pt.texture.get();
                }
              }

              if (tex) {
                chunk.texture = tex;

                WorldMap::applyFlipTexcoords(h, v, d, uv);

                if (tw > 0 && th > 0) {
                  chunk.vertices[0].texCoords = uv[0];
                  chunk.vertices[1].texCoords = uv[1];
                  chunk.vertices[2].texCoords = uv[2];
                  chunk.vertices[3].texCoords = uv[0];
                  chunk.vertices[4].texCoords = uv[2];
                  chunk.vertices[5].texCoords = uv[3];

                  thisChanged = true;
                }
              }
            }
          }
        }
      }

      if (thisChanged) {
        changed = true;
        markLayer(li);
      }
    }
  }

  // Position moves: use index-only approach. Rebuild index first to ensure
  // entries are current, then move chunks using exact keys from the index.
  if (hasPos) {
    // Rebuild index to ensure consistency before moving
    buildObjectIndex();
    auto itIdx = object_index_.find(objectId);
    if (itIdx == object_index_.end()) {
      throw std::runtime_error("Object index missing entries for objectId: " +
                               std::to_string(objectId));
    }

    const float newX = props["pos"].value("x", 0.f);
    const float newY = props["pos"].value("y", 0.f);

    // Gather distinct layers from index
    std::vector<int> layersToProcess;
    for (const auto& p : itIdx->second) layersToProcess.push_back(p.first);
    std::sort(layersToProcess.begin(), layersToProcess.end());
    layersToProcess.erase(
        std::unique(layersToProcess.begin(), layersToProcess.end()),
        layersToProcess.end());

    for (int li : layersToProcess) {
      if (li < 0 || li >= static_cast<int>(layers_.size())) continue;
      auto& meshRef = layers_[li];

      // Remove existing chunks by exact keys from the index
      std::vector<std::pair<LayerMesh::CellKey, LayerMesh::Chunk>> __pos_moved;
      for (const auto& p : itIdx->second) {
        if (p.first != li) continue;
        const LayerMesh::CellKey key = p.second;
        auto bit = meshRef.chunk_buckets.find(key);
        if (bit == meshRef.chunk_buckets.end()) continue;
        auto& bucket = bit->second;
        auto cit = std::find_if(bucket.chunks.begin(), bucket.chunks.end(),
                                [&](const LayerMesh::Chunk& c) {
                                  return static_cast<int>(c.id) == objectId;
                                });
        if (cit != bucket.chunks.end()) {
          __pos_moved.push_back({key, *cit});
          bucket.chunks.erase(cit);
        }
      }

      // Create moved chunks in new buckets and update index incrementally
      std::vector<std::pair<LayerMesh::CellKey, LayerMesh::Chunk>> __pos_moves;
      for (auto& [oldKey, chunk] : __pos_moved) {
        if (chunk.visible == false) continue;
        bool visible = true;
        const sf::Vector2f pos{
            newX, newY - static_cast<float>(chunk.vertices[5].position.y -
                                            chunk.vertices[0].position.y)};

        for (int dy = 0; dy < (chunk.vertices[5].position.y -
                               chunk.vertices[0].position.y) /
                                      tileHeight_ +
                                  1;
             ++dy) {
          for (int dx = 0; dx < (chunk.vertices[1].position.x -
                                 chunk.vertices[0].position.x) /
                                        tileWidth_ +
                                    1;
               ++dx) {
            LayerMesh::CellKey newKey{
                static_cast<int>(
                    std::floor((pos.x + dx * tileWidth_) / tileWidth_)),
                static_cast<int>(
                    std::floor((pos.y + dy * tileHeight_) / tileHeight_))};
            chunk.visible = visible;
            __pos_moves.push_back({newKey, chunk});
            visible = false;
          }
        }
      }

      if (!__pos_moves.empty()) {
        changed = true;
        markLayer(li);
      }

      // Erase old index entries for this layer before inserting new ones
      object_index_[objectId].erase(
          std::remove_if(object_index_[objectId].begin(),
                         object_index_[objectId].end(),
                         [&](const std::pair<int, LayerMesh::CellKey>& p) {
                           return p.first == li;
                         }),
          object_index_[objectId].end());

      for (auto& [newKey, chunk] : __pos_moves) {
        const sf::Vector2f pos{
            newX, newY - static_cast<float>(chunk.vertices[5].position.y -
                                            chunk.vertices[0].position.y)};
        const int tw = static_cast<int>(chunk.vertices[1].position.x -
                                        chunk.vertices[0].position.x);
        const int th = static_cast<int>(chunk.vertices[5].position.y -
                                        chunk.vertices[0].position.y);

        chunk.vertices[0].position = pos;
        chunk.vertices[1].position = {pos.x + tw, pos.y};
        chunk.vertices[2].position = {pos.x + tw, pos.y + th};
        chunk.vertices[3].position = pos;
        chunk.vertices[4].position = {pos.x + tw, pos.y + th};
        chunk.vertices[5].position = {pos.x, pos.y + th};

        meshRef.chunk_buckets[newKey].chunks.push_back(chunk);
        object_index_[objectId].push_back({li, newKey});
      }

      // Rebuild draw order for this layer
      rebuildObjectDrawOrderForLayer(li);
    }
  }

  // De-duplicate layer indices
  if (outAffectedLayers) {
    std::sort(outAffectedLayers->begin(), outAffectedLayers->end());
    outAffectedLayers->erase(
        std::unique(outAffectedLayers->begin(), outAffectedLayers->end()),
        outAffectedLayers->end());
  }

  return changed;
}

void WorldMap::rebuildObjectDrawOrderForLayer(int layerIndex) {
  if (layerIndex < 0 || layerIndex >= static_cast<int>(layers_.size())) return;
  auto& layer = layers_[layerIndex];
  if (layer.type != "objectgroup") return;

  size_t totalChunks = 0;
  for (const auto& kv : layer.chunk_buckets)
    totalChunks += kv.second.chunks.size();
  layer.object_draw_order.clear();
  layer.object_draw_order.reserve(totalChunks);
  for (auto& kv : layer.chunk_buckets) {
    for (auto& c : kv.second.chunks) layer.object_draw_order.push_back(&c);
  }
  std::stable_sort(
      layer.object_draw_order.begin(), layer.object_draw_order.end(),
      [](const WorldMap::LayerMesh::Chunk* a,
         const WorldMap::LayerMesh::Chunk* b) {
        if (a->sortY != b->sortY) return a->sortY < b->sortY;
        const float ax =
            a->vertices.getVertexCount() ? a->vertices[0].position.x : 0.f;
        const float bx =
            b->vertices.getVertexCount() ? b->vertices[0].position.x : 0.f;
        if (ax != bx) return ax < bx;
        return a->id < b->id;
      });
}

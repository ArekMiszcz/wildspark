// Copyright 2025 WildSpark Authors

#include "WorldMap.h"

#include <algorithm>
#include <cstdio>
#include <filesystem>
#include <fstream>
#include <limits>
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
        std::vector<LayerMesh::CellKey> keys;
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

    auto bit = mesh.chunk_buckets.find(key);

    if (bit == mesh.chunk_buckets.end()) {
      continue;
    }

    const auto& bucket = bit->second;

    if (bucket.chunks.empty()) {
      continue;
    }

    for (const auto& chunk : bucket.chunks) {
      if (chunk.vertices.getVertexCount() < 6) {
        continue;
      }

      const sf::FloatRect bbox = chunk.vertices.getBounds();

      if (bbox.contains(worldPos)) {
        const WorldMap::Tileset* tileset = findTilesetForGid(chunk.gid);

        if (!tileset) {
          continue;
        }

        const int localId = chunk.gid - tileset->firstGid;
        if (tileset->objectGroups.find(localId) ==
            tileset->objectGroups.end()) {
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

          if (!polygon.getGlobalBounds().contains(worldPos)) {
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
              return static_cast<int>(chunk.id);
            }
          }
        }
      }
    }
  }

  return -1;
}

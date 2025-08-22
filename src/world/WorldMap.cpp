// Copyright 2025 WildSpark Authors

#include "WorldMap.h"

#include <algorithm>
#include <filesystem>
#include <fstream>
#include <memory>
#include <stdexcept>
#include <string>
#include <unordered_map>
#include <utility>
#include <vector>

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
  fs::path mapPath = mapPathStr;
  fs::path mapDir = mapPath.parent_path();
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
      for (auto& c : mesh.chunks)
        if (c.texture == tex) {
          chunk = &c;
          break;
        }
      if (!chunk) {
        mesh.chunks.push_back(LayerMesh::Chunk{});
        chunk = &mesh.chunks.back();
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
          const uint32_t localId = clearFlipFlags(raw) - static_cast<uint32_t>(ts->firstGid);
          appendTile(mesh, ts, localId, tileToWorld(tx, ty), h, v, d);
        }
      }

      if (!mesh.chunks.empty()) layers_.push_back(std::move(mesh));
      continue;
    }

    // object layers (tile objects with gid)
    if (type == "objectgroup") {
      LayerMesh mesh;
      makeLayer(mesh);

      struct ObjDraw {
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

          const uint32_t localId = clearFlipFlags(raw) - static_cast<uint32_t>(ts->firstGid);

          // Build UVs + texture
          sf::Vector2f uv[4];
          int tw = 0, th = 0;
          const sf::Texture* tex = nullptr;
          if (!ts->imageCollection) {
            const int cols = ts->columns;
            if (cols <= 0)
              throw std::runtime_error("Tileset '" + ts->name +
                                       "' invalid columns (<=0).");
            const int tu = static_cast<int>(localId % cols), tv = static_cast<int>(localId / cols);
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

      // Sort by Y (top → bottom). Lower on screen draws later.
      std::sort(
          drawables.begin(), drawables.end(),
          [](const ObjDraw& a, const ObjDraw& b) { return a.sortY < b.sortY; });

      // ONE chunk per object (preserve order across different textures)
      for (const auto& od : drawables) {
        LayerMesh::Chunk ch;
        ch.texture = od.tex;
        ch.visible = mesh.visible;
        ch.opacity = mesh.opacity;
        ch.vertices.setPrimitiveType(sf::PrimitiveType::Triangles);
        ch.vertices.resize(6);
        for (int i = 0; i < 6; ++i) ch.vertices[i] = od.tri[i];
        mesh.chunks.push_back(std::move(ch));
      }

      if (!mesh.chunks.empty()) layers_.push_back(std::move(mesh));
      continue;
    }
  }
}

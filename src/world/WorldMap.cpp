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

static std::string readFile(const std::filesystem::path& p) {
  std::ifstream ifs(p, std::ios::binary);
  if (!ifs) throw std::runtime_error("Failed to open file: " + p.string());
  std::string s;
  ifs.seekg(0, std::ios::end);
  s.resize(static_cast<size_t>(ifs.tellg()));
  ifs.seekg(0, std::ios::beg);
  ifs.read(s.data(), static_cast<std::streamsize>(s.size()));
  return s;
}

WorldMap::WorldMap(const std::string& mapJsonPath) {
  loadFromJson(mapJsonPath);
}

void WorldMap::loadFromJson(const std::string& pathStr) {
  std::filesystem::path path = pathStr;
  std::filesystem::path mapDir = path.parent_path();
  auto j = nlohmann::json::parse(readFile(path));

  // enforce orthogonal only
  const std::string orient = j.value("orientation", "orthogonal");
  if (orient != "orthogonal")
    throw std::runtime_error(
        "Only 'orthogonal' orientation is supported. Current: " + orient);

  mapWidth_ = j.at("width").get<int>();
  mapHeight_ = j.at("height").get<int>();
  tileWidth_ = j.at("tilewidth").get<int>();
  tileHeight_ = j.at("tileheight").get<int>();

  // tilesets
  for (const auto& tsj : j.at("tilesets")) {
    if (tsj.contains("source"))
      loadTilesetExternal(mapDir.string(), tsj.at("source").get<std::string>(),
                          tsj.at("firstgid").get<int>());
    else
      loadTilesetInline(mapDir.string(), tsj);
  }
  std::sort(tilesets_.begin(), tilesets_.end(),
            [](const Tileset& a, const Tileset& b) {
              return a.firstGid < b.firstGid;
            });

  // layers
  buildLayers(j);
}

void WorldMap::loadTilesetInline(const std::string& mapDir, const json& tsj) {
  Tileset ts;
  ts.firstGid = tsj.at("firstgid").get<int>();
  ts.name = tsj.value("name", "");
  ts.tileWidth = tsj.at("tilewidth").get<int>();
  ts.tileHeight = tsj.at("tileheight").get<int>();
  ts.margin = tsj.value("margin", 0);
  ts.spacing = tsj.value("spacing", 0);
  ts.columns = tsj.value("columns", 0);

  const auto& img = tsj.at("image");
  std::filesystem::path imgPath =
      std::filesystem::path(mapDir) / img.get<std::string>();
  ts.imagePath = imgPath.string();
  ts.imageWidth = tsj.at("imagewidth").get<int>();
  ts.imageHeight = tsj.at("imageheight").get<int>();

  ts.texture = std::make_shared<sf::Texture>();
  if (!ts.texture->loadFromFile(ts.imagePath))
    throw std::runtime_error("Failed to load tileset image: " + ts.imagePath);
  ts.texture->setRepeated(false);
  ts.texture->setSmooth(false);

  if (ts.columns == 0 && ts.tileWidth > 0) {
    ts.columns = std::max(1, (ts.imageWidth - 2 * ts.margin + ts.spacing) /
                                 (ts.tileWidth + ts.spacing));
  }

  tilesets_.push_back(std::move(ts));
}

void WorldMap::loadTilesetExternal(const std::string& mapDir,
                                   const std::string& source, int firstGid) {
  std::filesystem::path src = std::filesystem::path(mapDir) / source;
  auto ext = src.extension().string();
  if (ext == ".tsx")
    throw std::runtime_error(
        "External TSX (XML) not supported. Export tileset as JSON (.tsj/.json) "
        "or embed.");

  auto tj = nlohmann::json::parse(readFile(src));

  Tileset ts;
  ts.firstGid = firstGid;
  ts.name = tj.value("name", "");
  ts.tileWidth = tj.at("tilewidth").get<int>();
  ts.tileHeight = tj.at("tileheight").get<int>();
  ts.margin = tj.value("margin", 0);
  ts.spacing = tj.value("spacing", 0);
  ts.columns = tj.value("columns", 0);

  // const auto& img = tj.at("image");
  // std::filesystem::path imgPath = src.parent_path() / img.get<std::string>();
  // ts.imagePath = (src.parent_path() / img.get<std::string>()).string();
  // ts.imageWidth = tj.at("imagewidth").get<int>();
  // ts.imageHeight = tj.at("imageheight").get<int>();

  // ts.texture = std::make_shared<sf::Texture>();
  // if (!ts.texture->loadFromFile(ts.imagePath))
  //   throw std::runtime_error("Failed to load tileset image: " +
  //   ts.imagePath);
  // ts.texture->setRepeated(false);
  // ts.texture->setSmooth(false);

  if (ts.columns == 0 && ts.tileWidth > 0) {
    ts.columns = std::max(1, (ts.imageWidth - 2 * ts.margin + ts.spacing) /
                                 (ts.tileWidth + ts.spacing));
  }

  tilesets_.push_back(std::move(ts));
}

// ===== Triangles helpers (6 vertices per tile) =====
static inline void trisForTile(sf::Vertex v[6], const sf::Vector2f& pos, int tw,
                               int th) {
  const sf::Vector2f tl = pos;
  const sf::Vector2f tr = {pos.x + static_cast<float>(tw), pos.y};
  const sf::Vector2f br = {pos.x + static_cast<float>(tw),
                           pos.y + static_cast<float>(th)};
  const sf::Vector2f bl = {pos.x, pos.y + static_cast<float>(th)};

  v[0].position = tl;
  v[1].position = tr;
  v[2].position = br;  // tri #1
  v[3].position = tl;
  v[4].position = br;
  v[5].position = bl;  // tri #2
}

static inline void setTriTex(sf::Vertex v[6], const sf::Vector2f tc[4]) {
  // tc order: tl, tr, br, bl (after flips)
  v[0].texCoords = tc[0];
  v[1].texCoords = tc[1];
  v[2].texCoords = tc[2];
  v[3].texCoords = tc[0];
  v[4].texCoords = tc[2];
  v[5].texCoords = tc[3];
}

const Tileset* WorldMap::findTilesetForGid(uint32_t gid) const {
  if (gid == 0) return nullptr;
  const uint32_t id = clearFlipFlags(gid);
  const Tileset* best = nullptr;
  for (const auto& ts : tilesets_) {
    if (static_cast<uint32_t>(ts.firstGid) <= id)
      best = &ts;
    else
      break;
  }
  return best;
}

void WorldMap::applyFlipTexcoords(bool flipH, bool flipV, bool flipD,
                                  sf::Vector2f tc[4]) {
  if (flipH) {
    std::swap(tc[0], tc[1]);
    std::swap(tc[3], tc[2]);
  }
  if (flipV) {
    std::swap(tc[0], tc[3]);
    std::swap(tc[1], tc[2]);
  }
  if (flipD) {
    std::swap(tc[1], tc[3]);
  }
}

void WorldMap::buildLayers(const json& j) {
  const auto& layers = j.at("layers");
  layers_.clear();
  layers_.reserve(layers.size());

  for (const auto& lj : layers) {
    if (lj.at("type").get<std::string>() != "tilelayer") {
      continue;
    }

    LayerMesh mesh;
    std::unordered_map<const Tileset*, size_t> chunkIndex;
    std::vector<uint32_t> data;

    mesh.name = lj.value("name", "");
    mesh.visible = lj.value("visible", true);
    mesh.opacity = lj.value("opacity", 1.0f);

    if (lj.contains("data") && lj.at("data").is_array()) {
      data = lj.at("data").get<std::vector<uint32_t>>();

      if (static_cast<int>(data.size()) != mapWidth_ * mapHeight_) {
        throw std::runtime_error("Tile layer data size mismatch");
      }
    } else if (lj.contains("chunks")) {
      throw std::runtime_error(
          "Chunked (infinite) maps not supported. Disable 'Infinite' in "
          "Tiled.");
    } else {
      continue;
    }

    auto ensureChunk = [&](const Tileset* ts) -> LayerMesh::Chunk& {
      auto it = chunkIndex.find(ts);

      if (it == chunkIndex.end()) {
        mesh.chunks.push_back(LayerMesh::Chunk{});
        mesh.chunks.back().ts = ts;
        mesh.chunks.back().visible = mesh.visible;
        mesh.chunks.back().opacity = mesh.opacity;
        mesh.chunks.back().vertices.setPrimitiveType(
            sf::PrimitiveType::Triangles);
        chunkIndex[ts] = mesh.chunks.size() - 1;

        return mesh.chunks.back();
      }

      return mesh.chunks[it->second];
    };

    for (int ty = 0; ty < mapHeight_; ++ty) {
      for (int tx = 0; tx < mapWidth_; ++tx) {
        const uint32_t raw = data[ty * mapWidth_ + tx];
        if (raw == 0) continue;

        bool flipH = (raw & 0x8000'0000u) != 0;
        bool flipV = (raw & 0x4000'0000u) != 0;
        bool flipD = (raw & 0x2000'0000u) != 0;

        const Tileset* ts = findTilesetForGid(raw);
        if (!ts) continue;

        const uint32_t gid = clearFlipFlags(raw);
        const uint32_t localId = gid - static_cast<uint32_t>(ts->firstGid);

        const int cols = ts->columns;
        const int tw = ts->tileWidth, th = ts->tileHeight;
        const int margin = ts->margin, spacing = ts->spacing;

        const int tu = static_cast<int>(localId % cols);
        const int tv = static_cast<int>(localId / cols);

        sf::Vector2f tex[4];
        const float left = static_cast<float>(margin + tu * (tw + spacing));
        const float top = static_cast<float>(margin + tv * (th + spacing));
        const float right = left + tw;
        const float bottom = top + th;
        tex[0] = {left, top};      // tl
        tex[1] = {right, top};     // tr
        tex[2] = {right, bottom};  // br
        tex[3] = {left, bottom};   // bl
        applyFlipTexcoords(flipH, flipV, flipD, tex);

        auto& chunk = ensureChunk(ts);
        const sf::Vector2f pos = tileToWorld(tx, ty);

        const size_t base = chunk.vertices.getVertexCount();
        chunk.vertices.resize(base + 6);
        sf::Vertex* t = &chunk.vertices[base];

        trisForTile(t, pos, tw, th);
        setTriTex(t, tex);

        const uint8_t a = static_cast<uint8_t>(255.f * mesh.opacity);
        for (int i = 0; i < 6; ++i) t[i].color.a = a;
      }
    }

    mesh.chunks.erase(std::remove_if(mesh.chunks.begin(), mesh.chunks.end(),
                                     [](const LayerMesh::Chunk& c) {
                                       return c.vertices.getVertexCount() == 0;
                                     }),
                      mesh.chunks.end());

    if (!mesh.chunks.empty()) {
      layers_.push_back(std::move(mesh));
    }
  }
}

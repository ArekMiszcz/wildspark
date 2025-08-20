// Copyright 2025 WildSpark Authors

#ifndef WORLD_WORLDMAP_H_
#define WORLD_WORLDMAP_H_

#include <memory>
#include <string>
#include <unordered_map>
#include <vector>

#include <SFML/Graphics.hpp>

#include <nlohmann/json.hpp>

struct Tileset {
  int firstGid = 0;
  int tileWidth = 0;
  int tileHeight = 0;
  int imageWidth = 0;
  int imageHeight = 0;
  int margin = 0;
  int spacing = 0;
  int columns = 0;
  std::string name;
  std::string imagePath;
  std::shared_ptr<sf::Texture> texture;
};

struct LayerMesh {
  struct Chunk {
    const Tileset* ts = nullptr;
    sf::VertexArray vertices;
    float opacity = 1.f;
    bool visible = true;

    Chunk() : vertices(sf::PrimitiveType::Triangles) {}
  };

  std::string name;
  std::vector<Chunk> chunks;
  bool visible = true;
  float opacity = 1.f;
};

class WorldMap {
 public:
  explicit WorldMap(const std::string& mapJsonPath);

  // Basic map info (tiles and pixels)
  int width() const { return mapWidth_; }       // tiles
  int height() const { return mapHeight_; }     // tiles
  int tileWidth() const { return tileWidth_; }  // px
  int tileHeight() const { return tileHeight_; }

  const std::vector<LayerMesh>& layers() const { return layers_; }

  // tile -> world top-left (orthogonal)
  inline sf::Vector2f tileToWorld(int tx, int ty) const {
    return {static_cast<float>(tx * tileWidth_),
            static_cast<float>(ty * tileHeight_)};
  }

  sf::FloatRect worldBounds() const {
    return sf::FloatRect(
        sf::Vector2f(0.f, 0.f),
        sf::Vector2f(static_cast<float>(mapWidth_ * tileWidth_),
                     static_cast<float>(mapHeight_ * tileHeight_)));
  }

 private:
  void loadFromJson(const std::string& path);
  void loadTilesetInline(const std::string& mapDir, const nlohmann::json& tsj);
  void loadTilesetExternal(const std::string& mapDir, const std::string& source,
                           int firstGid);
  void buildLayers(const nlohmann::json& map);

  const Tileset* findTilesetForGid(uint32_t gid) const;
  static inline uint32_t clearFlipFlags(uint32_t gid) {
    return gid & 0x1FFFFFFF;
  }
  static void applyFlipTexcoords(bool flipH, bool flipV, bool flipD,
                                 sf::Vector2f tc[4]);

 private:
  int mapWidth_ = 0, mapHeight_ = 0;
  int tileWidth_ = 0, tileHeight_ = 0;
  std::vector<LayerMesh> layers_;
  std::vector<Tileset> tilesets_;
};

#endif  // WORLD_WORLDMAP_H_

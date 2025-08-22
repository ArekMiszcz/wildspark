// Copyright 2025 WildSpark Authors

#ifndef WORLD_WORLDMAP_H_
#define WORLD_WORLDMAP_H_

#include <memory>
#include <string>
#include <unordered_map>
#include <vector>

#include <SFML/Graphics.hpp>
#include <nlohmann/json.hpp>

class WorldMap {
 public:
  explicit WorldMap(const std::string& mapJsonPath);

  struct Tileset {
    struct PerTile {  // for collection-of-images
      int localId = 0;
      std::shared_ptr<sf::Texture> texture;
      int width = 0, height = 0;
    };

    int firstGid = 0;
    std::string name;

    // common
    int tileWidth = 0, tileHeight = 0;
    int margin = 0, spacing = 0;
    int columns = 0;

    // spritesheet fields
    bool imageCollection = false;
    std::string imagePath;
    int imageWidth = 0, imageHeight = 0;
    std::shared_ptr<sf::Texture> texture;  // spritesheet

    // per-tile images
    std::unordered_map<int, PerTile> perTile;  // localId -> data
  };

  struct LayerMesh {
    struct Chunk {
      const sf::Texture* texture = nullptr;
      sf::VertexArray vertices;  // Triangles
      float opacity = 1.f;
      bool visible = true;
      Chunk() : vertices(sf::PrimitiveType::Triangles) {}
    };
    std::string name;
    std::vector<Chunk> chunks;
    bool visible = true;
    float opacity = 1.f;
  };

  // map info (orthogonal only)
  int width() const { return mapWidth_; }
  int height() const { return mapHeight_; }
  int tileWidth() const { return tileWidth_; }
  int tileHeight() const { return tileHeight_; }
  const std::vector<LayerMesh>& layers() const { return layers_; }
  sf::Vector2f tileToWorld(int tx, int ty) const {
    return {static_cast<float>(tx * tileWidth_), static_cast<float>(ty * tileHeight_)};
  }
  sf::FloatRect worldBounds() const {
    return {{0.f, 0.f},
            {static_cast<float>(mapWidth_ * tileWidth_), static_cast<float>(mapHeight_ * tileHeight_)}};
  }

 private:
  // loading
  void loadFromJson(const std::string& mapPath);
  void loadTilesetInline(const std::filesystem::path& mapDir,
                         const nlohmann::json& tsj);
  void loadTilesetExternal(const std::filesystem::path& mapDir,
                           const std::string& source, int firstGid);
  void buildLayers(const nlohmann::json& map);

  // helpers
  const Tileset* findTilesetForGid(uint32_t gid) const;
  static uint32_t clearFlipFlags(uint32_t gid) { return gid & 0x1FFFFFFFu; }
  static void applyFlipTexcoords(bool h, bool v, bool d, sf::Vector2f tc[4]);

 private:
  int mapWidth_ = 0, mapHeight_ = 0;
  int tileWidth_ = 0, tileHeight_ = 0;
  std::vector<Tileset> tilesets_;  // sorted by firstGid
  std::vector<LayerMesh> layers_;  // draw order
};

#endif  // WORLD_WORLDMAP_H_

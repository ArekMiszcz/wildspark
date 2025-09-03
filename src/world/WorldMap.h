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

  // Test-friendly default constructor (does not load JSON). Useful for unit
  // tests which manually populate layers/tilesets.
  WorldMap() = default;

  struct Tileset {
    struct PerTile {  // for collection-of-images
      int localId = 0;
      std::shared_ptr<sf::Texture> texture;
      int width = 0, height = 0;
    };

    struct Point {
      float x = 0.0f, y = 0.0f;
    };

    struct Object {
      int id = 0;
      std::string name;
      std::string type;
      float x = 0.0f, y = 0.0f;
      float width = 0.0f, height = 0.0f;
      float rotation = 0.0f;
      bool visible = true;
      std::vector<Point> polygon;  // Only supporting polygon objects for now
    };

    struct ObjectGroup {
      int id = 0;
      std::string name;
      std::string draworder;  // Usually "index" or "topdown"
      float opacity = 1.0f;
      bool visible = true;
      std::vector<Object> objects;
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

    // object groups (collision, action areas, etc)
    std::unordered_map<int, ObjectGroup>
        objectGroups;  // tile localId -> ObjectGroup
  };

  struct LayerMesh {
    struct CellKey {
      int x{};
      int y{};
      bool operator==(const CellKey& o) const { return x == o.x && y == o.y; }
    };
    struct CellKeyHash {
      std::size_t operator()(const CellKey& k) const noexcept {
        // 32-bit mix
        std::uint64_t ux = static_cast<std::uint64_t>(static_cast<std::uint32_t>(k.x));
        std::uint64_t uy = static_cast<std::uint64_t>(static_cast<std::uint32_t>(k.y));
        std::uint64_t h = (ux * 73856093u) ^ (uy * 19349663u);
        return static_cast<std::size_t>(h);
      }
    };
    struct Chunk {
      uint32_t id = 0;
      uint32_t gid = 0;
      const sf::Texture* texture = nullptr;
      sf::VertexArray vertices;  // Triangles
      float opacity = 1.f;
      bool visible = true;
      float sortY = 0.f;  // for sorting within a cell
      // Offset of this chunk relative to the owning object's top-left
      // (in world units). This is set at load time so runtime object
      // movements can preserve per-chunk layout.
      sf::Vector2f offset{0.f, 0.f};
      Chunk() : vertices(sf::PrimitiveType::Triangles) {}
    };
    struct ChunkBucket {
      std::vector<Chunk> chunks;
    };
    std::string type;
    std::string name;
    std::vector<CellKey> chunk_bucket_order;
    std::unordered_map<CellKey, ChunkBucket, CellKeyHash> chunk_buckets;
    // Global draw order for object layers: pointers to chunks sorted by
    // their world "foot" (bottom) Y. This allows rendering to follow a
    // global Y-order while keeping spatial buckets for lookups.
    std::vector<Chunk*> object_draw_order;
    bool visible = true;
    float opacity = 1.f;
  };

  // map info (orthogonal only)
  int width() const { return mapWidth_; }
  int height() const { return mapHeight_; }
  int tileWidth() const { return tileWidth_; }
  int tileHeight() const { return tileHeight_; }
  const std::vector<LayerMesh>& layers() const { return layers_; }
  const std::vector<Tileset>& tilesets() const { return tilesets_; }
  sf::Vector2f tileToWorld(int tx, int ty) const {
    return {static_cast<float>(tx * tileWidth_),
            static_cast<float>(ty * tileHeight_)};
  }

  // Get the world position for an object in a tileset
  // This maps tile-local coordinates to world coordinates based on the given
  // world position
  sf::Vector2f objectToWorld(int tileGid, float worldX, float worldY,
                             float objX, float objY) const {
    // In Tiled maps, objects reference a tile with gid and have their own world
    // coordinates (x,y) The objX and objY are relative to the tile's origin

    // Get the tileset for this gid to handle flipping properly
    const Tileset* ts = findTilesetForGid(tileGid);
    if (!ts) return {worldX + objX, worldY + objY};

    // Check if the tile is flipped (not handling this yet, but could be added)
    uint32_t clearedGid = clearFlipFlags(tileGid);

    // Simply add the object's local coordinates to the world position
    // For most tiles, origin is top-left
    return {worldX + objX, worldY + objY};
  }

  sf::FloatRect worldBounds() const {
    return {{0.f, 0.f},
            {static_cast<float>(mapWidth_ * tileWidth_),
             static_cast<float>(mapHeight_ * tileHeight_)}};
  }

  int getObjectIdAtPosition(const sf::Vector2f& worldPos) const;

  // Find which tileset a tile belongs to based on its GID
  const Tileset* findTilesetForGid(uint32_t gid) const;

  // Runtime update API: update an object (by Tiled object id) using a
  // flexible JSON payload from the server ACK. Supported keys include:
  //  - "gid": number (raw GID, may contain flip flags)
  //  - "visible": bool
  //  - "opacity": number (0.0 - 1.0)
  // The optional outAffectedLayers will be filled with indices of layers
  // that were modified (de-duplicated). Returns true if any chunk was
  // modified.
  bool updateObject(int objectId, const nlohmann::json& props,
                    std::vector<int>* outAffectedLayers = nullptr);

  // Rebuild the object_draw_order for a single object layer. This updates
  // the internal draw order pointers and is used by the renderer to only
  // refresh affected layers after runtime mutations.
  void rebuildObjectDrawOrderForLayer(int layerIndex);

  // Test helper: give tests mutable access to layers to populate small maps
  // without having to load JSON files from disk.
  std::vector<LayerMesh>& layersMutable() { return layers_; }

 private:
  // loading
  void loadFromJson(const std::string& mapPath);
  void loadTilesetInline(const std::filesystem::path& mapDir,
                         const nlohmann::json& tsj);
  void loadTilesetExternal(const std::filesystem::path& mapDir,
                           const std::string& source, int firstGid);
  void buildLayers(const nlohmann::json& map);
  static uint32_t clearFlipFlags(uint32_t gid) { return gid & 0x1FFFFFFFu; }
  static void applyFlipTexcoords(bool h, bool v, bool d, sf::Vector2f tc[4]);

 private:
  int mapWidth_ = 0, mapHeight_ = 0;
  int tileWidth_ = 0, tileHeight_ = 0;
  std::vector<Tileset> tilesets_;  // sorted by firstGid
  std::vector<LayerMesh> layers_;  // draw order
};

#endif  // WORLD_WORLDMAP_H_

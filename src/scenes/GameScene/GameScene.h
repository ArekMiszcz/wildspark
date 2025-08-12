// Copyright 2025 WildSpark Authors

#pragma once

#include <nakama-cpp/Nakama.h>
#include <map>
#include <memory>
#include <string>

#include <SFML/Graphics.hpp>

#include "../Scene.h"
#include "../../auth/AuthManager.h"
#include "../../graphics/Camera.h"
#include "../../input/InputManager.h"
#include "../../networking/Networking.h"
#include "../../world/WorldMap.h"
#include "../../world/WorldRenderer.h"
#include "../../world/entities/Player.h"

class SceneManager;

class GameScene : public Scene {
 public:
  GameScene(sf::RenderWindow& window, AuthManager& authManager, InputManager& inputManager,
           Nakama::NClientPtr nakamaClient, Nakama::NSessionPtr session);
  ~GameScene() override;

  void onEnter(SceneManager& sceneManager) override;
  void onExit(SceneManager& sceneManager) override;
  void handleEvent(const sf::Event& event, SceneManager& manager) override;
  void update(sf::Time deltaTime, SceneManager& manager) override;
  void render(sf::RenderTarget& target) override;  // Corrected signature

  void handlePlayerStateUpdate(const std::string& playerId, const sf::Vector2f& position,
                               unsigned int lastProcessedSequence);
  void handleInputAck(const std::string& playerId, unsigned int inputSequence, bool approved,
                      const sf::Vector2f& serverPosition);

 private:
  sf::RenderWindow& windowRef;
  AuthManager& authManagerRef;
  InputManager& m_inputManager;
  WorldMap m_worldMap;
  WorldRenderer m_worldRenderer;
  Camera m_camera;
  std::unique_ptr<Player> m_localPlayer;
  std::unique_ptr<Networking> m_networking;
  SceneManager* sceneManager = nullptr;
  std::map<std::string, std::unique_ptr<Player>> m_otherPlayers;
};

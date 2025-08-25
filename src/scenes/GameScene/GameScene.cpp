// Copyright 2025 WildSpark Authors

#include "GameScene.h"

#include <cmath>
#include <iostream>
#include <memory>
#include <string>
#include <utility>
#include <vector>

#include "../../input/InputManager.h"
#include "../SceneManager.h"

GameScene::GameScene(sf::RenderWindow& window, AuthManager& authManager,
                     InputManager& inputManager,
                     Nakama::NClientPtr nakamaClient,
                     Nakama::NSessionPtr session)
    : Scene(),
      windowRef(window),
      authManagerRef(authManager),
      m_inputManager(inputManager),
      m_worldMap("/elderford/world.json"),
      m_worldRenderer(std::make_unique<WorldRenderer>(m_worldMap)),
      m_camera(windowRef, 300.0f),
      m_networking(std::make_unique<Networking>(nakamaClient)) {
  std::cout << "GameScene created." << std::endl;
  m_inputManager.mapActionToKey("camera_move_up", sf::Keyboard::Key::W);
  m_inputManager.mapActionToKey("camera_move_down", sf::Keyboard::Key::S);
  m_inputManager.mapActionToKey("camera_move_left", sf::Keyboard::Key::A);
  m_inputManager.mapActionToKey("camera_move_right", sf::Keyboard::Key::D);
  m_inputManager.mapActionToMouseButton("player_move",
                                        sf::Mouse::Button::Right);
}

GameScene::~GameScene() { std::cout << "GameScene destroyed." << std::endl; }

void GameScene::onEnter(SceneManager& sceneManager) {
  std::cout << "Entering GameScene." << std::endl;
  this->sceneManager = &sceneManager;

  Nakama::NSessionPtr session = authManagerRef.getNakamaSessionPtr();

  if (!session) {
    std::cerr << "GameScene::onEnter: Nakama session is null. Cannot "
                 "initialize networking."
              << std::endl;
    return;
  }

  if (m_networking) {
    if (!m_networking->initialize(session)) {
      std::cerr
          << "GameScene::onEnter: Failed to initialize Networking with session."
          << std::endl;
      return;
    }

    m_networking->setPlayerStateUpdateCallback(
        [this](const std::string& playerId, const sf::Vector2f& position,
               unsigned int lastProcessedSequence) {
          this->handlePlayerStateUpdate(playerId, position,
                                        lastProcessedSequence);
        });

    m_networking->setInputAckCallback(
        [this](const std::string& playerId, unsigned int inputSequence,
               bool approved, const sf::Vector2f& serverPosition) {
          this->handleInputAck(playerId, inputSequence, approved,
                               serverPosition);
        });

  } else {
    std::cerr << "GameScene::onEnter: m_networking is null. "
              << "This should not happen if constructor was called."
              << std::endl;
    return;
  }

  m_localPlayer = std::make_unique<Player>("local_player_id", sf::Color::Black);
  if (session) {
    m_localPlayer->setId(session->getUserId());
    std::cout << "GameScene: Local player ID set to: " << session->getUserId()
              << std::endl;
  }
  m_localPlayer->setPosition(sf::Vector2f(100.f, 100.f));

  if (m_networking) {
    m_networking->listMatches(
        [this](const std::vector<Nakama::NMatch>& matches) {
          if (!matches.empty()) {
            std::cout << "Found " << matches.size()
                      << " matches. Joining the first one: "
                      << matches[0].matchId << std::endl;
            m_networking->joinMatch(
                matches[0].matchId,
                [this, matchId = matches[0].matchId](bool success) {
                  if (success) {
                    std::cout << "Successfully joined match: " << matchId
                              << std::endl;
                  } else {
                    std::cerr
                        << "Failed to join match: " << matchId
                        << ". Check server logs and ensure a match is running."
                        << std::endl;
                  }
                });
          } else {
            std::cout
                << "No matches found. Ensure a match is created on the server."
                << std::endl;
          }
        },
        [this](const Nakama::NError& error) {
          std::cerr << "Error listing matches: " << error.message << std::endl;
        });
  }
}

void GameScene::onExit(SceneManager& sceneManager) {
  std::cout << "Exiting GameScene." << std::endl;
  if (m_networking && !m_networking->getCurrentMatchId().empty()) {
    // TODO(miszczu): Implement
    // m_networking->leaveMatch(m_networking->getCurrentMatchId());
  }
}

void GameScene::handleEvent(const sf::Event& event, SceneManager& manager) {
  m_inputManager.handleEvent(event);

  if (event.is<sf::Event::Closed>()) {
    windowRef.close();
    return;
  }
}

void GameScene::update(sf::Time deltaTime, SceneManager& manager) {
  if (m_networking) {
    m_networking->tick();
  }

  m_camera.setMovingUp(m_inputManager.isActionActive("camera_move_up"));
  m_camera.setMovingDown(m_inputManager.isActionActive("camera_move_down"));
  m_camera.setMovingLeft(m_inputManager.isActionActive("camera_move_left"));
  m_camera.setMovingRight(m_inputManager.isActionActive("camera_move_right"));
  m_camera.update(deltaTime);

  if (m_localPlayer && m_networking) {
    if (m_inputManager.isActionActive("player_move")) {
      sf::Vector2i mousePos = sf::Mouse::getPosition(windowRef);
      sf::Vector2f worldPos =
          windowRef.mapPixelToCoords(mousePos, m_camera.getView());
      sf::Vector2f newDirection = worldPos - m_localPlayer->getPosition();
      float length = std::sqrt(newDirection.x * newDirection.x +
                               newDirection.y * newDirection.y);
      if (length > 0) {
        newDirection /= length;
      }
      m_localPlayer->setTargetDirection(newDirection);
      if (newDirection.x != 0.f || newDirection.y != 0.f) {
        m_networking->sendPlayerUpdate(m_localPlayer->getDirection(),
                                       m_localPlayer->getSpeed(),
                                       m_localPlayer->getNextSequenceNumber());
        std::cout << "GameScene: Player movement command sent. Direction: ("
                  << m_localPlayer->getDirection().x << ", "
                  << m_localPlayer->getDirection().y << ")" << std::endl;
      }
    } else {
      if (m_localPlayer->getDirection().x != 0.f ||
          m_localPlayer->getDirection().y != 0.f) {
        m_localPlayer->setTargetDirection({0.f, 0.f});
        m_networking->sendPlayerUpdate(m_localPlayer->getDirection(),
                                       m_localPlayer->getSpeed(),
                                       m_localPlayer->getNextSequenceNumber());
        std::cout << "GameScene: Player stopped. Sending zero direction. Seq: "
                  << m_localPlayer->getNextSequenceNumber() - 1 << std::endl;
      }
    }
  }

  if (m_localPlayer) {
    m_localPlayer->update(deltaTime);
  }

  for (auto& pair : m_otherPlayers) {
    pair.second->update(deltaTime);
  }

  m_inputManager.update();
}

void GameScene::render(sf::RenderTarget& target) {
  // Apply camera to world
  m_camera.applyTo(target);

  // Configure renderer as you already did
  m_worldRenderer->setCulling(true);
  m_worldRenderer->setDebugGrid(true);

  // 1) Ground & floor layers first
  m_worldRenderer->renderGround(target);

  // 2) Actors (local player, then others)
  if (m_localPlayer) {
    m_localPlayer->render(target);
    m_camera.setCenter(m_localPlayer->getPosition());
  }

  for (const auto& pair : m_otherPlayers) {
    pair.second->render(target);
  }

  // 3) Occluders/overlays (walls, roofs, etc.) after players
  m_worldRenderer->renderOverlays(target);

  // Restore default view for UI
  target.setView(target.getDefaultView());
}

void GameScene::handlePlayerStateUpdate(const std::string& playerId,
                                        const sf::Vector2f& position,
                                        unsigned int lastProcessedSequence) {
  if (m_localPlayer && playerId == m_localPlayer->getId()) {
    m_localPlayer->handleServerUpdate(position, lastProcessedSequence);
  } else {
    auto it = m_otherPlayers.find(playerId);
    if (it == m_otherPlayers.end()) {
      std::cout << "GameScene: New player detected with ID: " << playerId
                << std::endl;
      auto newPlayer = std::make_unique<Player>(playerId, sf::Color::Red);
      newPlayer->setPosition(position);
      newPlayer->handleServerUpdate(position, lastProcessedSequence);
      m_otherPlayers[playerId] = std::move(newPlayer);
    } else {
      it->second->handleServerUpdate(position, lastProcessedSequence);
    }
  }
}

void GameScene::handleInputAck(const std::string& playerId,
                               unsigned int inputSequence, bool approved,
                               const sf::Vector2f& serverPosition) {
  if (m_localPlayer && playerId == m_localPlayer->getId()) {
    std::cout << "GameScene: Input ACK received for local player. ID: "
              << playerId << ", Seq: " << inputSequence
              << ", Approved: " << (approved ? "Yes" : "No") << ", ServerPos: ("
              << serverPosition.x << ", " << serverPosition.y << ")"
              << std::endl;
    m_localPlayer->handleServerAck(inputSequence, approved, serverPosition);
  } else if (m_localPlayer && playerId != m_localPlayer->getId()) {
    // ACK for another player, usually not processed by other clients directly
    // unless for specific game logic. std::cout << "GameScene: Received Input
    // ACK for another player: " << playerId << std::endl;
  } else if (!m_localPlayer) {
    std::cerr
        << "GameScene: Received Input ACK but local player is null. PlayerID: "
        << playerId << std::endl;
  }
}

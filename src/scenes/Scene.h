// Copyright 2025 WildSpark Authors

#pragma once

#include <SFML/Graphics.hpp>

class SceneManager;

enum class SceneType {
  None,
  Loading,
  Login,
  CharacterSelection,
  CharacterCreation,
  Game,
  Settings,
  Credits
};

class Scene {
 public:
  virtual ~Scene() = default;
  virtual void onEnter(SceneManager& sceneManager) {}
  virtual void onExit(SceneManager& sceneManager) {}
  virtual void handleEvent(const sf::Event& event, SceneManager& manager) = 0;
  virtual void update(sf::Time deltaTime, SceneManager& manager) = 0;
  virtual void render(sf::RenderTarget& target) = 0;
  SceneManager* sceneManager = nullptr;
};

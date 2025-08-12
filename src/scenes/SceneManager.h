// Copyright 2025 WildSpark Authors

#pragma once

#include <map>
#include <memory>

#include <SFML/Graphics.hpp>

#include "Scene.h"

class SceneManager {
 public:
  explicit SceneManager(sf::RenderWindow& window);
  virtual ~SceneManager();

  void addScene(SceneType type, std::unique_ptr<Scene> scene);
  void removeScene(SceneType type);
  virtual void switchTo(SceneType type);

  SceneType getCurrentSceneType() const;
  size_t getSceneCount() const;

  void handleEvent(sf::RenderWindow& window, const sf::Event& event);
  void update(sf::RenderWindow& window, sf::Time deltaTime);
  void render(sf::RenderWindow& window);
  void shutdown();

  sf::RenderWindow* window = nullptr;

 private:
  std::map<SceneType, std::unique_ptr<Scene>> scenes;
  Scene* currentScene = nullptr;
  SceneType currentSceneType = SceneType::None;
  SceneType requestedSceneType = SceneType::None;

  void processSceneSwitch();
};

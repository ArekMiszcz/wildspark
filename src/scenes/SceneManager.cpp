// Copyright 2025 WildSpark Authors

#include "SceneManager.h"

#include <iostream>
#include <stdexcept>
#include <memory>
#include <utility>

#include "imgui-SFML.h"

SceneManager::SceneManager(sf::RenderWindow& window)
    : currentScene(nullptr),
      currentSceneType(SceneType::None),
      requestedSceneType(SceneType::None) {
  this->window = &window;

  // Initialize the window or any other resources if needed
  if (!ImGui::SFML::Init(window)) {
    std::cerr << "Failed to initialize ImGui-SFML" << std::endl;
    throw std::runtime_error("Failed to initialize ImGui-SFML");
  }

  std::cout << "SceneManager initialized" << std::endl;
}

SceneManager::~SceneManager() {
  scenes.clear();
}

void SceneManager::addScene(SceneType type, std::unique_ptr<Scene> scene) {
  if (!scene) {
    std::cerr << "SceneManager::addScene: ERROR - Attempted to add null scene for type "
              << static_cast<int>(type) << std::endl;
    return;
  }

  scene->sceneManager = this;
  scenes[type] = std::move(scene);
  auto it = scenes.find(type);
  if (it != scenes.end()) {
    // Scene successfully added and found
  } else {
    std::cerr << "SceneManager::addScene: VERIFICATION FAILED: Scene type "
              << static_cast<int>(type)
              << " NOT found in map immediately after adding." << std::endl;
  }
}

void SceneManager::removeScene(SceneType type) {
  // Check if the scene exists
  auto it = scenes.find(type);
  if (it == scenes.end()) {
    std::cerr << "SceneManager::removeScene: Scene type " << static_cast<int>(type)
              << " not found in map." << std::endl;
    return;
  }

  // If we're removing the current scene, reset current scene pointers
  if (currentSceneType == type) {
    if (currentScene) {
      std::cout << "SceneManager::removeScene: Removing current active scene. Calling onExit." << std::endl;
      currentScene->onExit(*this);
    }
    currentScene = nullptr;
    currentSceneType = SceneType::None;
  }

  // Remove the scene from the map
  scenes.erase(it);
}

void SceneManager::switchTo(SceneType type) { requestedSceneType = type; }

SceneType SceneManager::getCurrentSceneType() const { return currentSceneType; }

size_t SceneManager::getSceneCount() const { return scenes.size(); }

void SceneManager::processSceneSwitch() {
  if (requestedSceneType != SceneType::None &&
      requestedSceneType != currentSceneType) {
    if (currentScene) {
      currentScene->onExit(*this);
    }

    auto it = scenes.find(requestedSceneType);
    if (it != scenes.end()) {
      currentScene = it->second.get();
      currentSceneType = requestedSceneType;
      currentScene->onEnter(*this);
    } else {
      std::cerr << "SceneManager::processSceneSwitch: ERROR - Scene type "
                << static_cast<int>(requestedSceneType)
                << " NOT found in map." << std::endl;
      std::cerr << "SceneManager::processSceneSwitch: Current map contents (size "
                << scenes.size() << "):" << std::endl;
      for (const auto& pair : scenes) {
        std::cerr << "SceneManager::processSceneSwitch: Map has key: "
                  << static_cast<int>(pair.first) << " -> points to scene: "
                  << (pair.second ? "valid" : "invalid/null") << std::endl;
      }
      currentScene = nullptr;
      currentSceneType = SceneType::None;
      std::cerr << "SceneManager::processSceneSwitch: Reset currentScene to nullptr and currentSceneType "
                << "to None due to find failure." << std::endl;
    }
    requestedSceneType = SceneType::None;
  }
}

void SceneManager::handleEvent(sf::RenderWindow& window, const sf::Event& event) {
  ImGui::SFML::ProcessEvent(window, event);

  if (currentScene) {
    currentScene->handleEvent(event, *this);
  }
}

void SceneManager::update(sf::RenderWindow& window, sf::Time deltaTime) {
  processSceneSwitch();
  if (currentScene) {
    currentScene->update(deltaTime, *this);
  }
  ImGui::SFML::Update(window, deltaTime);
}

void SceneManager::render(sf::RenderWindow& target) {
  if (currentScene) {
    currentScene->render(target);
  }
  ImGui::SFML::Render(target);
}

void SceneManager::shutdown() {
  ImGui::SFML::Shutdown();
  std::cout << "SceneManager ImGui::SFML::Shutdown() called" << std::endl;
}

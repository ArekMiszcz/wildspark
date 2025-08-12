// Copyright 2025 WildSpark Authors

#pragma once

#include "gmock/gmock.h"
#include "scenes/SceneManager.h"

#include <SFML/Graphics.hpp>

class MockSceneManager : public SceneManager {
 public:
  explicit MockSceneManager(sf::RenderWindow& window) : SceneManager(window) {}
  MOCK_METHOD(void, switchTo, (SceneType type), (override));
};

// Copyright 2025 WildSpark Authors

#pragma once

#include "gmock/gmock.h"

#include <SFML/Graphics.hpp>

class MockRenderWindow : public sf::RenderWindow {
 public:
  MockRenderWindow() {}

  MOCK_METHOD(bool, setActive, (bool active), (override));
  MOCK_METHOD(void, close, (), (override));
};

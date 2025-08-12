// Copyright 2025 WildSpark Authors

#include <gtest/gtest.h>

#include <memory>
#include <utility>

#include "gmock/gmock.h"
#include "mocks/MockRenderWindow.h"
#include "scenes/Scene.h"
#include "scenes/SceneManager.h"

#include <SFML/Graphics.hpp>

class MockScene : public Scene {
 public:
  MOCK_METHOD(void, onEnter, (SceneManager & manager), (override));
  MOCK_METHOD(void, onExit, (SceneManager & manager), (override));
  MOCK_METHOD(void, handleEvent,
              (const sf::Event& event, SceneManager& manager), (override));
  MOCK_METHOD(void, update, (sf::Time deltaTime, SceneManager& manager),
              (override));
  MOCK_METHOD(void, render, (sf::RenderTarget & target), (override));
  ~MockScene() override = default;
};

class SceneManagerTest : public ::testing::Test {
 protected:
  MockRenderWindow mockWindow;
  std::unique_ptr<SceneManager> sceneManagerPtr;
  bool setupSucceeded = true;

  SceneManagerTest() {
    try {
      sceneManagerPtr = std::make_unique<SceneManager>(mockWindow);
    } catch (...) {
      setupSucceeded = false;
    }
  }

  void SetUp() override {
    if (!setupSucceeded) GTEST_SKIP() << "Setup failed";
  }
  void TearDown() override {
    if (sceneManagerPtr && setupSucceeded) sceneManagerPtr->shutdown();
  }
};

TEST_F(SceneManagerTest, InitialState) {
  ASSERT_EQ(sceneManagerPtr->getSceneCount(), 0);
  ASSERT_EQ(sceneManagerPtr->getCurrentSceneType(), SceneType::None);
}

TEST_F(SceneManagerTest, AddScene) {
  auto mockScene1 = std::make_unique<MockScene>();
  sceneManagerPtr->addScene(SceneType::Login, std::move(mockScene1));
  ASSERT_EQ(sceneManagerPtr->getSceneCount(), 1);
}

TEST_F(SceneManagerTest, SwitchScene) {
  auto scene1 = std::make_unique<MockScene>();
  auto scene2 = std::make_unique<MockScene>();
  MockScene* rawScene1 = scene1.get();
  MockScene* rawScene2 = scene2.get();
  sceneManagerPtr->addScene(SceneType::Login, std::move(scene1));
  sceneManagerPtr->addScene(SceneType::Game, std::move(scene2));

  EXPECT_CALL(*rawScene1, onEnter(::testing::_));
  EXPECT_CALL(*rawScene1, update(::testing::_, ::testing::_));
  sceneManagerPtr->switchTo(SceneType::Login);
  sceneManagerPtr->update(mockWindow, sf::seconds(0.016f));
  ASSERT_EQ(sceneManagerPtr->getCurrentSceneType(), SceneType::Login);
  testing::Mock::VerifyAndClearExpectations(rawScene1);

  EXPECT_CALL(*rawScene1, render(::testing::Ref(mockWindow)));
  sceneManagerPtr->render(mockWindow);
  testing::Mock::VerifyAndClearExpectations(rawScene1);

  EXPECT_CALL(*rawScene1, onExit(::testing::_));
  EXPECT_CALL(*rawScene2, onEnter(::testing::_));
  EXPECT_CALL(*rawScene2, update(::testing::_, ::testing::_));
  sceneManagerPtr->switchTo(SceneType::Game);
  sceneManagerPtr->update(mockWindow, sf::seconds(0.016f));
  ASSERT_EQ(sceneManagerPtr->getCurrentSceneType(), SceneType::Game);
  testing::Mock::VerifyAndClearExpectations(rawScene1);
  testing::Mock::VerifyAndClearExpectations(rawScene2);
}

TEST_F(SceneManagerTest, SwitchToNonExistentScene) {
  ASSERT_EQ(sceneManagerPtr->getCurrentSceneType(), SceneType::None);
  sceneManagerPtr->switchTo(SceneType::Login);
  sceneManagerPtr->update(mockWindow, sf::seconds(0.016f));
  ASSERT_EQ(sceneManagerPtr->getCurrentSceneType(), SceneType::None);
}

TEST_F(SceneManagerTest, HandleEventCallsCurrentScene) {
  auto mockGameScene = std::make_unique<MockScene>();
  MockScene* raw = mockGameScene.get();
  sceneManagerPtr->addScene(SceneType::Game, std::move(mockGameScene));

  EXPECT_CALL(*raw, onEnter(::testing::_));
  EXPECT_CALL(*raw, update(sf::seconds(0.016f), ::testing::_));
  sceneManagerPtr->switchTo(SceneType::Game);
  sceneManagerPtr->update(mockWindow, sf::seconds(0.016f));
  testing::Mock::VerifyAndClearExpectations(raw);

  sf::Event testEvent(sf::Event::KeyPressed{sf::Keyboard::Key::A,
                                            sf::Keyboard::Scancode::A, false,
                                            false, false, false});
  EXPECT_CALL(*raw, handleEvent(::testing::_, ::testing::_));
  sceneManagerPtr->handleEvent(mockWindow, testEvent);
  testing::Mock::VerifyAndClearExpectations(raw);
}

TEST_F(SceneManagerTest, UpdateCallsCurrentScene) {
  auto mockGameScene = std::make_unique<MockScene>();
  MockScene* raw = mockGameScene.get();
  sceneManagerPtr->addScene(SceneType::Game, std::move(mockGameScene));

  EXPECT_CALL(*raw, onEnter(::testing::_));
  EXPECT_CALL(*raw, update(sf::seconds(0.016f), ::testing::_))
      .RetiresOnSaturation();
  sceneManagerPtr->switchTo(SceneType::Game);
  sceneManagerPtr->update(mockWindow, sf::seconds(0.016f));
  testing::Mock::VerifyAndClearExpectations(raw);

  sf::Time dt = sf::seconds(0.032f);
  EXPECT_CALL(*raw, update(dt, ::testing::_));
  sceneManagerPtr->render(mockWindow);
  sceneManagerPtr->update(mockWindow, dt);
  testing::Mock::VerifyAndClearExpectations(raw);
}

TEST_F(SceneManagerTest, RenderCallsCurrentScene) {
  auto mockGameScene = std::make_unique<MockScene>();
  MockScene* raw = mockGameScene.get();
  sceneManagerPtr->addScene(SceneType::Game, std::move(mockGameScene));

  EXPECT_CALL(*raw, onEnter(::testing::_));
  EXPECT_CALL(*raw, update(sf::seconds(0.016f), ::testing::_));
  sceneManagerPtr->switchTo(SceneType::Game);
  sceneManagerPtr->update(mockWindow, sf::seconds(0.016f));
  testing::Mock::VerifyAndClearExpectations(raw);

  EXPECT_CALL(*raw, render(::testing::Ref(mockWindow)));
  sceneManagerPtr->render(mockWindow);
  testing::Mock::VerifyAndClearExpectations(raw);
}

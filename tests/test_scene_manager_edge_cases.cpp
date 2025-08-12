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
  MOCK_METHOD(void, onEnter, (SceneManager&), (override));
  MOCK_METHOD(void, onExit, (SceneManager&), (override));
  MOCK_METHOD(void, handleEvent, (const sf::Event&, SceneManager&), (override));
  MOCK_METHOD(void, update, (sf::Time, SceneManager&), (override));
  MOCK_METHOD(void, render, (sf::RenderTarget&), (override));
  ~MockScene() override = default;
};

class SceneManagerEdgeCasesTest : public ::testing::Test {
 protected:
  MockRenderWindow mockWindow;
  std::unique_ptr<SceneManager> sceneManagerPtr;
  bool setupSucceeded = true;
  SceneManagerEdgeCasesTest() {
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

TEST_F(SceneManagerEdgeCasesTest, DuplicateSceneTypes) {
  auto s1 = std::make_unique<MockScene>();
  auto s2 = std::make_unique<MockScene>();
  MockScene* raw2 = s2.get();
  sceneManagerPtr->addScene(SceneType::Login, std::move(s1));
  ASSERT_EQ(sceneManagerPtr->getSceneCount(), 1);
  sceneManagerPtr->addScene(SceneType::Login, std::move(s2));
  ASSERT_EQ(sceneManagerPtr->getSceneCount(), 1);
  EXPECT_CALL(*raw2, onEnter(::testing::_));
  EXPECT_CALL(*raw2, update(::testing::_, ::testing::_));
  sceneManagerPtr->switchTo(SceneType::Login);
  sceneManagerPtr->update(mockWindow, sf::seconds(0.016f));
  testing::Mock::VerifyAndClearExpectations(raw2);
  EXPECT_CALL(*raw2, render(::testing::_));
  sceneManagerPtr->render(mockWindow);
}

TEST_F(SceneManagerEdgeCasesTest, ComplexSceneSwitchingSequence) {
  auto login = std::make_unique<MockScene>();
  auto game = std::make_unique<MockScene>();
  auto settings = std::make_unique<MockScene>();
  auto rLogin = login.get();
  auto rGame = game.get();
  auto rSettings = settings.get();
  sceneManagerPtr->addScene(SceneType::Login, std::move(login));
  sceneManagerPtr->addScene(SceneType::Game, std::move(game));
  sceneManagerPtr->addScene(SceneType::Settings, std::move(settings));
  ASSERT_EQ(sceneManagerPtr->getCurrentSceneType(), SceneType::None);
  EXPECT_CALL(*rLogin, onEnter(::testing::_));
  EXPECT_CALL(*rLogin, update(::testing::_, ::testing::_));
  sceneManagerPtr->switchTo(SceneType::Login);
  sceneManagerPtr->update(mockWindow, sf::seconds(0.016f));
  EXPECT_CALL(*rLogin, render(::testing::_));
  sceneManagerPtr->render(mockWindow);
  EXPECT_CALL(*rLogin, onExit(::testing::_));
  EXPECT_CALL(*rGame, onEnter(::testing::_));
  EXPECT_CALL(*rGame, update(::testing::_, ::testing::_));
  sceneManagerPtr->switchTo(SceneType::Game);
  sceneManagerPtr->update(mockWindow, sf::seconds(0.016f));
  EXPECT_CALL(*rGame, render(::testing::_));
  sceneManagerPtr->render(mockWindow);
  EXPECT_CALL(*rGame, onExit(::testing::_));
  EXPECT_CALL(*rSettings, onEnter(::testing::_));
  EXPECT_CALL(*rSettings, update(::testing::_, ::testing::_));
  sceneManagerPtr->switchTo(SceneType::Settings);
  sceneManagerPtr->update(mockWindow, sf::seconds(0.016f));
  EXPECT_CALL(*rSettings, render(::testing::_));
  sceneManagerPtr->render(mockWindow);
  EXPECT_CALL(*rSettings, onExit(::testing::_));
  EXPECT_CALL(*rLogin, onEnter(::testing::_));
  EXPECT_CALL(*rLogin, update(::testing::_, ::testing::_));
  sceneManagerPtr->switchTo(SceneType::Login);
  sceneManagerPtr->update(mockWindow, sf::seconds(0.016f));
  EXPECT_CALL(*rLogin, render(::testing::_));
  sceneManagerPtr->render(mockWindow);
}

TEST_F(SceneManagerEdgeCasesTest, NullSceneHandling) {
  sceneManagerPtr->addScene(SceneType::Login, nullptr);
  ASSERT_EQ(sceneManagerPtr->getSceneCount(), 0);
  sceneManagerPtr->switchTo(SceneType::Login);
  sceneManagerPtr->update(mockWindow, sf::seconds(0.016f));
  sceneManagerPtr->render(mockWindow);  // ensure frame ended
  ASSERT_EQ(sceneManagerPtr->getCurrentSceneType(), SceneType::None);
  auto valid = std::make_unique<MockScene>();
  auto raw = valid.get();
  sceneManagerPtr->addScene(SceneType::Game, std::move(valid));
  ASSERT_EQ(sceneManagerPtr->getSceneCount(), 1);
  EXPECT_CALL(*raw, onEnter(::testing::_));
  EXPECT_CALL(*raw, update(::testing::_, ::testing::_));
  sceneManagerPtr->switchTo(SceneType::Game);
  sceneManagerPtr->update(mockWindow, sf::seconds(0.016f));
  EXPECT_CALL(*raw, render(::testing::_));
  sceneManagerPtr->render(mockWindow);
  sceneManagerPtr->addScene(SceneType::Game, nullptr);
  ASSERT_EQ(sceneManagerPtr->getSceneCount(), 1);
  EXPECT_CALL(*raw, update(::testing::_, ::testing::_));
  sceneManagerPtr->update(mockWindow, sf::seconds(0.016f));
  EXPECT_CALL(*raw, render(::testing::_));
  sceneManagerPtr->render(mockWindow);
}

TEST_F(SceneManagerEdgeCasesTest, ConcurrentSceneSwitches) {
  auto login = std::make_unique<MockScene>();
  auto game = std::make_unique<MockScene>();
  auto rLogin = login.get();
  auto rGame = game.get();
  sceneManagerPtr->addScene(SceneType::Login, std::move(login));
  sceneManagerPtr->addScene(SceneType::Game, std::move(game));
  sceneManagerPtr->switchTo(SceneType::Login);
  sceneManagerPtr->switchTo(SceneType::Game);
  EXPECT_CALL(*rGame, onEnter(::testing::_));
  EXPECT_CALL(*rGame, update(::testing::_, ::testing::_));
  sceneManagerPtr->update(mockWindow, sf::seconds(0.016f));
  ASSERT_EQ(sceneManagerPtr->getCurrentSceneType(), SceneType::Game);
  EXPECT_CALL(*rGame, render(::testing::_));
  sceneManagerPtr->render(mockWindow);
  sceneManagerPtr->switchTo(SceneType::Login);
  sceneManagerPtr->switchTo(SceneType::Game);
  EXPECT_CALL(*rGame, update(::testing::_, ::testing::_));
  sceneManagerPtr->update(mockWindow, sf::seconds(0.016f));
  EXPECT_CALL(*rGame, render(::testing::_));
  sceneManagerPtr->render(mockWindow);
  EXPECT_CALL(*rGame, onExit(::testing::_));
  EXPECT_CALL(*rLogin, onEnter(::testing::_));
  EXPECT_CALL(*rLogin, update(::testing::_, ::testing::_));
  sceneManagerPtr->switchTo(SceneType::Login);
  sceneManagerPtr->update(mockWindow, sf::seconds(0.016f));
  EXPECT_CALL(*rLogin, render(::testing::_));
  sceneManagerPtr->render(mockWindow);
}

TEST_F(SceneManagerEdgeCasesTest, SceneLifecycleEvents) {
  auto login = std::make_unique<MockScene>();
  auto game = std::make_unique<MockScene>();
  auto rLogin = login.get();
  auto rGame = game.get();
  sceneManagerPtr->addScene(SceneType::Login, std::move(login));
  sceneManagerPtr->addScene(SceneType::Game, std::move(game));
  EXPECT_CALL(*rLogin, onEnter(::testing::_));
  EXPECT_CALL(*rLogin, update(::testing::_, ::testing::_));
  sceneManagerPtr->switchTo(SceneType::Login);
  sceneManagerPtr->update(mockWindow, sf::seconds(0.016f));
  EXPECT_CALL(*rLogin, render(::testing::_));
  sceneManagerPtr->render(mockWindow);
  EXPECT_CALL(*rLogin, onExit(::testing::_));
  EXPECT_CALL(*rGame, onEnter(::testing::_));
  EXPECT_CALL(*rGame, update(::testing::_, ::testing::_));
  sceneManagerPtr->switchTo(SceneType::Game);
  sceneManagerPtr->update(mockWindow, sf::seconds(0.016f));
  EXPECT_CALL(*rGame, render(::testing::_));
  sceneManagerPtr->render(mockWindow);
}

TEST_F(SceneManagerEdgeCasesTest, SwitchToNonExistentSceneTypes) {
  auto login = std::make_unique<MockScene>();
  auto raw = login.get();
  sceneManagerPtr->addScene(SceneType::Login, std::move(login));
  EXPECT_CALL(*raw, onEnter(::testing::_));
  EXPECT_CALL(*raw, update(::testing::_, ::testing::_));
  sceneManagerPtr->switchTo(SceneType::Login);
  sceneManagerPtr->update(mockWindow, sf::seconds(0.016f));
  EXPECT_CALL(*raw, render(::testing::_));
  sceneManagerPtr->render(mockWindow);
  EXPECT_CALL(*raw, onExit(::testing::_));
  sceneManagerPtr->switchTo(SceneType::Game);
  sceneManagerPtr->update(mockWindow, sf::seconds(0.016f));
  ASSERT_EQ(sceneManagerPtr->getCurrentSceneType(), SceneType::None);
  sceneManagerPtr->render(mockWindow);
}

TEST_F(SceneManagerEdgeCasesTest, HandleEventsWithNoActiveScene) {
  sf::Event e(sf::Event::KeyPressed{sf::Keyboard::Key::A,
                                    sf::Keyboard::Scancode::A, false, false,
                                    false, false});
  sceneManagerPtr->handleEvent(mockWindow, e);
  ASSERT_EQ(sceneManagerPtr->getCurrentSceneType(), SceneType::None);
  sceneManagerPtr->update(mockWindow, sf::seconds(0.016f));
  sceneManagerPtr->render(mockWindow);
}

TEST_F(SceneManagerEdgeCasesTest, UpdateWithNoActiveScene) {
  sceneManagerPtr->update(mockWindow, sf::seconds(0.016f));
  ASSERT_EQ(sceneManagerPtr->getCurrentSceneType(), SceneType::None);
  sceneManagerPtr->render(mockWindow);
}

TEST_F(SceneManagerEdgeCasesTest, RenderWithNoActiveScene) {
  sceneManagerPtr->update(mockWindow, sf::seconds(0.016f));
  sceneManagerPtr->render(mockWindow);
  ASSERT_EQ(sceneManagerPtr->getCurrentSceneType(), SceneType::None);
}

TEST_F(SceneManagerEdgeCasesTest, RemoveScene) {
  auto login = std::make_unique<MockScene>();
  auto game = std::make_unique<MockScene>();
  auto rLogin = login.get();
  auto rGame = game.get();
  sceneManagerPtr->addScene(SceneType::Login, std::move(login));
  sceneManagerPtr->addScene(SceneType::Game, std::move(game));
  ASSERT_EQ(sceneManagerPtr->getSceneCount(), 2);
  sceneManagerPtr->removeScene(SceneType::Login);
  ASSERT_EQ(sceneManagerPtr->getSceneCount(), 1);
  sceneManagerPtr->switchTo(SceneType::Login);
  sceneManagerPtr->update(mockWindow, sf::seconds(0.016f));
  sceneManagerPtr->render(mockWindow);  // end frame for empty update
  ASSERT_EQ(sceneManagerPtr->getCurrentSceneType(), SceneType::None);
  EXPECT_CALL(*rGame, onEnter(::testing::_));
  EXPECT_CALL(*rGame, update(::testing::_, ::testing::_));
  sceneManagerPtr->switchTo(SceneType::Game);
  sceneManagerPtr->update(mockWindow, sf::seconds(0.016f));
  EXPECT_CALL(*rGame, render(::testing::_));
  sceneManagerPtr->render(mockWindow);
  EXPECT_CALL(*rGame, onExit(::testing::_));
  sceneManagerPtr->removeScene(SceneType::Game);
  ASSERT_EQ(sceneManagerPtr->getSceneCount(), 0);
  ASSERT_EQ(sceneManagerPtr->getCurrentSceneType(), SceneType::None);
  sceneManagerPtr->update(mockWindow, sf::seconds(0.016f));
  sceneManagerPtr->render(mockWindow);
}

TEST_F(SceneManagerEdgeCasesTest, RemoveNonExistentScene) {
  sceneManagerPtr->removeScene(SceneType::Login);
  ASSERT_EQ(sceneManagerPtr->getSceneCount(), 0);
  auto login = std::make_unique<MockScene>();
  auto raw = login.get();
  sceneManagerPtr->addScene(SceneType::Login, std::move(login));
  ASSERT_EQ(sceneManagerPtr->getSceneCount(), 1);
  sceneManagerPtr->removeScene(SceneType::Game);
  ASSERT_EQ(sceneManagerPtr->getSceneCount(), 1);
  EXPECT_CALL(*raw, onEnter(::testing::_));
  EXPECT_CALL(*raw, update(::testing::_, ::testing::_));
  sceneManagerPtr->switchTo(SceneType::Login);
  sceneManagerPtr->update(mockWindow, sf::seconds(0.016f));
  EXPECT_CALL(*raw, render(::testing::_));
  sceneManagerPtr->render(mockWindow);
}

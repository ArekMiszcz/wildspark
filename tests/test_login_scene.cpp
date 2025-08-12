// Copyright 2025 WildSpark Authors

#include <memory>
#include <string>

#include "gmock/gmock.h"
#include "gtest/gtest.h"
#include "imgui-SFML.h"
#include "imgui.h"
#include "mocks/MockAuthManager.h"
#include "mocks/MockRenderWindow.h"
#include "mocks/MockSceneManager.h"
#include "scenes/LoginScene/LoginScene.h"

#include <SFML/Graphics.hpp>

class LoginSceneTest : public ::testing::Test {
 protected:
  MockRenderWindow mockWindow;
  MockAuthManager mockAuthManager;
  std::unique_ptr<MockSceneManager> mockSceneManager;
  std::unique_ptr<LoginScene> loginScene;

  void SetUp() override {
    ASSERT_TRUE(ImGui::SFML::Init(mockWindow));

    mockSceneManager = std::make_unique<MockSceneManager>(mockWindow);
    loginScene = std::make_unique<LoginScene>(mockWindow, mockAuthManager);
  }

  void TearDown() override {
    loginScene.reset();
    mockSceneManager.reset();
    ImGui::SFML::Shutdown(mockWindow);
  }
};

TEST_F(LoginSceneTest, OnEnterResetsStatusAndSetsSceneManager) {
  loginScene->onEnter(*mockSceneManager);
  EXPECT_CALL(*mockSceneManager, switchTo(testing::_)).Times(0);
}

TEST_F(LoginSceneTest, HandleLoginSuccess) {
  loginScene->onEnter(*mockSceneManager);

  EXPECT_CALL(mockAuthManager,
              mockableAttemptLogin(testing::StrEq("test@example.com"),
                                   testing::StrEq("password"), testing::_))
      .WillOnce(testing::InvokeArgument<2>(true, "Login successful"));

  EXPECT_CALL(*mockSceneManager, switchTo(SceneType::CharacterSelection))
      .Times(1);

  loginScene->handleLogin("test@example.com", "password");
}

TEST_F(LoginSceneTest, HandleLoginFailure) {
  loginScene->onEnter(*mockSceneManager);

  EXPECT_CALL(mockAuthManager,
              mockableAttemptLogin(testing::StrEq("test@example.com"),
                                   testing::StrEq("wrongpassword"), testing::_))
      .WillOnce(testing::InvokeArgument<2>(false, "Login failed"));

  EXPECT_CALL(*mockSceneManager, switchTo(testing::_)).Times(0);

  loginScene->handleLogin("test@example.com", "wrongpassword");
}

TEST_F(LoginSceneTest, RenderDoesNotCrash) {
  loginScene->onEnter(*mockSceneManager);
  ImGui::SFML::Update(mockWindow, sf::seconds(0.016f));

  loginScene->render(mockWindow);

  ImGui::EndFrame();
  ImGui::Render();
}

TEST_F(LoginSceneTest, OnExitIsCallable) {
  loginScene->onEnter(*mockSceneManager);
  loginScene->onExit(*mockSceneManager);
}

TEST_F(LoginSceneTest, HandleEventIsCallable) {
  sf::Event dummyEvent = sf::Event::Closed{};
  loginScene->onEnter(*mockSceneManager);
  loginScene->handleEvent(dummyEvent, *mockSceneManager);
}

TEST_F(LoginSceneTest, UpdateIsCallable) {
  sf::Time dummyTime = sf::seconds(0.016f);
  loginScene->onEnter(*mockSceneManager);
  loginScene->update(dummyTime, *mockSceneManager);
}

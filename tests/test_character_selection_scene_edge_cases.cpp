// Copyright 2025 WildSpark Authors

#include <memory>

#include "gmock/gmock.h"
#include "gtest/gtest.h"
#include "imgui-SFML.h"
#include "imgui.h"
#include "mocks/MockAccountManager.h"
#include "mocks/MockAuthManager.h"
#include "mocks/MockRenderWindow.h"
#include "mocks/MockSceneManager.h"
#include "scenes/CharacterScene/CharacterSelectionScene.h"

#include <SFML/Graphics.hpp>

class CharacterSelectionSceneEdgeCasesTest : public ::testing::Test {
 protected:
  MockRenderWindow mockWindow;
  std::shared_ptr<MockAuthManager> mockAuthManager;
  std::shared_ptr<MockAccountManager> mockAccountManager;
  std::unique_ptr<CharacterSelectionScene> characterSelectionScene;
  std::shared_ptr<MockSceneManager> mockSceneManager;

  CharacterSelectionSceneEdgeCasesTest() {
    mockAuthManager = std::make_shared<MockAuthManager>();
    mockAccountManager = std::make_shared<MockAccountManager>(*mockAuthManager);
    mockSceneManager = std::make_shared<MockSceneManager>(mockWindow);
    characterSelectionScene = std::make_unique<CharacterSelectionScene>(
        mockWindow, *mockAuthManager, *mockAccountManager);
  }

  void SetUp() override {
    EXPECT_CALL(*mockAccountManager, listCharacters(::testing::_, ::testing::_))
        .Times(::testing::AnyNumber());
    ASSERT_TRUE(ImGui::SFML::Init(mockWindow, false));
    ImGui::GetIO().DeltaTime = 1.0f / 60.0f;
  }

  void TearDown() override { ImGui::SFML::Shutdown(mockWindow); }
};

TEST_F(CharacterSelectionSceneEdgeCasesTest, OnEnterCalledMultipleTimes) {
  EXPECT_CALL(*mockAccountManager, listCharacters(::testing::_, ::testing::_))
      .Times(2)
      .WillRepeatedly(::testing::Invoke(
          [](std::function<void(Nakama::NStorageObjectListPtr)> successCb,
             std::function<void(const Nakama::NError&)> errorCb) {
            auto emptyList = std::make_shared<Nakama::NStorageObjectList>();
            emptyList->objects = {};
            successCb(emptyList);
          }));
  characterSelectionScene->onEnter(*mockSceneManager);
  characterSelectionScene->onEnter(*mockSceneManager);
  SUCCEED();
  characterSelectionScene->onExit(*mockSceneManager);
}

TEST_F(CharacterSelectionSceneEdgeCasesTest, NoCharactersAvailable) {
  EXPECT_CALL(*mockAccountManager, listCharacters(::testing::_, ::testing::_))
      .WillOnce(::testing::Invoke(
          [](std::function<void(Nakama::NStorageObjectListPtr)> successCb,
             std::function<void(const Nakama::NError&)> errorCb) {
            auto emptyList = std::make_shared<Nakama::NStorageObjectList>();
            emptyList->objects = {};
            successCb(emptyList);
          }));
  characterSelectionScene->onEnter(*mockSceneManager);
  ImGui::SFML::Update(mockWindow, sf::seconds(1.f / 60.f));
  EXPECT_NO_THROW(characterSelectionScene->render(mockWindow));
  characterSelectionScene->onExit(*mockSceneManager);
}

TEST_F(CharacterSelectionSceneEdgeCasesTest, ApiErrorOnListCharacters) {
  EXPECT_CALL(*mockAccountManager, listCharacters(::testing::_, ::testing::_))
      .WillOnce(::testing::Invoke(
          [](std::function<void(Nakama::NStorageObjectListPtr)> successCb,
             std::function<void(const Nakama::NError&)> errorCb) {
            Nakama::NError error;
            error.message = "Network error";
            errorCb(error);
          }));
  characterSelectionScene->onEnter(*mockSceneManager);
  ImGui::SFML::Update(mockWindow, sf::seconds(1.f / 60.f));
  EXPECT_NO_THROW(characterSelectionScene->render(mockWindow));
  characterSelectionScene->onExit(*mockSceneManager);
}

TEST_F(CharacterSelectionSceneEdgeCasesTest, SelectCharacterWithInvalidId) {
  EXPECT_CALL(*mockSceneManager, switchTo(::testing::_)).Times(0);
  characterSelectionScene->onEnter(*mockSceneManager);
  characterSelectionScene->selectCharacterAction("");
  characterSelectionScene->selectCharacterAction("nonexistent_id");
  SUCCEED();
  characterSelectionScene->onExit(*mockSceneManager);
}

TEST_F(CharacterSelectionSceneEdgeCasesTest, OnExitWithoutOnEnter) {
  EXPECT_NO_THROW(characterSelectionScene->onExit(*mockSceneManager));
}

TEST_F(CharacterSelectionSceneEdgeCasesTest, RenderBeforeOnEnter) {
  ImGui::SFML::Update(mockWindow, sf::seconds(1.f / 60.f));
  EXPECT_NO_THROW(characterSelectionScene->render(mockWindow));
}

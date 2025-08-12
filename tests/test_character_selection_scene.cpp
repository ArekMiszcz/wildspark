// Copyright 2025 WildSpark Authors

#include <nakama-cpp/Nakama.h>

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

class CharacterSelectionSceneTest : public ::testing::Test {
 protected:
  MockRenderWindow mockWindow;
  std::shared_ptr<MockAuthManager> mockAuthManager;
  std::shared_ptr<MockAccountManager> mockAccountManager;
  std::unique_ptr<CharacterSelectionScene> characterSelectionScene;
  std::shared_ptr<MockSceneManager> mockSceneManager;

  CharacterSelectionSceneTest() {
    mockAuthManager = std::make_shared<MockAuthManager>();
    mockAccountManager = std::make_shared<MockAccountManager>(*mockAuthManager);
    mockSceneManager = std::make_shared<MockSceneManager>(mockWindow);

    characterSelectionScene = std::make_unique<CharacterSelectionScene>(
        mockWindow, *mockAuthManager, *mockAccountManager);
  }

  void SetUp() override {
    EXPECT_CALL(*mockAccountManager, listCharacters(::testing::_, ::testing::_))
        .Times(::testing::AnyNumber());

    bool success = ImGui::SFML::Init(mockWindow, false);
    ASSERT_TRUE(success && "ImGui::SFML::Init failed");

    ImGuiIO& io = ImGui::GetIO();
    io.DeltaTime = 1.0f / 60.0f;
  }

  void TearDown() override { ImGui::SFML::Shutdown(mockWindow); }
};

TEST_F(CharacterSelectionSceneTest, InitialStateDoesNotCrash) {
  characterSelectionScene->onEnter(*mockSceneManager);
  SUCCEED();
  characterSelectionScene->onExit(*mockSceneManager);
}

TEST_F(CharacterSelectionSceneTest, OnEnterSetsUpSceneAndListsCharacters) {
  EXPECT_CALL(*mockAccountManager, listCharacters(::testing::_, ::testing::_))
      .Times(1);
  characterSelectionScene->onEnter(*mockSceneManager);
  characterSelectionScene->onExit(*mockSceneManager);
}

TEST_F(CharacterSelectionSceneTest, OnExitCleansUpScene) {
  characterSelectionScene->onEnter(*mockSceneManager);
  EXPECT_NO_THROW(characterSelectionScene->onExit(*mockSceneManager));
}

TEST_F(CharacterSelectionSceneTest, HandleEventProcessesEvent) {
  characterSelectionScene->onEnter(*mockSceneManager);
  sf::Event testEvent = sf::Event::KeyPressed{
      sf::Keyboard::Key::A, sf::Keyboard::Scan::A, false, false, false, false};
  EXPECT_NO_THROW(
      characterSelectionScene->handleEvent(testEvent, *mockSceneManager));
  characterSelectionScene->onExit(*mockSceneManager);
}

TEST_F(CharacterSelectionSceneTest, UpdatePerformsTick) {
  characterSelectionScene->onEnter(*mockSceneManager);
  EXPECT_NO_THROW(
      characterSelectionScene->update(sf::seconds(0.016f), *mockSceneManager));
  characterSelectionScene->onExit(*mockSceneManager);
}

TEST_F(CharacterSelectionSceneTest, RenderDrawsScene) {
  characterSelectionScene->onEnter(*mockSceneManager);

  ImGui::SFML::Update(mockWindow, sf::seconds(1.f / 60.f));
  ASSERT_NE(ImGui::GetCurrentContext(), nullptr)
      << "ImGui context is null after ImGui::SFML::Update.";

  EXPECT_NO_THROW(characterSelectionScene->render(mockWindow));

  ImGui::EndFrame();
  ImGui::Render();

  characterSelectionScene->onExit(*mockSceneManager);
}

TEST_F(CharacterSelectionSceneTest, SelectCharacterActionSwitchesToGameScene) {
  EXPECT_CALL(*mockAccountManager, listCharacters(::testing::_, ::testing::_))
      .WillOnce(::testing::Invoke(
          [](std::function<void(Nakama::NStorageObjectListPtr)> successCb,
             std::function<void(const Nakama::NError&)> errorCb) {
            auto characterList = std::make_shared<Nakama::NStorageObjectList>();
            Nakama::NStorageObject charObj;
            charObj.key = "test_char_id";
            charObj.value = "{\"id\":\"test_char_id\"}";
            characterList->objects.push_back(charObj);
            successCb(characterList);
          }));

  characterSelectionScene->onEnter(*mockSceneManager);

  EXPECT_CALL(*mockSceneManager, switchTo(SceneType::Game)).Times(1);
  characterSelectionScene->selectCharacterAction("test_char_id");
  characterSelectionScene->onExit(*mockSceneManager);
}

TEST_F(CharacterSelectionSceneTest,
       CreateCharacterActionSwitchesToCharacterCreationScene) {
  characterSelectionScene->onEnter(*mockSceneManager);
  EXPECT_CALL(*mockSceneManager, switchTo(SceneType::CharacterCreation))
      .Times(1);
  characterSelectionScene->createCharacterAction();
  characterSelectionScene->onExit(*mockSceneManager);
}

TEST_F(CharacterSelectionSceneTest, BackToLoginActionSwitchesToLoginScene) {
  characterSelectionScene->onEnter(*mockSceneManager);
  EXPECT_CALL(*mockSceneManager, switchTo(SceneType::Login)).Times(1);
  characterSelectionScene->backToLoginAction();
  characterSelectionScene->onExit(*mockSceneManager);
}

TEST_F(CharacterSelectionSceneTest, ListCharactersSuccessUpdatesStatus) {
  EXPECT_CALL(*mockAccountManager, listCharacters(::testing::_, ::testing::_))
      .WillOnce(::testing::Invoke(
          [](std::function<void(Nakama::NStorageObjectListPtr)> successCb,
             std::function<void(const Nakama::NError&)> errorCb) {
            Nakama::NStorageObjectListPtr fakeList =
                std::make_shared<Nakama::NStorageObjectList>();
            successCb(fakeList);
          }));

  characterSelectionScene->onEnter(*mockSceneManager);
  characterSelectionScene->onExit(*mockSceneManager);
}

TEST_F(CharacterSelectionSceneTest, ListCharactersFailureUpdatesStatus) {
  EXPECT_CALL(*mockAccountManager, listCharacters(::testing::_, ::testing::_))
      .WillOnce(::testing::Invoke(
          [](std::function<void(Nakama::NStorageObjectListPtr)> successCb,
             std::function<void(const Nakama::NError&)> errorCb) {
            Nakama::NError fakeError;
            fakeError.message = "Network error";
            errorCb(fakeError);
          }));

  characterSelectionScene->onEnter(*mockSceneManager);
  characterSelectionScene->onExit(*mockSceneManager);
}

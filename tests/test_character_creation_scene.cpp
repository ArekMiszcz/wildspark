// Copyright 2025 WildSpark Authors

#include <memory>
#include <string>

#include "gmock/gmock.h"
#include "gtest/gtest.h"
#include "imgui-SFML.h"
#include "imgui.h"
#include "mocks/MockAccountManager.h"
#include "mocks/MockAuthManager.h"
#include "mocks/MockRenderWindow.h"
#include "mocks/MockSceneManager.h"
#include "scenes/CharacterScene/CharacterCreationScene.h"

#include <SFML/Graphics.hpp>

class CharacterCreationSceneTest : public ::testing::Test {
 protected:
  MockRenderWindow mockWindow;
  std::shared_ptr<MockAuthManager> mockAuthManager;
  std::shared_ptr<MockAccountManager> mockAccountManager;
  std::unique_ptr<CharacterCreationScene> characterCreationScene;
  std::shared_ptr<MockSceneManager> mockSceneManager;

  CharacterCreationSceneTest() {
    mockAuthManager = std::make_shared<MockAuthManager>();
    mockAccountManager = std::make_shared<MockAccountManager>(*mockAuthManager);
    mockSceneManager = std::make_shared<MockSceneManager>(mockWindow);
    characterCreationScene = std::make_unique<CharacterCreationScene>(
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

  void SetInitialCharacterFormState(const char* name, int sexIndex) {
    strncpy(characterCreationScene->characterName, name,
            sizeof(characterCreationScene->characterName) - 1);
    characterCreationScene
        ->characterName[sizeof(characterCreationScene->characterName) - 1] =
        '\0';
    characterCreationScene->selectedSexIndex = sexIndex;
  }

  char GetCharacterNameFirstChar() const {
    return characterCreationScene->characterName[0];
  }

  int GetSelectedSexIndex() const {
    return characterCreationScene->selectedSexIndex;
  }
};

TEST_F(CharacterCreationSceneTest, InitialStateDoesNotCrash) {
  characterCreationScene->onEnter(*mockSceneManager);
  SUCCEED();
  characterCreationScene->onExit(*mockSceneManager);
}

TEST_F(CharacterCreationSceneTest, OnEnterResetsForm) {
  SetInitialCharacterFormState("OldName", 1);

  characterCreationScene->onEnter(*mockSceneManager);

  ASSERT_EQ(GetCharacterNameFirstChar(), '\0');
  ASSERT_EQ(GetSelectedSexIndex(), 0);

  characterCreationScene->onExit(*mockSceneManager);
}

TEST_F(CharacterCreationSceneTest, HandleEventProcessesEvent) {
  characterCreationScene->onEnter(*mockSceneManager);
  sf::Event testEvent = sf::Event::KeyPressed{
      sf::Keyboard::Key::A, sf::Keyboard::Scan::A, false, false, false, false};
  EXPECT_NO_THROW(
      characterCreationScene->handleEvent(testEvent, *mockSceneManager));
  characterCreationScene->onExit(*mockSceneManager);
}

TEST_F(CharacterCreationSceneTest, UpdatePerformsTick) {
  characterCreationScene->onEnter(*mockSceneManager);
  EXPECT_NO_THROW(
      characterCreationScene->update(sf::seconds(0.016f), *mockSceneManager));
  characterCreationScene->onExit(*mockSceneManager);
}

TEST_F(CharacterCreationSceneTest, RenderDrawsScene) {
  characterCreationScene->onEnter(*mockSceneManager);
  ImGui::SFML::Update(mockWindow,
                      sf::seconds(1.f / 60.f));  // Update ImGui-SFML state

  EXPECT_NO_THROW(characterCreationScene->render(mockWindow));

  characterCreationScene->onExit(*mockSceneManager);
}

TEST_F(CharacterCreationSceneTest,
       SaveCharacterActionCallsAccountManagerAndSwitchesSceneOnSuccess) {
  characterCreationScene->onEnter(*mockSceneManager);

  SetInitialCharacterFormState("ValidName", 0);

  EXPECT_CALL(*mockAccountManager, saveCharacter(::testing::StrEq("ValidName"),
                                                 ::testing::StrEq("Male"),
                                                 ::testing::_, ::testing::_))
      .WillOnce(::testing::Invoke(
          [](const std::string& name, const std::string& sex,
             std::function<void(const Nakama::NStorageObjectAcks&)> successCb,
             std::function<void(const Nakama::NError&)> errorCb) {
            Nakama::NStorageObjectAcks fakeAcks;
            Nakama::NStorageObjectAck ack;
            ack.collection = "characters";
            ack.key = "new_char_id";
            ack.userId = "user_id";
            ack.version = "version_token";
            fakeAcks.push_back(ack);
            successCb(fakeAcks);
          }));
  EXPECT_CALL(*mockSceneManager, switchTo(SceneType::CharacterSelection))
      .Times(1);

  characterCreationScene->saveCharacterAction();
  characterCreationScene->onExit(*mockSceneManager);
}

TEST_F(CharacterCreationSceneTest, SaveCharacterActionHandlesFailure) {
  characterCreationScene->onEnter(*mockSceneManager);

  SetInitialCharacterFormState("AnotherName", 0);

  EXPECT_CALL(
      *mockAccountManager,
      saveCharacter(::testing::StrEq("AnotherName"), ::testing::StrEq("Male"),
                    ::testing::_, ::testing::_))
      .WillOnce(::testing::Invoke(
          [](const std::string& name, const std::string& sex,
             std::function<void(const Nakama::NStorageObjectAcks&)> successCb,
             std::function<void(const Nakama::NError&)> errorCb) {
            Nakama::NError fakeError;
            fakeError.message = "Failed to save";
            errorCb(fakeError);
          }));
  EXPECT_CALL(*mockSceneManager, switchTo(::testing::_)).Times(0);

  characterCreationScene->saveCharacterAction();
  characterCreationScene->onExit(*mockSceneManager);
}

TEST_F(CharacterCreationSceneTest,
       SaveCharacterActionWithEmptyNameDoesNotCallManager) {
  characterCreationScene->onEnter(
      *mockSceneManager);  // This should clear characterName

  ASSERT_EQ(GetCharacterNameFirstChar(), '\0');

  EXPECT_CALL(*mockAccountManager, saveCharacter(::testing::_, ::testing::_,
                                                 ::testing::_, ::testing::_))
      .Times(0);
  EXPECT_CALL(*mockSceneManager, switchTo(::testing::_)).Times(0);

  characterCreationScene->saveCharacterAction();

  characterCreationScene->onExit(*mockSceneManager);
}

TEST_F(CharacterCreationSceneTest, BackToSelectionActionSwitchesScene) {
  characterCreationScene->onEnter(*mockSceneManager);

  EXPECT_CALL(*mockSceneManager, switchTo(SceneType::CharacterSelection))
      .Times(1);

  characterCreationScene->backToSelectionAction();

  characterCreationScene->onExit(*mockSceneManager);
}

TEST_F(CharacterCreationSceneTest, SaveCharacterActionUsesFormInputValues) {
  characterCreationScene->onEnter(*mockSceneManager);

  const char* testName = "TestCharacter";
  SetInitialCharacterFormState(testName, 1);  // Female

  EXPECT_CALL(*mockAccountManager, saveCharacter(::testing::StrEq(testName),
                                                 ::testing::StrEq("Female"),
                                                 ::testing::_, ::testing::_))
      .WillOnce(::testing::Invoke(
          [](const std::string& name, const std::string& sex,
             std::function<void(const Nakama::NStorageObjectAcks&)> successCb,
             std::function<void(const Nakama::NError&)> errorCb) {
            Nakama::NStorageObjectAcks fakeAcks;
            Nakama::NStorageObjectAck ack;
            ack.key = "test_char_id";
            fakeAcks.push_back(ack);
            successCb(fakeAcks);
          }));
  EXPECT_CALL(*mockSceneManager, switchTo(SceneType::CharacterSelection))
      .Times(1);

  characterCreationScene->saveCharacterAction();

  characterCreationScene->onExit(*mockSceneManager);
}

TEST_F(CharacterCreationSceneTest, RenderUsesFormInputValues) {
  characterCreationScene->onEnter(*mockSceneManager);

  const char* testName = "RenderTestChar";
  SetInitialCharacterFormState(testName, 0);  // Male

  ImGui::SFML::Update(mockWindow, sf::seconds(1.f / 60.f));

  EXPECT_NO_THROW(characterCreationScene->render(mockWindow));

  characterCreationScene->onExit(*mockSceneManager);
}

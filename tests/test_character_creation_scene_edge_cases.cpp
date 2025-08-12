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

class CharacterCreationSceneEdgeCasesTest : public ::testing::Test {
 protected:
  MockRenderWindow mockWindow;
  std::shared_ptr<MockAuthManager> mockAuthManager;
  std::shared_ptr<MockAccountManager> mockAccountManager;
  std::unique_ptr<CharacterCreationScene> characterCreationScene;
  std::shared_ptr<MockSceneManager> mockSceneManager;

  CharacterCreationSceneEdgeCasesTest() {
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

TEST_F(CharacterCreationSceneEdgeCasesTest, SaveCharacterWithLongName) {
  characterCreationScene->onEnter(*mockSceneManager);

  const size_t characterNameBufferSize = 128;
  std::string originalLongName(characterNameBufferSize + 100, 'A');
  std::string expectedTruncatedName =
      originalLongName.substr(0, characterNameBufferSize - 1);

  SetInitialCharacterFormState(originalLongName.c_str(), 0);

  EXPECT_CALL(
      *mockAccountManager,
      saveCharacter(::testing::StrEq(expectedTruncatedName),
                    ::testing::StrEq("Male"), ::testing::_, ::testing::_))
      .Times(1)
      .WillOnce(::testing::Invoke(
          [](const std::string&, const std::string&,
             std::function<void(const Nakama::NStorageObjectAcks&)> successCb,
             std::function<void(const Nakama::NError&)>) {
            Nakama::NStorageObjectAcks acks;
            Nakama::NStorageObjectAck ack;
            ack.collection = "characters";
            ack.key = "someKey";
            acks.push_back(ack);
            successCb(acks);
          }));

  EXPECT_CALL(*mockSceneManager, switchTo(SceneType::CharacterSelection))
      .Times(1);

  characterCreationScene->saveCharacterAction();
  characterCreationScene->onExit(*mockSceneManager);
}

TEST_F(CharacterCreationSceneEdgeCasesTest,
       SaveCharacterWithSpecialCharactersInName) {
  characterCreationScene->onEnter(*mockSceneManager);
  SetInitialCharacterFormState("N@m# W!th $p3c!@l Ch@rs", 1);

  EXPECT_CALL(
      *mockAccountManager,
      saveCharacter(::testing::StrEq("N@m# W!th $p3c!@l Ch@rs"),
                    ::testing::StrEq("Female"), ::testing::_, ::testing::_))
      .Times(1)
      .WillOnce(::testing::Invoke(
          [](const std::string&, const std::string&,
             std::function<void(const Nakama::NStorageObjectAcks&)> successCb,
             std::function<void(const Nakama::NError&)>) {
            Nakama::NStorageObjectAcks acks;
            Nakama::NStorageObjectAck ack;
            ack.key = "N@m# W!th $p3c!@l Ch@rs";
            acks.push_back(ack);
            successCb(acks);
          }));

  EXPECT_CALL(*mockSceneManager, switchTo(SceneType::CharacterSelection))
      .Times(1);

  characterCreationScene->saveCharacterAction();
  characterCreationScene->onExit(*mockSceneManager);
}

TEST_F(CharacterCreationSceneEdgeCasesTest, SaveCharacterApiError) {
  characterCreationScene->onEnter(*mockSceneManager);
  SetInitialCharacterFormState("TestCharacter", 0);

  EXPECT_CALL(
      *mockAccountManager,
      saveCharacter(::testing::StrEq("TestCharacter"), ::testing::StrEq("Male"),
                    ::testing::_, ::testing::_))
      .WillOnce(::testing::Invoke(
          [](const std::string&, const std::string&,
             std::function<void(const Nakama::NStorageObjectAcks&)>,
             std::function<void(const Nakama::NError&)> errorCb) {
            Nakama::NError error;
            error.message = "Failed to save character";
            errorCb(error);
          }));

  EXPECT_CALL(*mockSceneManager, switchTo(::testing::_)).Times(0);

  characterCreationScene->saveCharacterAction();
  characterCreationScene->onExit(*mockSceneManager);
}

TEST_F(CharacterCreationSceneEdgeCasesTest, OnExitWithoutOnEnter) {
  characterCreationScene->onExit(*mockSceneManager);
}

TEST_F(CharacterCreationSceneEdgeCasesTest, RenderBeforeOnEnter) {
  ImGui::SFML::Update(mockWindow, sf::seconds(1.f / 60.f));
  characterCreationScene->render(mockWindow);
}

TEST_F(CharacterCreationSceneEdgeCasesTest, SaveCharacterWithSpacesAsName) {
  characterCreationScene->onEnter(*mockSceneManager);
  SetInitialCharacterFormState("   ", 0);

  EXPECT_CALL(*mockAccountManager, saveCharacter(::testing::_, ::testing::_,
                                                 ::testing::_, ::testing::_))
      .Times(0);
  EXPECT_CALL(*mockSceneManager, switchTo(::testing::_)).Times(0);

  characterCreationScene->saveCharacterAction();
  characterCreationScene->onExit(*mockSceneManager);
}

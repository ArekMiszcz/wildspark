#include "gtest/gtest.h"
#include "gmock/gmock.h"
#include "scenes/CharacterScene/CharacterCreationScene.h"
#include "mocks/MockRenderWindow.h"
#include "mocks/MockSceneManager.h"
#include "mocks/MockAuthManager.h"
#include "mocks/MockAccountManager.h"
#include <SFML/Graphics.hpp>
#include <memory>
#include "imgui.h"
#include "imgui-SFML.h"

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
        characterCreationScene = std::make_unique<CharacterCreationScene>(mockWindow, *mockAuthManager, *mockAccountManager);
    }

    void SetUp() override {
        EXPECT_CALL(*mockAccountManager, listCharacters(::testing::_, ::testing::_))
            .Times(::testing::AnyNumber());

        bool success = ImGui::SFML::Init(mockWindow, false);
        ASSERT_TRUE(success && "ImGui::SFML::Init failed");

        ImGuiIO& io = ImGui::GetIO();
        io.DeltaTime = 1.0f / 60.0f;
    }

    void TearDown() override {
        ImGui::SFML::Shutdown(mockWindow);
    }

    void SetInitialCharacterFormState(const char* name, int sexIndex) {
        strncpy(characterCreationScene->characterName, name, sizeof(characterCreationScene->characterName) - 1);
        characterCreationScene->characterName[sizeof(characterCreationScene->characterName) - 1] = '\0';
        characterCreationScene->selectedSexIndex = sexIndex;
    }

    char GetCharacterNameFirstChar() const {
        return characterCreationScene->characterName[0];
    }

    int GetSelectedSexIndex() const {
        return characterCreationScene->selectedSexIndex;
    }
};

// Edge Case: Attempt to save character with an extremely long name
// This test now verifies that if a name longer than the buffer is provided,
// the scene processes the truncated name (as ImGui's InputText would typically provide)
// and attempts to save it.
TEST_F(CharacterCreationSceneEdgeCasesTest, SaveCharacterWithLongName) {
    characterCreationScene->onEnter(*mockSceneManager);
    
    const size_t characterNameBufferSize = 128; // From CharacterCreationScene.h
    std::string originalLongName(characterNameBufferSize + 100, 'A'); // Name longer than buffer
    // Determine what the name will be after truncation by SetInitialCharacterFormState (or ImGui)
    std::string expectedTruncatedName = originalLongName.substr(0, characterNameBufferSize - 1);

    SetInitialCharacterFormState(originalLongName.c_str(), 0); // Sex: "Male"

    // Expect saveCharacter to be called with the truncated name
    EXPECT_CALL(*mockAccountManager, saveCharacter(
        ::testing::StrEq(expectedTruncatedName), 
        ::testing::StrEq("Male"), 
        ::testing::_, 
        ::testing::_))
        .Times(1)
        .WillOnce(::testing::Invoke([](const std::string&, const std::string&, 
                                       std::function<void(const Nakama::NStorageObjectAcks&)> successCb, 
                                       std::function<void(const Nakama::NError&)>) {
            // Provide a non-empty Acks object
            Nakama::NStorageObjectAcks acks;
            Nakama::NStorageObjectAck ack;
            ack.collection = "characterCollection"; // Example value
            ack.key = "someKey";                   // Example value
            ack.userId = "someUserId";               // Example value
            ack.version = "version";                 // Example value
            acks.push_back(ack);
            successCb(acks);
        }));
    
    // Expect scene to switch to CharacterSelection on successful save
    EXPECT_CALL(*mockSceneManager, switchTo(SceneType::CharacterSelection)).Times(1);
    
    characterCreationScene->saveCharacterAction(); // Attempt to save
    
    characterCreationScene->onExit(*mockSceneManager);
}

// Edge Case: Attempt to save character with special characters in name
TEST_F(CharacterCreationSceneEdgeCasesTest, SaveCharacterWithSpecialCharactersInName) {
    characterCreationScene->onEnter(*mockSceneManager);
    SetInitialCharacterFormState("N@m# W!th $p3c!@l Ch@rs", 1); // Use helper "Female"

    // Depending on validation rules, this might be allowed or disallowed.
    // Assuming it's allowed for this test.
    EXPECT_CALL(*mockAccountManager, saveCharacter(::testing::StrEq("N@m# W!th $p3c!@l Ch@rs"), ::testing::StrEq("Female"), ::testing::_, ::testing::_))
        .Times(1)
        .WillOnce(::testing::Invoke([](const std::string&, const std::string&, 
                                       std::function<void(const Nakama::NStorageObjectAcks&)> successCb, 
                                       std::function<void(const Nakama::NError&)>) {
            // Provide a non-empty Acks object
            Nakama::NStorageObjectAcks acks;
            Nakama::NStorageObjectAck ack;
            ack.collection = "characterCollection"; // Example value
            ack.key = "N@m# W!th $p3c!@l Ch@rs";    // Example value
            ack.userId = "someUserId";               // Example value
            ack.version = "version";                 // Example value
            acks.push_back(ack);
            successCb(acks);
        }));
    
    EXPECT_CALL(*mockSceneManager, switchTo(SceneType::CharacterSelection)).Times(1);

    characterCreationScene->saveCharacterAction();
    characterCreationScene->onExit(*mockSceneManager);
}

// Edge Case: API error during character save
TEST_F(CharacterCreationSceneEdgeCasesTest, SaveCharacterApiError) {
    characterCreationScene->onEnter(*mockSceneManager);
    SetInitialCharacterFormState("TestCharacter", 0); // Use helper "Male"

    EXPECT_CALL(*mockAccountManager, saveCharacter(::testing::StrEq("TestCharacter"), ::testing::StrEq("Male"), ::testing::_, ::testing::_))
        .Times(1)
        .WillOnce(::testing::Invoke([](const std::string&, const std::string&,
                                       std::function<void(const Nakama::NStorageObjectAcks&)> successCb,
                                       std::function<void(const Nakama::NError&)> errorCb) {
            Nakama::NError error;
            error.message = "Failed to save character";
            errorCb(error);
        }));
    
    EXPECT_CALL(*mockSceneManager, switchTo(::testing::_)).Times(0); // Should not switch scene on error

    characterCreationScene->saveCharacterAction();
    // Add assertions here if the UI should show an error message
    // For example, check characterCreationScene->statusMessage
    // EXPECT_THAT(characterCreationScene->statusMessage, ::testing::HasSubstr("Failed to save character"));
    characterCreationScene->onExit(*mockSceneManager);
}

// Edge Case: Calling onExit without onEnter
TEST_F(CharacterCreationSceneEdgeCasesTest, OnExitWithoutOnEnter) {
    EXPECT_NO_THROW(characterCreationScene->onExit(*mockSceneManager));
    SUCCEED();
}

// Edge Case: Render before onEnter
TEST_F(CharacterCreationSceneEdgeCasesTest, RenderBeforeOnEnter) {
    ImGui::SFML::Update(mockWindow, sf::seconds(1.f / 60.f)); // Update ImGui state
    EXPECT_NO_THROW(characterCreationScene->render(mockWindow));
    SUCCEED();
}

// Edge Case: Input field contains only spaces
TEST_F(CharacterCreationSceneEdgeCasesTest, SaveCharacterWithSpacesAsName) {
    characterCreationScene->onEnter(*mockSceneManager);
    SetInitialCharacterFormState("   ", 0); // Use helper "Male"

    // Assuming names with only spaces are invalid and save should not be attempted.
    EXPECT_CALL(*mockAccountManager, saveCharacter(::testing::_, ::testing::_, ::testing::_, ::testing::_))
        .Times(0);
    EXPECT_CALL(*mockSceneManager, switchTo(::testing::_)).Times(0);

    characterCreationScene->saveCharacterAction();
    // Optionally, check for a status message indicating invalid name.
    // EXPECT_THAT(characterCreationScene->statusMessage, ::testing::HasSubstr("Character name cannot be empty or just spaces"));
    SUCCEED();
    characterCreationScene->onExit(*mockSceneManager);
}

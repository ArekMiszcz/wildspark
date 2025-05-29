#include "gtest/gtest.h"
#include "gmock/gmock.h"
#include "scenes/CharacterScene/CharacterSelectionScene.h"
#include "mocks/MockRenderWindow.h"
#include "mocks/MockSceneManager.h"
#include "mocks/MockAuthManager.h"
#include "mocks/MockAccountManager.h"
#include <SFML/Graphics.hpp>
#include <memory>
#include "imgui.h"
#include "imgui-SFML.h"

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
        characterSelectionScene = std::make_unique<CharacterSelectionScene>(mockWindow, *mockAuthManager, *mockAccountManager);
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
};

// Edge Case: onEnter is called multiple times
TEST_F(CharacterSelectionSceneEdgeCasesTest, OnEnterCalledMultipleTimes) {
    EXPECT_CALL(*mockAccountManager, listCharacters(::testing::_, ::testing::_))
        .Times(2) // Expect listCharacters to be called twice
        .WillRepeatedly(::testing::Invoke([](std::function<void(Nakama::NStorageObjectListPtr)> successCb,
                                              std::function<void(const Nakama::NError&)> errorCb) {
            auto emptyList = std::make_shared<Nakama::NStorageObjectList>();
            emptyList->objects = {}; // Ensure it's an empty list
            successCb(emptyList);
        }));

    characterSelectionScene->onEnter(*mockSceneManager); // First call
    characterSelectionScene->onEnter(*mockSceneManager); // Second call
    // No crash and listCharacters was called twice.
    SUCCEED();
    characterSelectionScene->onExit(*mockSceneManager);
}

// Edge Case: No characters available
TEST_F(CharacterSelectionSceneEdgeCasesTest, NoCharactersAvailable) {
    EXPECT_CALL(*mockAccountManager, listCharacters(::testing::_, ::testing::_))
        .WillOnce(::testing::Invoke([](std::function<void(Nakama::NStorageObjectListPtr)> successCb,
                                         std::function<void(const Nakama::NError&)> errorCb) {
            auto emptyList = std::make_shared<Nakama::NStorageObjectList>();
            emptyList->objects = {}; // Ensure it's an empty list
            successCb(emptyList);
        }));

    characterSelectionScene->onEnter(*mockSceneManager);
    // Render to process ImGui state
    ImGui::SFML::Update(mockWindow, sf::seconds(1.f / 60.f));
    EXPECT_NO_THROW(characterSelectionScene->render(mockWindow));
    // Potentially check for "No characters found" message if UI reflects this
    characterSelectionScene->onExit(*mockSceneManager);
}

// Edge Case: API error when listing characters
TEST_F(CharacterSelectionSceneEdgeCasesTest, ApiErrorOnListCharacters) {
    EXPECT_CALL(*mockAccountManager, listCharacters(::testing::_, ::testing::_))
        .WillOnce(::testing::Invoke([](std::function<void(Nakama::NStorageObjectListPtr)> successCb,
                                         std::function<void(const Nakama::NError&)> errorCb) {
            Nakama::NError error;
            error.message = "Network error";
            errorCb(error);
        }));

    characterSelectionScene->onEnter(*mockSceneManager);
    // Render to process ImGui state
    ImGui::SFML::Update(mockWindow, sf::seconds(1.f / 60.f));
    EXPECT_NO_THROW(characterSelectionScene->render(mockWindow));
    // Potentially check for error message in UI
    characterSelectionScene->onExit(*mockSceneManager);
}

// Edge Case: Select character with an invalid/empty ID
TEST_F(CharacterSelectionSceneEdgeCasesTest, SelectCharacterWithInvalidId) {
    // Assuming selectCharacterAction might try to switch scene or similar
    // For an invalid ID, it probably shouldn't switch or crash.
    EXPECT_CALL(*mockSceneManager, switchTo(::testing::_)).Times(0);
    characterSelectionScene->onEnter(*mockSceneManager); // To set sceneManagerRef
    characterSelectionScene->selectCharacterAction(""); // Empty ID
    characterSelectionScene->selectCharacterAction("nonexistent_id"); // Non-existent ID
    SUCCEED(); // If no crash and no scene switch, it's a pass for this basic check
    characterSelectionScene->onExit(*mockSceneManager);
}

// Edge Case: Calling onExit without onEnter
TEST_F(CharacterSelectionSceneEdgeCasesTest, OnExitWithoutOnEnter) {
    // sceneManagerRef might be null, ensure no crash
    EXPECT_NO_THROW(characterSelectionScene->onExit(*mockSceneManager));
    SUCCEED();
}

// Edge Case: Render before onEnter (ImGui might not be fully set up for the scene)
TEST_F(CharacterSelectionSceneEdgeCasesTest, RenderBeforeOnEnter) {
    // This might be problematic if onEnter does critical ImGui setup for the scene
    // For now, just ensure it doesn't crash. Specific ImGui checks would be more involved.
    ImGui::SFML::Update(mockWindow, sf::seconds(1.f / 60.f));
    EXPECT_NO_THROW(characterSelectionScene->render(mockWindow));
    SUCCEED();
}

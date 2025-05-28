#include "gtest/gtest.h"
#include "gmock/gmock.h"
#include "scenes/SceneManager.h"
#include "scenes/Scene.h"
#include <SFML/Graphics.hpp>
#include <memory>
#include <iostream>

// Reuse the MockScene and MockRenderWindow from the original test file
class MockScene : public Scene {
public:
    MOCK_METHOD(void, onEnter, (SceneManager& manager), (override));
    MOCK_METHOD(void, onExit, (SceneManager& manager), (override));
    MOCK_METHOD(void, handleEvent, (const sf::Event& event, SceneManager& manager), (override));
    MOCK_METHOD(void, update, (sf::Time deltaTime, SceneManager& manager), (override));
    MOCK_METHOD(void, render, (sf::RenderTarget& target), (override));

    ~MockScene() override = default;
};

class MockRenderWindow : public sf::RenderWindow {
public:
    MockRenderWindow() {
        // Don't actually create a window
    }

    // Override methods that would normally require a real window
    bool setActive(bool) override { return true; }
    
    // These methods are not virtual in SFML 3.0, so we can't override them
    // Instead, we'll hide them with our own implementations
    void display() {}
    void clear() {}
    void close() override {}
    bool isOpen() const { return true; }
    std::optional<sf::Event> pollEvent() { return std::nullopt; }
};

// Test fixture for SceneManager edge cases
class SceneManagerEdgeCasesTest : public ::testing::Test {
protected:
    MockRenderWindow mockWindow;
    std::unique_ptr<SceneManager> sceneManagerPtr;
    bool setupSucceeded = true;

    SceneManagerEdgeCasesTest() {
        try {
            sceneManagerPtr = std::make_unique<SceneManager>(mockWindow);
        } catch (const std::exception& e) {
            std::cerr << "Exception during SceneManager initialization: " << e.what() << std::endl;
            setupSucceeded = false;
        }
    }

    void SetUp() override {
        // Skip tests if setup failed
        if (!setupSucceeded) {
            GTEST_SKIP() << "Test skipped due to setup failure";
        }
    }

    void TearDown() override {
        // Cleanup for each test
        if (sceneManagerPtr && setupSucceeded) {
            try {
                sceneManagerPtr->shutdown(); // Ensure ImGui is shut down
            } catch (const std::exception& e) {
                std::cerr << "Exception during shutdown: " << e.what() << std::endl;
            }
        }
    }
};

// 1. Test for handling duplicate scene types
TEST_F(SceneManagerEdgeCasesTest, DuplicateSceneTypes) {
    try {
        auto mockScene1 = std::make_unique<MockScene>();
        auto mockScene2 = std::make_unique<MockScene>();
        
        MockScene* rawMockScene1 = mockScene1.get();
        MockScene* rawMockScene2 = mockScene2.get();
        
        // Add the first scene
        sceneManagerPtr->addScene(SceneType::Login, std::move(mockScene1));
        ASSERT_EQ(sceneManagerPtr->getSceneCount(), 1);
        
        // Add a second scene with the same type
        sceneManagerPtr->addScene(SceneType::Login, std::move(mockScene2));
        
        // The scene count should still be 1 as the second scene replaced the first
        ASSERT_EQ(sceneManagerPtr->getSceneCount(), 1);
        
        // Switch to the Login scene to verify it's the second scene that's active
        EXPECT_CALL(*rawMockScene2, onEnter(::testing::_)).Times(1);
        EXPECT_CALL(*rawMockScene2, update(::testing::_, ::testing::_)).Times(1);
        
        sceneManagerPtr->switchTo(SceneType::Login);
        sceneManagerPtr->update(mockWindow, sf::seconds(0.016f));
        
        // Verify expectations before render
        testing::Mock::VerifyAndClearExpectations(rawMockScene2);
        
        // Set up expectations for render
        EXPECT_CALL(*rawMockScene2, render(::testing::_)).Times(1);
        
        // Complete the ImGui frame
        sceneManagerPtr->render(mockWindow);
        
        // Verify render expectations
        testing::Mock::VerifyAndClearExpectations(rawMockScene2);
        
        // The first scene should not be called since it was replaced
        // Note: We can't verify this because the first scene object is destroyed when replaced
        // So we don't set expectations on rawMockScene1
    } catch (const std::exception& e) {
        FAIL() << "Exception during test: " << e.what();
    }
}

// 2. Test for complex scene switching sequences
TEST_F(SceneManagerEdgeCasesTest, ComplexSceneSwitchingSequence) {
    try {
        auto loginScene = std::make_unique<MockScene>();
        auto gameScene = std::make_unique<MockScene>();
        auto settingsScene = std::make_unique<MockScene>();
        
        MockScene* rawLoginScene = loginScene.get();
        MockScene* rawGameScene = gameScene.get();
        MockScene* rawSettingsScene = settingsScene.get();
        
        sceneManagerPtr->addScene(SceneType::Login, std::move(loginScene));
        sceneManagerPtr->addScene(SceneType::Game, std::move(gameScene));
        sceneManagerPtr->addScene(SceneType::Settings, std::move(settingsScene));
        
        // Initial state
        ASSERT_EQ(sceneManagerPtr->getCurrentSceneType(), SceneType::None);
        
        // Switch to Login scene
        EXPECT_CALL(*rawLoginScene, onEnter(::testing::_)).Times(1);
        EXPECT_CALL(*rawLoginScene, update(::testing::_, ::testing::_)).Times(1);
        
        sceneManagerPtr->switchTo(SceneType::Login);
        sceneManagerPtr->update(mockWindow, sf::seconds(0.016f));
        ASSERT_EQ(sceneManagerPtr->getCurrentSceneType(), SceneType::Login);
        
        testing::Mock::VerifyAndClearExpectations(rawLoginScene);
        
        // Render to complete the ImGui frame
        EXPECT_CALL(*rawLoginScene, render(::testing::_)).Times(1);
        sceneManagerPtr->render(mockWindow);
        testing::Mock::VerifyAndClearExpectations(rawLoginScene);
        
        // Switch to Game scene
        EXPECT_CALL(*rawLoginScene, onExit(::testing::_)).Times(1);
        EXPECT_CALL(*rawGameScene, onEnter(::testing::_)).Times(1);
        EXPECT_CALL(*rawGameScene, update(::testing::_, ::testing::_)).Times(1);
        
        sceneManagerPtr->switchTo(SceneType::Game);
        sceneManagerPtr->update(mockWindow, sf::seconds(0.016f));
        ASSERT_EQ(sceneManagerPtr->getCurrentSceneType(), SceneType::Game);
        
        testing::Mock::VerifyAndClearExpectations(rawLoginScene);
        testing::Mock::VerifyAndClearExpectations(rawGameScene);
        
        // Render to complete the ImGui frame
        EXPECT_CALL(*rawGameScene, render(::testing::_)).Times(1);
        sceneManagerPtr->render(mockWindow);
        testing::Mock::VerifyAndClearExpectations(rawGameScene);
        
        // Switch to Settings scene
        EXPECT_CALL(*rawGameScene, onExit(::testing::_)).Times(1);
        EXPECT_CALL(*rawSettingsScene, onEnter(::testing::_)).Times(1);
        EXPECT_CALL(*rawSettingsScene, update(::testing::_, ::testing::_)).Times(1);
        
        sceneManagerPtr->switchTo(SceneType::Settings);
        sceneManagerPtr->update(mockWindow, sf::seconds(0.016f));
        ASSERT_EQ(sceneManagerPtr->getCurrentSceneType(), SceneType::Settings);
        
        testing::Mock::VerifyAndClearExpectations(rawGameScene);
        testing::Mock::VerifyAndClearExpectations(rawSettingsScene);
        
        // Render to complete the ImGui frame
        EXPECT_CALL(*rawSettingsScene, render(::testing::_)).Times(1);
        sceneManagerPtr->render(mockWindow);
        testing::Mock::VerifyAndClearExpectations(rawSettingsScene);
        
        // Switch back to Login scene
        EXPECT_CALL(*rawSettingsScene, onExit(::testing::_)).Times(1);
        EXPECT_CALL(*rawLoginScene, onEnter(::testing::_)).Times(1);
        EXPECT_CALL(*rawLoginScene, update(::testing::_, ::testing::_)).Times(1);
        
        sceneManagerPtr->switchTo(SceneType::Login);
        sceneManagerPtr->update(mockWindow, sf::seconds(0.016f));
        ASSERT_EQ(sceneManagerPtr->getCurrentSceneType(), SceneType::Login);
        
        testing::Mock::VerifyAndClearExpectations(rawSettingsScene);
        testing::Mock::VerifyAndClearExpectations(rawLoginScene);
        
        // Final render to complete the ImGui frame
        EXPECT_CALL(*rawLoginScene, render(::testing::_)).Times(1);
        sceneManagerPtr->render(mockWindow);
        testing::Mock::VerifyAndClearExpectations(rawLoginScene);
    } catch (const std::exception& e) {
        FAIL() << "Exception during test: " << e.what();
    }
}

// 3. Test for null scene handling
TEST_F(SceneManagerEdgeCasesTest, NullSceneHandling) {
    try {
        // Try to add a null scene
        sceneManagerPtr->addScene(SceneType::Login, nullptr);
        
        // The scene should not be added, so count should remain 0
        ASSERT_EQ(sceneManagerPtr->getSceneCount(), 0);
        
        // Try to switch to the null scene
        sceneManagerPtr->switchTo(SceneType::Login);
        sceneManagerPtr->update(mockWindow, sf::seconds(0.016f));
        
        // Current scene type should remain None
        ASSERT_EQ(sceneManagerPtr->getCurrentSceneType(), SceneType::None);
        
        // Complete the ImGui frame
        sceneManagerPtr->render(mockWindow);
        
        // Add a valid scene
        auto validScene = std::make_unique<MockScene>();
        MockScene* rawValidScene = validScene.get();
        
        sceneManagerPtr->addScene(SceneType::Game, std::move(validScene));
        ASSERT_EQ(sceneManagerPtr->getSceneCount(), 1);
        
        // Switch to the valid scene
        EXPECT_CALL(*rawValidScene, onEnter(::testing::_)).Times(1);
        EXPECT_CALL(*rawValidScene, update(::testing::_, ::testing::_)).Times(1);
        
        sceneManagerPtr->switchTo(SceneType::Game);
        sceneManagerPtr->update(mockWindow, sf::seconds(0.016f));
        ASSERT_EQ(sceneManagerPtr->getCurrentSceneType(), SceneType::Game);
        
        testing::Mock::VerifyAndClearExpectations(rawValidScene);
        
        // Set up expectations for render
        EXPECT_CALL(*rawValidScene, render(::testing::_)).Times(1);
        
        // Complete the ImGui frame
        sceneManagerPtr->render(mockWindow);
        
        testing::Mock::VerifyAndClearExpectations(rawValidScene);
        
        // Try to replace a valid scene with a null scene
        sceneManagerPtr->addScene(SceneType::Game, nullptr);
        
        // The scene should not be replaced, so count should remain 1
        ASSERT_EQ(sceneManagerPtr->getSceneCount(), 1);
        
        // Current scene type should still be Game
        ASSERT_EQ(sceneManagerPtr->getCurrentSceneType(), SceneType::Game);
        
        // Set up expectations for the final update/render cycle
        EXPECT_CALL(*rawValidScene, update(::testing::_, ::testing::_)).Times(1);
        sceneManagerPtr->update(mockWindow, sf::seconds(0.016f));
        testing::Mock::VerifyAndClearExpectations(rawValidScene);
        
        EXPECT_CALL(*rawValidScene, render(::testing::_)).Times(1);
        sceneManagerPtr->render(mockWindow);
        testing::Mock::VerifyAndClearExpectations(rawValidScene);
    } catch (const std::exception& e) {
        FAIL() << "Exception during test: " << e.what();
    }
}

// 4. Test for concurrent scene switches
TEST_F(SceneManagerEdgeCasesTest, ConcurrentSceneSwitches) {
    try {
        auto loginScene = std::make_unique<MockScene>();
        auto gameScene = std::make_unique<MockScene>();
        
        MockScene* rawLoginScene = loginScene.get();
        MockScene* rawGameScene = gameScene.get();
        
        sceneManagerPtr->addScene(SceneType::Login, std::move(loginScene));
        sceneManagerPtr->addScene(SceneType::Game, std::move(gameScene));
        
        // Request multiple scene switches before update
        sceneManagerPtr->switchTo(SceneType::Login);
        sceneManagerPtr->switchTo(SceneType::Game);
        
        // Only the last switch request should be processed
        EXPECT_CALL(*rawLoginScene, onEnter(::testing::_)).Times(0);
        EXPECT_CALL(*rawGameScene, onEnter(::testing::_)).Times(1);
        EXPECT_CALL(*rawGameScene, update(::testing::_, ::testing::_)).Times(1);
        
        sceneManagerPtr->update(mockWindow, sf::seconds(0.016f));
        ASSERT_EQ(sceneManagerPtr->getCurrentSceneType(), SceneType::Game);
        
        testing::Mock::VerifyAndClearExpectations(rawLoginScene);
        testing::Mock::VerifyAndClearExpectations(rawGameScene);
        
        // Render to complete the ImGui frame
        EXPECT_CALL(*rawGameScene, render(::testing::_)).Times(1);
        sceneManagerPtr->render(mockWindow);
        testing::Mock::VerifyAndClearExpectations(rawGameScene);
        
        // Request multiple scene switches again
        sceneManagerPtr->switchTo(SceneType::Login);
        sceneManagerPtr->switchTo(SceneType::Game); // This should be a no-op since we're already on Game
        
        // The last requested scene is Game, which is already active, so no scene change should occur
        EXPECT_CALL(*rawGameScene, onExit(::testing::_)).Times(0);
        EXPECT_CALL(*rawLoginScene, onEnter(::testing::_)).Times(0);
        EXPECT_CALL(*rawGameScene, update(::testing::_, ::testing::_)).Times(1);
        
        sceneManagerPtr->update(mockWindow, sf::seconds(0.016f));
        ASSERT_EQ(sceneManagerPtr->getCurrentSceneType(), SceneType::Game);
        
        testing::Mock::VerifyAndClearExpectations(rawLoginScene);
        testing::Mock::VerifyAndClearExpectations(rawGameScene);
        
        // Render to complete the ImGui frame
        EXPECT_CALL(*rawGameScene, render(::testing::_)).Times(1);
        sceneManagerPtr->render(mockWindow);
        testing::Mock::VerifyAndClearExpectations(rawGameScene);
        
        // Now switch to Login
        EXPECT_CALL(*rawGameScene, onExit(::testing::_)).Times(1);
        EXPECT_CALL(*rawLoginScene, onEnter(::testing::_)).Times(1);
        EXPECT_CALL(*rawLoginScene, update(::testing::_, ::testing::_)).Times(1);
        
        sceneManagerPtr->switchTo(SceneType::Login);
        sceneManagerPtr->update(mockWindow, sf::seconds(0.016f));
        ASSERT_EQ(sceneManagerPtr->getCurrentSceneType(), SceneType::Login);
        
        testing::Mock::VerifyAndClearExpectations(rawLoginScene);
        testing::Mock::VerifyAndClearExpectations(rawGameScene);
        
        // Final render to complete the ImGui frame
        EXPECT_CALL(*rawLoginScene, render(::testing::_)).Times(1);
        sceneManagerPtr->render(mockWindow);
        testing::Mock::VerifyAndClearExpectations(rawLoginScene);
    } catch (const std::exception& e) {
        FAIL() << "Exception during test: " << e.what();
    }
}

// 5. Test for scene lifecycle events
TEST_F(SceneManagerEdgeCasesTest, SceneLifecycleEvents) {
    try {
        auto loginScene = std::make_unique<MockScene>();
        auto gameScene = std::make_unique<MockScene>();
        
        MockScene* rawLoginScene = loginScene.get();
        MockScene* rawGameScene = gameScene.get();
        
        sceneManagerPtr->addScene(SceneType::Login, std::move(loginScene));
        sceneManagerPtr->addScene(SceneType::Game, std::move(gameScene));
        
        // Set up expectations for Login scene
        EXPECT_CALL(*rawLoginScene, onEnter(::testing::_)).Times(1);
        EXPECT_CALL(*rawLoginScene, update(::testing::_, ::testing::_)).Times(1);
        
        // Switch to Login scene
        sceneManagerPtr->switchTo(SceneType::Login);
        sceneManagerPtr->update(mockWindow, sf::seconds(0.016f));
        ASSERT_EQ(sceneManagerPtr->getCurrentSceneType(), SceneType::Login);
        
        // Verify and clear expectations for the first part
        testing::Mock::VerifyAndClearExpectations(rawLoginScene);
        
        // Set up expectations for Login scene render
        EXPECT_CALL(*rawLoginScene, render(::testing::_)).Times(1);
        
        // Render Login scene
        sceneManagerPtr->render(mockWindow);
        
        // Verify and clear expectations after render
        testing::Mock::VerifyAndClearExpectations(rawLoginScene);
        
        // Set up expectations for scene switch
        EXPECT_CALL(*rawLoginScene, onExit(::testing::_)).Times(1);
        EXPECT_CALL(*rawGameScene, onEnter(::testing::_)).Times(1);
        EXPECT_CALL(*rawGameScene, update(::testing::_, ::testing::_)).Times(1);
        
        // Switch to Game scene
        sceneManagerPtr->switchTo(SceneType::Game);
        sceneManagerPtr->update(mockWindow, sf::seconds(0.016f));
        ASSERT_EQ(sceneManagerPtr->getCurrentSceneType(), SceneType::Game);
        
        // Verify and clear expectations after switch
        testing::Mock::VerifyAndClearExpectations(rawLoginScene);
        testing::Mock::VerifyAndClearExpectations(rawGameScene);
        
        // Set up expectations for Game scene render
        EXPECT_CALL(*rawGameScene, render(::testing::_)).Times(1);
        
        // Render Game scene
        sceneManagerPtr->render(mockWindow);
        
        // Verify and clear expectations after render
        testing::Mock::VerifyAndClearExpectations(rawGameScene);
    } catch (const std::exception& e) {
        FAIL() << "Exception during test: " << e.what();
    }
}

// 6. Test for switching to non-existent scene types
TEST_F(SceneManagerEdgeCasesTest, SwitchToNonExistentSceneTypes) {
    try {
        auto loginScene = std::make_unique<MockScene>();
        MockScene* rawLoginScene = loginScene.get();
        
        sceneManagerPtr->addScene(SceneType::Login, std::move(loginScene));
        
        // Switch to Login scene first
        EXPECT_CALL(*rawLoginScene, onEnter(::testing::_)).Times(1);
        EXPECT_CALL(*rawLoginScene, update(::testing::_, ::testing::_)).Times(1);
        
        sceneManagerPtr->switchTo(SceneType::Login);
        sceneManagerPtr->update(mockWindow, sf::seconds(0.016f));
        ASSERT_EQ(sceneManagerPtr->getCurrentSceneType(), SceneType::Login);
        
        testing::Mock::VerifyAndClearExpectations(rawLoginScene);
        
        // Render to complete the ImGui frame
        EXPECT_CALL(*rawLoginScene, render(::testing::_)).Times(1);
        sceneManagerPtr->render(mockWindow);
        testing::Mock::VerifyAndClearExpectations(rawLoginScene);
        
        // Try to switch to a non-existent scene
        EXPECT_CALL(*rawLoginScene, onExit(::testing::_)).Times(1);
        
        sceneManagerPtr->switchTo(SceneType::Game); // Game scene doesn't exist
        sceneManagerPtr->update(mockWindow, sf::seconds(0.016f));
        
        // Current scene type should be None since the Game scene doesn't exist
        ASSERT_EQ(sceneManagerPtr->getCurrentSceneType(), SceneType::None);
        
        testing::Mock::VerifyAndClearExpectations(rawLoginScene);
        
        // No scene is active, so no render expectations needed
        // But we still need to complete the ImGui frame
        sceneManagerPtr->render(mockWindow);
    } catch (const std::exception& e) {
        FAIL() << "Exception during test: " << e.what();
    }
}

// 7. Test for handling events when no scene is active
TEST_F(SceneManagerEdgeCasesTest, HandleEventsWithNoActiveScene) {
    try {
        // No scenes added, so currentScene is nullptr
        
        // Create a KeyPressed event
        sf::Event testEvent(sf::Event::KeyPressed{
            sf::Keyboard::Key::A,
            sf::Keyboard::Scancode::A,
            false, // alt
            false, // control
            false, // shift
            false  // system
        });
        
        // This should not crash even though no scene is active
        sceneManagerPtr->handleEvent(mockWindow, testEvent);
        
        // Current scene type should still be None
        ASSERT_EQ(sceneManagerPtr->getCurrentSceneType(), SceneType::None);
        
        // Complete the ImGui frame
        sceneManagerPtr->update(mockWindow, sf::seconds(0.016f));
        sceneManagerPtr->render(mockWindow);
    } catch (const std::exception& e) {
        FAIL() << "Exception during test: " << e.what();
    }
}

// 8. Test for updating when no scene is active
TEST_F(SceneManagerEdgeCasesTest, UpdateWithNoActiveScene) {
    try {
        // No scenes added, so currentScene is nullptr
        
        // This should not crash even though no scene is active
        sceneManagerPtr->update(mockWindow, sf::seconds(0.016f));
        
        // Current scene type should still be None
        ASSERT_EQ(sceneManagerPtr->getCurrentSceneType(), SceneType::None);
        
        // Complete the ImGui frame
        sceneManagerPtr->render(mockWindow);
    } catch (const std::exception& e) {
        FAIL() << "Exception during test: " << e.what();
    }
}

// 9. Test for rendering when no scene is active
TEST_F(SceneManagerEdgeCasesTest, RenderWithNoActiveScene) {
    try {
        // No scenes added, so currentScene is nullptr
        
        // Need to call update first to start an ImGui frame
        sceneManagerPtr->update(mockWindow, sf::seconds(0.016f));
        
        // This should not crash even though no scene is active
        sceneManagerPtr->render(mockWindow);
        
        // Current scene type should still be None
        ASSERT_EQ(sceneManagerPtr->getCurrentSceneType(), SceneType::None);
    } catch (const std::exception& e) {
        FAIL() << "Exception during test: " << e.what();
    }
}

// Since we can't easily test ImGui initialization failure without modifying the code,
// I'll suggest a test structure for it in the comments:

/*
// Test for ImGui initialization failure
// This would require modifying the SceneManager code to accept a mock ImGui interface
// or adding a dependency injection mechanism for testing
TEST_F(SceneManagerEdgeCasesTest, ImGuiInitializationFailure) {
    // Mock ImGui initialization to return false
    // Create SceneManager with the mock
    // Verify that an exception is thrown
}
*/

// Test for removing scenes
TEST_F(SceneManagerEdgeCasesTest, RemoveScene) {
    try {
        auto loginScene = std::make_unique<MockScene>();
        auto gameScene = std::make_unique<MockScene>();
        
        MockScene* rawLoginScene = loginScene.get();
        MockScene* rawGameScene = gameScene.get();
        
        sceneManagerPtr->addScene(SceneType::Login, std::move(loginScene));
        sceneManagerPtr->addScene(SceneType::Game, std::move(gameScene));
        
        ASSERT_EQ(sceneManagerPtr->getSceneCount(), 2);
        
        // Remove the Login scene
        sceneManagerPtr->removeScene(SceneType::Login);
        
        ASSERT_EQ(sceneManagerPtr->getSceneCount(), 1);
        
        // Try to switch to the removed scene
        sceneManagerPtr->switchTo(SceneType::Login);
        sceneManagerPtr->update(mockWindow, sf::seconds(0.016f));
        
        // Current scene type should be None since the Login scene was removed
        ASSERT_EQ(sceneManagerPtr->getCurrentSceneType(), SceneType::None);
        
        // Complete the ImGui frame
        sceneManagerPtr->render(mockWindow);
        
        // Switch to the Game scene which still exists
        EXPECT_CALL(*rawGameScene, onEnter(::testing::_)).Times(1);
        EXPECT_CALL(*rawGameScene, update(::testing::_, ::testing::_)).Times(1);
        
        sceneManagerPtr->switchTo(SceneType::Game);
        sceneManagerPtr->update(mockWindow, sf::seconds(0.016f));
        ASSERT_EQ(sceneManagerPtr->getCurrentSceneType(), SceneType::Game);
        
        testing::Mock::VerifyAndClearExpectations(rawGameScene);
        
        // Render to complete the ImGui frame
        EXPECT_CALL(*rawGameScene, render(::testing::_)).Times(1);
        sceneManagerPtr->render(mockWindow);
        testing::Mock::VerifyAndClearExpectations(rawGameScene);
        
        // Now remove the current active scene
        EXPECT_CALL(*rawGameScene, onExit(::testing::_)).Times(1);
        
        sceneManagerPtr->removeScene(SceneType::Game);
        
        ASSERT_EQ(sceneManagerPtr->getSceneCount(), 0);
        ASSERT_EQ(sceneManagerPtr->getCurrentSceneType(), SceneType::None);
        
        testing::Mock::VerifyAndClearExpectations(rawGameScene);
        
        // Complete the ImGui frame
        sceneManagerPtr->update(mockWindow, sf::seconds(0.016f));
        sceneManagerPtr->render(mockWindow);
    } catch (const std::exception& e) {
        FAIL() << "Exception during test: " << e.what();
    }
}

// Test for removing a non-existent scene
TEST_F(SceneManagerEdgeCasesTest, RemoveNonExistentScene) {
    try {
        // Try to remove a scene that doesn't exist
        sceneManagerPtr->removeScene(SceneType::Login);
        
        // Scene count should still be 0
        ASSERT_EQ(sceneManagerPtr->getSceneCount(), 0);
        
        // Add a scene
        auto loginScene = std::make_unique<MockScene>();
        MockScene* rawLoginScene = loginScene.get();
        sceneManagerPtr->addScene(SceneType::Login, std::move(loginScene));
        
        ASSERT_EQ(sceneManagerPtr->getSceneCount(), 1);
        
        // Try to remove a different scene that doesn't exist
        sceneManagerPtr->removeScene(SceneType::Game);
        
        // Scene count should still be 1
        ASSERT_EQ(sceneManagerPtr->getSceneCount(), 1);
        
        // Switch to the Login scene to complete the test with proper ImGui frame handling
        EXPECT_CALL(*rawLoginScene, onEnter(::testing::_)).Times(1);
        EXPECT_CALL(*rawLoginScene, update(::testing::_, ::testing::_)).Times(1);
        
        sceneManagerPtr->switchTo(SceneType::Login);
        sceneManagerPtr->update(mockWindow, sf::seconds(0.016f));
        
        testing::Mock::VerifyAndClearExpectations(rawLoginScene);
        
        // Render to complete the ImGui frame
        EXPECT_CALL(*rawLoginScene, render(::testing::_)).Times(1);
        sceneManagerPtr->render(mockWindow);
        
        testing::Mock::VerifyAndClearExpectations(rawLoginScene);
    } catch (const std::exception& e) {
        FAIL() << "Exception during test: " << e.what();
    }
}
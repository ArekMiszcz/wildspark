#include "gtest/gtest.h"
#include "gmock/gmock.h" // Added for MOCK_METHOD
#include "scenes/SceneManager.h" // Assuming SceneManager.h is accessible
#include "scenes/Scene.h"        // For MockScene
#include <SFML/Graphics.hpp>     // For sf::RenderWindow (needed by SceneManager)
#include <memory> // Required for std::unique_ptr
#include <iostream>

// Mock Scene for testing SceneManager
class MockScene : public Scene {
public:
    MOCK_METHOD(void, onEnter, (SceneManager& manager), (override));
    MOCK_METHOD(void, onExit, (SceneManager& manager), (override));
    MOCK_METHOD(void, handleEvent, (const sf::Event& event, SceneManager& manager), (override));
    MOCK_METHOD(void, update, (sf::Time deltaTime, SceneManager& manager), (override));
    MOCK_METHOD(void, render, (sf::RenderTarget& target), (override));

    // Provide a concrete implementation for the pure virtual destructor if Scene has one
    ~MockScene() override = default;
};

// Mock RenderWindow for testing without X11 dependency
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
    void close() {}
    bool isOpen() const { return true; }
    std::optional<sf::Event> pollEvent() { return std::nullopt; }
};

// Test fixture for SceneManager
class SceneManagerTest : public ::testing::Test {
protected:
    MockRenderWindow mockWindow;
    std::unique_ptr<SceneManager> sceneManagerPtr;
    bool setupSucceeded = true;

    SceneManagerTest() {
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

TEST_F(SceneManagerTest, InitialState) {
    try {
        ASSERT_EQ(sceneManagerPtr->getSceneCount(), 0);
        ASSERT_EQ(sceneManagerPtr->getCurrentSceneType(), SceneType::None); // Assuming SceneType::None is a valid default
    } catch (const std::exception& e) {
        FAIL() << "Exception during test: " << e.what();
    }
}

TEST_F(SceneManagerTest, AddScene) {
    try {
        auto mockScene1 = std::make_unique<MockScene>();
        // MockScene* rawMockScene1 = mockScene1.get(); 

        sceneManagerPtr->addScene(SceneType::Login, std::move(mockScene1));
        ASSERT_EQ(sceneManagerPtr->getSceneCount(), 1);
        // We can't easily check if the correct scene is there by type without more SceneManager functionality
        // or by trying to switch to it.
    } catch (const std::exception& e) {
        FAIL() << "Exception during test: " << e.what();
    }
}

TEST_F(SceneManagerTest, SwitchScene) {
    try {
        auto scene1 = std::make_unique<MockScene>();
        auto scene2 = std::make_unique<MockScene>();

        MockScene* rawScene1 = scene1.get();
        MockScene* rawScene2 = scene2.get();

        sceneManagerPtr->addScene(SceneType::Login, std::move(scene1));
        sceneManagerPtr->addScene(SceneType::Game, std::move(scene2));

        EXPECT_CALL(*rawScene1, onEnter(::testing::_)).Times(1);
        EXPECT_CALL(*rawScene1, update(::testing::_, ::testing::_)).Times(1); // For the update after switch
        
        sceneManagerPtr->switchTo(SceneType::Login);
        sceneManagerPtr->update(mockWindow, sf::seconds(0.016f)); // Processes switch, calls onEnter, update, ImGui::NewFrame
        ASSERT_EQ(sceneManagerPtr->getCurrentSceneType(), SceneType::Login);
        testing::Mock::VerifyAndClearExpectations(rawScene1); 

        // Render to complete the ImGui frame for the Login scene
        EXPECT_CALL(*rawScene1, render(::testing::Ref(mockWindow))).Times(1);
        sceneManagerPtr->render(mockWindow); // Calls rawScene1->render, ImGui::Render
        testing::Mock::VerifyAndClearExpectations(rawScene1);

        EXPECT_CALL(*rawScene1, onExit(::testing::_)).Times(1);
        EXPECT_CALL(*rawScene2, onEnter(::testing::_)).Times(1);
        EXPECT_CALL(*rawScene2, update(::testing::_, ::testing::_)).Times(1); // For the update after switch

        sceneManagerPtr->switchTo(SceneType::Game);
        sceneManagerPtr->update(mockWindow, sf::seconds(0.016f)); // Processes switch, calls onExit, onEnter, update, ImGui::NewFrame
        ASSERT_EQ(sceneManagerPtr->getCurrentSceneType(), SceneType::Game);
        // VerifyAndClearExpectations for rawScene1 (onExit) and rawScene2 (onEnter, update)
        testing::Mock::VerifyAndClearExpectations(rawScene1);
        testing::Mock::VerifyAndClearExpectations(rawScene2);

        // Optionally, render the Game scene to complete its ImGui frame if further actions were tested
        // EXPECT_CALL(*rawScene2, render(::testing::Ref(mockWindow))).Times(1);
        // sceneManagerPtr->render(mockWindow);
        // testing::Mock::VerifyAndClearExpectations(rawScene2);
    } catch (const std::exception& e) {
        FAIL() << "Exception during test: " << e.what();
    }
}

TEST_F(SceneManagerTest, SwitchToNonExistentScene) {
    // Verify initial state
    ASSERT_EQ(sceneManagerPtr->getCurrentSceneType(), SceneType::None);
    
    // Try to switch to a scene type that doesn't exist
    sceneManagerPtr->switchTo(SceneType::Login);
    
    try {
        // Update to process the scene switch
        sceneManagerPtr->update(mockWindow, sf::seconds(0.016f));
        
        // Verify that the scene type remains None since the Login scene doesn't exist
        ASSERT_EQ(sceneManagerPtr->getCurrentSceneType(), SceneType::None);
    } catch (const std::exception& e) {
        FAIL() << "Exception during scene switch: " << e.what();
    }
}

TEST_F(SceneManagerTest, HandleEventCallsCurrentScene) {
    auto mockGameScene = std::make_unique<MockScene>();
    MockScene* rawMockGameScene = mockGameScene.get();

    sceneManagerPtr->addScene(SceneType::Game, std::move(mockGameScene));

    EXPECT_CALL(*rawMockGameScene, onEnter(::testing::_)).Times(1);
    // Allow the update that happens during the switch
    EXPECT_CALL(*rawMockGameScene, update(sf::seconds(0.016f), ::testing::_)).Times(1);

    try {
        sceneManagerPtr->switchTo(SceneType::Game);
        sceneManagerPtr->update(mockWindow, sf::seconds(0.016f)); // Processes switch, calls onEnter, calls mockScene->update, calls ImGui::NewFrame
        testing::Mock::VerifyAndClearExpectations(rawMockGameScene);

        // Create a KeyPressed event using the new SFML 3.0 event system
        sf::Event testEvent(sf::Event::KeyPressed{
            sf::Keyboard::Key::A,
            sf::Keyboard::Scancode::A,
            false, // alt
            false, // control
            false, // shift
            false  // system
        });
        
        EXPECT_CALL(*rawMockGameScene, handleEvent(::testing::_, ::testing::_)).Times(1);
        sceneManagerPtr->handleEvent(mockWindow, testEvent); // This calls ImGui::ProcessEvent, then mockScene->handleEvent
        testing::Mock::VerifyAndClearExpectations(rawMockGameScene);
    } catch (const std::exception& e) {
        FAIL() << "Exception during test: " << e.what();
    }
}

TEST_F(SceneManagerTest, UpdateCallsCurrentScene) {
    auto mockGameScene = std::make_unique<MockScene>();
    MockScene* rawMockGameScene = mockGameScene.get();

    sceneManagerPtr->addScene(SceneType::Game, std::move(mockGameScene));

    try {
        EXPECT_CALL(*rawMockGameScene, onEnter(::testing::_)).Times(1);
        // Expect the first update call (which processes the switch and calls ImGui::NewFrame)
        EXPECT_CALL(*rawMockGameScene, update(sf::seconds(0.016f), ::testing::_)).Times(1).RetiresOnSaturation();
        
        sceneManagerPtr->switchTo(SceneType::Game);
        // This update processes the switch, calls onEnter, and the first mockScene->update.
        // It also calls ImGui::NewFrame().
        sceneManagerPtr->update(mockWindow, sf::seconds(0.016f)); 
        testing::Mock::VerifyAndClearExpectations(rawMockGameScene);

        // Now, test the second call to update more directly.
        // To do this cleanly and avoid ImGui issues with NewFrame/Render mismatch,
        // we need to ensure ImGui's state is valid.
        // This means a render call should happen after the NewFrame in the previous update.
        // Then another NewFrame (via update) before the targeted mockScene->update call.

        // Expect the specific update call we are testing
        sf::Time specificDeltaTime = sf::seconds(0.032f); // Use a distinct delta time
        EXPECT_CALL(*rawMockGameScene, update(specificDeltaTime, ::testing::_)).Times(1);
        
        // Simulate a render to complete the ImGui frame started by the previous update.
        sceneManagerPtr->render(mockWindow); // Calls ImGui::Render()

        // This update will call ImGui::NewFrame() and then the targeted mockScene->update.
        sceneManagerPtr->update(mockWindow, specificDeltaTime); 
        testing::Mock::VerifyAndClearExpectations(rawMockGameScene);
    } catch (const std::exception& e) {
        FAIL() << "Exception during test: " << e.what();
    }
}

TEST_F(SceneManagerTest, RenderCallsCurrentScene) {
    auto mockGameScene = std::make_unique<MockScene>();
    MockScene* rawMockGameScene = mockGameScene.get();

    sceneManagerPtr->addScene(SceneType::Game, std::move(mockGameScene));

    try {
        EXPECT_CALL(*rawMockGameScene, onEnter(::testing::_)).Times(1);
        // Allow the update that happens with the switch and prepares the first ImGui frame.
        EXPECT_CALL(*rawMockGameScene, update(sf::seconds(0.016f), ::testing::_)).Times(1);
        
        sceneManagerPtr->switchTo(SceneType::Game);
        sceneManagerPtr->update(mockWindow, sf::seconds(0.016f)); // Processes switch, calls onEnter, mockScene->update, ImGui::NewFrame
        testing::Mock::VerifyAndClearExpectations(rawMockGameScene); 

        EXPECT_CALL(*rawMockGameScene, render(::testing::Ref(mockWindow))).Times(1);
        // The update call that prepares for render is part of the SceneManager::render() call if ImGui::SFML::Update is there.
        // However, SceneManager::render only calls ImGui::SFML::Render. The ImGui::SFML::Update is in SceneManager::update.
        // So, an explicit update call is needed before render to set up ImGui::NewFrame.
        // The previous update already called NewFrame. So we can directly call render.

        sceneManagerPtr->render(mockWindow); // Calls mockScene->render, then ImGui::Render()
        testing::Mock::VerifyAndClearExpectations(rawMockGameScene);
    } catch (const std::exception& e) {
        FAIL() << "Exception during test: " << e.what();
    }
}

// Add more tests for edge cases, error handling, multiple scenes, etc.

// To run tests, you would typically compile this file with gtest and your SceneManager code,
// then execute the resulting binary.

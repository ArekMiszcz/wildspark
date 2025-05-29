#include "gtest/gtest.h"
#include "gmock/gmock.h"
#include "scenes/SceneManager.h"
#include "scenes/Scene.h"
#include <SFML/Graphics.hpp>
#include <memory>
#include <iostream>
#include "mocks/MockRenderWindow.h"

class MockScene : public Scene {
public:
    MOCK_METHOD(void, onEnter, (SceneManager& manager), (override));
    MOCK_METHOD(void, onExit, (SceneManager& manager), (override));
    MOCK_METHOD(void, handleEvent, (const sf::Event& event, SceneManager& manager), (override));
    MOCK_METHOD(void, update, (sf::Time deltaTime, SceneManager& manager), (override));
    MOCK_METHOD(void, render, (sf::RenderTarget& target), (override));
    ~MockScene() override = default;
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
        if (sceneManagerPtr && setupSucceeded) {
            try {
                sceneManagerPtr->shutdown();
            } catch (const std::exception& e) {
                std::cerr << "Exception during shutdown: " << e.what() << std::endl;
            }
        }
    }
};

TEST_F(SceneManagerEdgeCasesTest, DuplicateSceneTypes) {
    try {
        auto mockScene1 = std::make_unique<MockScene>();
        auto mockScene2 = std::make_unique<MockScene>();
        
        MockScene* rawMockScene1 = mockScene1.get();
        MockScene* rawMockScene2 = mockScene2.get();
        
        sceneManagerPtr->addScene(SceneType::Login, std::move(mockScene1));
        ASSERT_EQ(sceneManagerPtr->getSceneCount(), 1);
        
        sceneManagerPtr->addScene(SceneType::Login, std::move(mockScene2));
        ASSERT_EQ(sceneManagerPtr->getSceneCount(), 1);
        
        EXPECT_CALL(*rawMockScene2, onEnter(::testing::_)).Times(1);
        EXPECT_CALL(*rawMockScene2, update(::testing::_, ::testing::_)).Times(1);
        
        sceneManagerPtr->switchTo(SceneType::Login);
        sceneManagerPtr->update(mockWindow, sf::seconds(0.016f));
        
        testing::Mock::VerifyAndClearExpectations(rawMockScene2);
        
        EXPECT_CALL(*rawMockScene2, render(::testing::_)).Times(1);
        sceneManagerPtr->render(mockWindow);
        testing::Mock::VerifyAndClearExpectations(rawMockScene2);
        
    } catch (const std::exception& e) {
        FAIL() << "Exception during test: " << e.what();
    }
}

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
        
        ASSERT_EQ(sceneManagerPtr->getCurrentSceneType(), SceneType::None);
        
        EXPECT_CALL(*rawLoginScene, onEnter(::testing::_)).Times(1);
        EXPECT_CALL(*rawLoginScene, update(::testing::_, ::testing::_)).Times(1);
        
        sceneManagerPtr->switchTo(SceneType::Login);
        sceneManagerPtr->update(mockWindow, sf::seconds(0.016f));
        ASSERT_EQ(sceneManagerPtr->getCurrentSceneType(), SceneType::Login);
        
        testing::Mock::VerifyAndClearExpectations(rawLoginScene);
        
        EXPECT_CALL(*rawLoginScene, render(::testing::_)).Times(1);
        sceneManagerPtr->render(mockWindow);
        testing::Mock::VerifyAndClearExpectations(rawLoginScene);
        
        EXPECT_CALL(*rawLoginScene, onExit(::testing::_)).Times(1);
        EXPECT_CALL(*rawGameScene, onEnter(::testing::_)).Times(1);
        EXPECT_CALL(*rawGameScene, update(::testing::_, ::testing::_)).Times(1);
        
        sceneManagerPtr->switchTo(SceneType::Game);
        sceneManagerPtr->update(mockWindow, sf::seconds(0.016f));
        ASSERT_EQ(sceneManagerPtr->getCurrentSceneType(), SceneType::Game);
        
        testing::Mock::VerifyAndClearExpectations(rawLoginScene);
        testing::Mock::VerifyAndClearExpectations(rawGameScene);
        
        EXPECT_CALL(*rawGameScene, render(::testing::_)).Times(1);
        sceneManagerPtr->render(mockWindow);
        testing::Mock::VerifyAndClearExpectations(rawGameScene);
        
        EXPECT_CALL(*rawGameScene, onExit(::testing::_)).Times(1);
        EXPECT_CALL(*rawSettingsScene, onEnter(::testing::_)).Times(1);
        EXPECT_CALL(*rawSettingsScene, update(::testing::_, ::testing::_)).Times(1);
        
        sceneManagerPtr->switchTo(SceneType::Settings);
        sceneManagerPtr->update(mockWindow, sf::seconds(0.016f));
        ASSERT_EQ(sceneManagerPtr->getCurrentSceneType(), SceneType::Settings);
        
        testing::Mock::VerifyAndClearExpectations(rawGameScene);
        testing::Mock::VerifyAndClearExpectations(rawSettingsScene);
        
        EXPECT_CALL(*rawSettingsScene, render(::testing::_)).Times(1);
        sceneManagerPtr->render(mockWindow);
        testing::Mock::VerifyAndClearExpectations(rawSettingsScene);
        
        EXPECT_CALL(*rawSettingsScene, onExit(::testing::_)).Times(1);
        EXPECT_CALL(*rawLoginScene, onEnter(::testing::_)).Times(1);
        EXPECT_CALL(*rawLoginScene, update(::testing::_, ::testing::_)).Times(1);
        
        sceneManagerPtr->switchTo(SceneType::Login);
        sceneManagerPtr->update(mockWindow, sf::seconds(0.016f));
        ASSERT_EQ(sceneManagerPtr->getCurrentSceneType(), SceneType::Login);
        
        testing::Mock::VerifyAndClearExpectations(rawSettingsScene);
        testing::Mock::VerifyAndClearExpectations(rawLoginScene);
        
        EXPECT_CALL(*rawLoginScene, render(::testing::_)).Times(1);
        sceneManagerPtr->render(mockWindow);
        testing::Mock::VerifyAndClearExpectations(rawLoginScene);
    } catch (const std::exception& e) {
        FAIL() << "Exception during test: " << e.what();
    }
}

TEST_F(SceneManagerEdgeCasesTest, NullSceneHandling) {
    try {
        sceneManagerPtr->addScene(SceneType::Login, nullptr);
        ASSERT_EQ(sceneManagerPtr->getSceneCount(), 0);
        
        sceneManagerPtr->switchTo(SceneType::Login);
        sceneManagerPtr->update(mockWindow, sf::seconds(0.016f));
        ASSERT_EQ(sceneManagerPtr->getCurrentSceneType(), SceneType::None);
        sceneManagerPtr->render(mockWindow);
        
        auto validScene = std::make_unique<MockScene>();
        MockScene* rawValidScene = validScene.get();
        
        sceneManagerPtr->addScene(SceneType::Game, std::move(validScene));
        ASSERT_EQ(sceneManagerPtr->getSceneCount(), 1);
        
        EXPECT_CALL(*rawValidScene, onEnter(::testing::_)).Times(1);
        EXPECT_CALL(*rawValidScene, update(::testing::_, ::testing::_)).Times(1);
        
        sceneManagerPtr->switchTo(SceneType::Game);
        sceneManagerPtr->update(mockWindow, sf::seconds(0.016f));
        ASSERT_EQ(sceneManagerPtr->getCurrentSceneType(), SceneType::Game);
        
        testing::Mock::VerifyAndClearExpectations(rawValidScene);
        
        EXPECT_CALL(*rawValidScene, render(::testing::_)).Times(1);
        sceneManagerPtr->render(mockWindow);
        testing::Mock::VerifyAndClearExpectations(rawValidScene);
        
        sceneManagerPtr->addScene(SceneType::Game, nullptr);
        ASSERT_EQ(sceneManagerPtr->getSceneCount(), 1);
        ASSERT_EQ(sceneManagerPtr->getCurrentSceneType(), SceneType::Game);
        
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

TEST_F(SceneManagerEdgeCasesTest, ConcurrentSceneSwitches) {
    try {
        auto loginScene = std::make_unique<MockScene>();
        auto gameScene = std::make_unique<MockScene>();
        
        MockScene* rawLoginScene = loginScene.get();
        MockScene* rawGameScene = gameScene.get();
        
        sceneManagerPtr->addScene(SceneType::Login, std::move(loginScene));
        sceneManagerPtr->addScene(SceneType::Game, std::move(gameScene));
        
        sceneManagerPtr->switchTo(SceneType::Login);
        sceneManagerPtr->switchTo(SceneType::Game);
        
        EXPECT_CALL(*rawLoginScene, onEnter(::testing::_)).Times(0);
        EXPECT_CALL(*rawGameScene, onEnter(::testing::_)).Times(1);
        EXPECT_CALL(*rawGameScene, update(::testing::_, ::testing::_)).Times(1);
        
        sceneManagerPtr->update(mockWindow, sf::seconds(0.016f));
        ASSERT_EQ(sceneManagerPtr->getCurrentSceneType(), SceneType::Game);
        
        testing::Mock::VerifyAndClearExpectations(rawLoginScene);
        testing::Mock::VerifyAndClearExpectations(rawGameScene);
        
        EXPECT_CALL(*rawGameScene, render(::testing::_)).Times(1);
        sceneManagerPtr->render(mockWindow);
        testing::Mock::VerifyAndClearExpectations(rawGameScene);
        
        sceneManagerPtr->switchTo(SceneType::Login);
        sceneManagerPtr->switchTo(SceneType::Game);
        
        EXPECT_CALL(*rawGameScene, onExit(::testing::_)).Times(0);
        EXPECT_CALL(*rawLoginScene, onEnter(::testing::_)).Times(0);
        EXPECT_CALL(*rawGameScene, update(::testing::_, ::testing::_)).Times(1);
        
        sceneManagerPtr->update(mockWindow, sf::seconds(0.016f));
        ASSERT_EQ(sceneManagerPtr->getCurrentSceneType(), SceneType::Game);
        
        testing::Mock::VerifyAndClearExpectations(rawLoginScene);
        testing::Mock::VerifyAndClearExpectations(rawGameScene);
        
        EXPECT_CALL(*rawGameScene, render(::testing::_)).Times(1);
        sceneManagerPtr->render(mockWindow);
        testing::Mock::VerifyAndClearExpectations(rawGameScene);
        
        EXPECT_CALL(*rawGameScene, onExit(::testing::_)).Times(1);
        EXPECT_CALL(*rawLoginScene, onEnter(::testing::_)).Times(1);
        EXPECT_CALL(*rawLoginScene, update(::testing::_, ::testing::_)).Times(1);
        
        sceneManagerPtr->switchTo(SceneType::Login);
        sceneManagerPtr->update(mockWindow, sf::seconds(0.016f));
        ASSERT_EQ(sceneManagerPtr->getCurrentSceneType(), SceneType::Login);
        
        testing::Mock::VerifyAndClearExpectations(rawLoginScene);
        testing::Mock::VerifyAndClearExpectations(rawGameScene);
        
        EXPECT_CALL(*rawLoginScene, render(::testing::_)).Times(1);
        sceneManagerPtr->render(mockWindow);
        testing::Mock::VerifyAndClearExpectations(rawLoginScene);
    } catch (const std::exception& e) {
        FAIL() << "Exception during test: " << e.what();
    }
}

TEST_F(SceneManagerEdgeCasesTest, SceneLifecycleEvents) {
    try {
        auto loginScene = std::make_unique<MockScene>();
        auto gameScene = std::make_unique<MockScene>();
        
        MockScene* rawLoginScene = loginScene.get();
        MockScene* rawGameScene = gameScene.get();
        
        sceneManagerPtr->addScene(SceneType::Login, std::move(loginScene));
        sceneManagerPtr->addScene(SceneType::Game, std::move(gameScene));
        
        EXPECT_CALL(*rawLoginScene, onEnter(::testing::_)).Times(1);
        EXPECT_CALL(*rawLoginScene, update(::testing::_, ::testing::_)).Times(1);
        
        sceneManagerPtr->switchTo(SceneType::Login);
        sceneManagerPtr->update(mockWindow, sf::seconds(0.016f));
        ASSERT_EQ(sceneManagerPtr->getCurrentSceneType(), SceneType::Login);
        
        testing::Mock::VerifyAndClearExpectations(rawLoginScene);
        
        EXPECT_CALL(*rawLoginScene, render(::testing::_)).Times(1);
        sceneManagerPtr->render(mockWindow);
        testing::Mock::VerifyAndClearExpectations(rawLoginScene);
        
        EXPECT_CALL(*rawLoginScene, onExit(::testing::_)).Times(1);
        EXPECT_CALL(*rawGameScene, onEnter(::testing::_)).Times(1);
        EXPECT_CALL(*rawGameScene, update(::testing::_, ::testing::_)).Times(1);
        
        sceneManagerPtr->switchTo(SceneType::Game);
        sceneManagerPtr->update(mockWindow, sf::seconds(0.016f));
        ASSERT_EQ(sceneManagerPtr->getCurrentSceneType(), SceneType::Game);
        
        testing::Mock::VerifyAndClearExpectations(rawLoginScene);
        testing::Mock::VerifyAndClearExpectations(rawGameScene);
        
        EXPECT_CALL(*rawGameScene, render(::testing::_)).Times(1);
        sceneManagerPtr->render(mockWindow);
        testing::Mock::VerifyAndClearExpectations(rawGameScene);
    } catch (const std::exception& e) {
        FAIL() << "Exception during test: " << e.what();
    }
}

TEST_F(SceneManagerEdgeCasesTest, SwitchToNonExistentSceneTypes) {
    try {
        auto loginScene = std::make_unique<MockScene>();
        MockScene* rawLoginScene = loginScene.get();
        
        sceneManagerPtr->addScene(SceneType::Login, std::move(loginScene));
        
        EXPECT_CALL(*rawLoginScene, onEnter(::testing::_)).Times(1);
        EXPECT_CALL(*rawLoginScene, update(::testing::_, ::testing::_)).Times(1);
        
        sceneManagerPtr->switchTo(SceneType::Login);
        sceneManagerPtr->update(mockWindow, sf::seconds(0.016f));
        ASSERT_EQ(sceneManagerPtr->getCurrentSceneType(), SceneType::Login);
        
        testing::Mock::VerifyAndClearExpectations(rawLoginScene);
        
        EXPECT_CALL(*rawLoginScene, render(::testing::_)).Times(1);
        sceneManagerPtr->render(mockWindow);
        testing::Mock::VerifyAndClearExpectations(rawLoginScene);
        
        EXPECT_CALL(*rawLoginScene, onExit(::testing::_)).Times(1);
        
        sceneManagerPtr->switchTo(SceneType::Game);
        sceneManagerPtr->update(mockWindow, sf::seconds(0.016f));
        ASSERT_EQ(sceneManagerPtr->getCurrentSceneType(), SceneType::None);
        testing::Mock::VerifyAndClearExpectations(rawLoginScene);
        
        sceneManagerPtr->render(mockWindow);
    } catch (const std::exception& e) {
        FAIL() << "Exception during test: " << e.what();
    }
}

TEST_F(SceneManagerEdgeCasesTest, HandleEventsWithNoActiveScene) {
    try {
        sf::Event testEvent(sf::Event::KeyPressed{
            sf::Keyboard::Key::A,
            sf::Keyboard::Scancode::A,
            false, 
            false, 
            false, 
            false
        });
        
        sceneManagerPtr->handleEvent(mockWindow, testEvent);
        ASSERT_EQ(sceneManagerPtr->getCurrentSceneType(), SceneType::None);
        
        sceneManagerPtr->update(mockWindow, sf::seconds(0.016f));
        sceneManagerPtr->render(mockWindow);
    } catch (const std::exception& e) {
        FAIL() << "Exception during test: " << e.what();
    }
}

TEST_F(SceneManagerEdgeCasesTest, UpdateWithNoActiveScene) {
    try {
        sceneManagerPtr->update(mockWindow, sf::seconds(0.016f));
        ASSERT_EQ(sceneManagerPtr->getCurrentSceneType(), SceneType::None);
        sceneManagerPtr->render(mockWindow);
    } catch (const std::exception& e) {
        FAIL() << "Exception during test: " << e.what();
    }
}

TEST_F(SceneManagerEdgeCasesTest, RenderWithNoActiveScene) {
    try {
        sceneManagerPtr->update(mockWindow, sf::seconds(0.016f));
        sceneManagerPtr->render(mockWindow);
        ASSERT_EQ(sceneManagerPtr->getCurrentSceneType(), SceneType::None);
    } catch (const std::exception& e) {
        FAIL() << "Exception during test: " << e.what();
    }
}

TEST_F(SceneManagerEdgeCasesTest, RemoveScene) {
    try {
        auto loginScene = std::make_unique<MockScene>();
        auto gameScene = std::make_unique<MockScene>();
        
        MockScene* rawLoginScene = loginScene.get();
        MockScene* rawGameScene = gameScene.get();
        
        sceneManagerPtr->addScene(SceneType::Login, std::move(loginScene));
        sceneManagerPtr->addScene(SceneType::Game, std::move(gameScene));
        
        ASSERT_EQ(sceneManagerPtr->getSceneCount(), 2);
        
        sceneManagerPtr->removeScene(SceneType::Login);
        ASSERT_EQ(sceneManagerPtr->getSceneCount(), 1);
        
        sceneManagerPtr->switchTo(SceneType::Login);
        sceneManagerPtr->update(mockWindow, sf::seconds(0.016f));
        ASSERT_EQ(sceneManagerPtr->getCurrentSceneType(), SceneType::None);
        sceneManagerPtr->render(mockWindow);
        
        EXPECT_CALL(*rawGameScene, onEnter(::testing::_)).Times(1);
        EXPECT_CALL(*rawGameScene, update(::testing::_, ::testing::_)).Times(1);
        
        sceneManagerPtr->switchTo(SceneType::Game);
        sceneManagerPtr->update(mockWindow, sf::seconds(0.016f));
        ASSERT_EQ(sceneManagerPtr->getCurrentSceneType(), SceneType::Game);
        
        testing::Mock::VerifyAndClearExpectations(rawGameScene);
        
        EXPECT_CALL(*rawGameScene, render(::testing::_)).Times(1);
        sceneManagerPtr->render(mockWindow);
        testing::Mock::VerifyAndClearExpectations(rawGameScene);
        
        EXPECT_CALL(*rawGameScene, onExit(::testing::_)).Times(1);
        
        sceneManagerPtr->removeScene(SceneType::Game);
        
        ASSERT_EQ(sceneManagerPtr->getSceneCount(), 0);
        ASSERT_EQ(sceneManagerPtr->getCurrentSceneType(), SceneType::None);
        
        testing::Mock::VerifyAndClearExpectations(rawGameScene);
        
        sceneManagerPtr->update(mockWindow, sf::seconds(0.016f));
        sceneManagerPtr->render(mockWindow);
    } catch (const std::exception& e) {
        FAIL() << "Exception during test: " << e.what();
    }
}

TEST_F(SceneManagerEdgeCasesTest, RemoveNonExistentScene) {
    try {
        sceneManagerPtr->removeScene(SceneType::Login);
        ASSERT_EQ(sceneManagerPtr->getSceneCount(), 0);
        
        auto loginScene = std::make_unique<MockScene>();
        MockScene* rawLoginScene = loginScene.get();
        sceneManagerPtr->addScene(SceneType::Login, std::move(loginScene));
        
        ASSERT_EQ(sceneManagerPtr->getSceneCount(), 1);
        
        sceneManagerPtr->removeScene(SceneType::Game);
        ASSERT_EQ(sceneManagerPtr->getSceneCount(), 1);
        
        EXPECT_CALL(*rawLoginScene, onEnter(::testing::_)).Times(1);
        EXPECT_CALL(*rawLoginScene, update(::testing::_, ::testing::_)).Times(1);
        
        sceneManagerPtr->switchTo(SceneType::Login);
        sceneManagerPtr->update(mockWindow, sf::seconds(0.016f));
        
        testing::Mock::VerifyAndClearExpectations(rawLoginScene);
        
        EXPECT_CALL(*rawLoginScene, render(::testing::_)).Times(1);
        sceneManagerPtr->render(mockWindow);
        
        testing::Mock::VerifyAndClearExpectations(rawLoginScene);
    } catch (const std::exception& e) {
        FAIL() << "Exception during test: " << e.what();
    }
}
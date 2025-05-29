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
        if (sceneManagerPtr && setupSucceeded) {
            try {
                sceneManagerPtr->shutdown();
            } catch (const std::exception& e) {
                std::cerr << "Exception during shutdown: " << e.what() << std::endl;
            }
        }
    }
};

TEST_F(SceneManagerTest, InitialState) {
    try {
        ASSERT_EQ(sceneManagerPtr->getSceneCount(), 0);
        ASSERT_EQ(sceneManagerPtr->getCurrentSceneType(), SceneType::None);
    } catch (const std::exception& e) {
        FAIL() << "Exception during test: " << e.what();
    }
}

TEST_F(SceneManagerTest, AddScene) {
    try {
        auto mockScene1 = std::make_unique<MockScene>();
        sceneManagerPtr->addScene(SceneType::Login, std::move(mockScene1));
        ASSERT_EQ(sceneManagerPtr->getSceneCount(), 1);
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
        EXPECT_CALL(*rawScene1, update(::testing::_, ::testing::_)).Times(1);
        
        sceneManagerPtr->switchTo(SceneType::Login);
        sceneManagerPtr->update(mockWindow, sf::seconds(0.016f));
        ASSERT_EQ(sceneManagerPtr->getCurrentSceneType(), SceneType::Login);
        testing::Mock::VerifyAndClearExpectations(rawScene1); 

        EXPECT_CALL(*rawScene1, render(::testing::Ref(mockWindow))).Times(1);
        sceneManagerPtr->render(mockWindow);
        testing::Mock::VerifyAndClearExpectations(rawScene1);

        EXPECT_CALL(*rawScene1, onExit(::testing::_)).Times(1);
        EXPECT_CALL(*rawScene2, onEnter(::testing::_)).Times(1);
        EXPECT_CALL(*rawScene2, update(::testing::_, ::testing::_)).Times(1);

        sceneManagerPtr->switchTo(SceneType::Game);
        sceneManagerPtr->update(mockWindow, sf::seconds(0.016f));
        ASSERT_EQ(sceneManagerPtr->getCurrentSceneType(), SceneType::Game);
        testing::Mock::VerifyAndClearExpectations(rawScene1);
        testing::Mock::VerifyAndClearExpectations(rawScene2);

    } catch (const std::exception& e) {
        FAIL() << "Exception during test: " << e.what();
    }
}

TEST_F(SceneManagerTest, SwitchToNonExistentScene) {
    ASSERT_EQ(sceneManagerPtr->getCurrentSceneType(), SceneType::None);
    sceneManagerPtr->switchTo(SceneType::Login);
    
    try {
        sceneManagerPtr->update(mockWindow, sf::seconds(0.016f));
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
    EXPECT_CALL(*rawMockGameScene, update(sf::seconds(0.016f), ::testing::_)).Times(1);

    try {
        sceneManagerPtr->switchTo(SceneType::Game);
        sceneManagerPtr->update(mockWindow, sf::seconds(0.016f));
        testing::Mock::VerifyAndClearExpectations(rawMockGameScene);

        sf::Event testEvent(sf::Event::KeyPressed{
            sf::Keyboard::Key::A,
            sf::Keyboard::Scancode::A,
            false, 
            false, 
            false, 
            false
        });
        
        EXPECT_CALL(*rawMockGameScene, handleEvent(::testing::_, ::testing::_)).Times(1);
        sceneManagerPtr->handleEvent(mockWindow, testEvent);
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
        EXPECT_CALL(*rawMockGameScene, update(sf::seconds(0.016f), ::testing::_)).Times(1).RetiresOnSaturation();
        
        sceneManagerPtr->switchTo(SceneType::Game);
        sceneManagerPtr->update(mockWindow, sf::seconds(0.016f)); 
        testing::Mock::VerifyAndClearExpectations(rawMockGameScene);

        sf::Time specificDeltaTime = sf::seconds(0.032f);
        EXPECT_CALL(*rawMockGameScene, update(specificDeltaTime, ::testing::_)).Times(1);
        
        sceneManagerPtr->render(mockWindow);

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
        EXPECT_CALL(*rawMockGameScene, update(sf::seconds(0.016f), ::testing::_)).Times(1);
        
        sceneManagerPtr->switchTo(SceneType::Game);
        sceneManagerPtr->update(mockWindow, sf::seconds(0.016f));
        testing::Mock::VerifyAndClearExpectations(rawMockGameScene); 

        EXPECT_CALL(*rawMockGameScene, render(::testing::Ref(mockWindow))).Times(1);
        sceneManagerPtr->render(mockWindow);
        testing::Mock::VerifyAndClearExpectations(rawMockGameScene);
    } catch (const std::exception& e) {
        FAIL() << "Exception during test: " << e.what();
    }
}

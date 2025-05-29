#include "gtest/gtest.h"
#include "gmock/gmock.h"
#include "scenes/LoginScene/LoginScene.h"
#include <SFML/Graphics.hpp>
#include "imgui.h"
#include "imgui-SFML.h"
#include "mocks/MockRenderWindow.h"
#include "mocks/MockAuthManager.h"
#include "mocks/MockSceneManager.h"

class LoginSceneTest : public ::testing::Test {
protected:
    MockRenderWindow mockWindow;
    MockAuthManager mockAuthManager;
    std::unique_ptr<MockSceneManager> mockSceneManager;
    std::unique_ptr<LoginScene> loginScene;

    LoginSceneTest() {
    }

    void SetUp() override {
        bool success = ImGui::SFML::Init(mockWindow);
        ASSERT_TRUE(success) << "ImGui::SFML::Init failed. This is crucial for tests.";

        mockSceneManager = std::make_unique<MockSceneManager>(mockWindow);
        loginScene = std::make_unique<LoginScene>(mockWindow, mockAuthManager);
    }

    void TearDown() override {
        loginScene.reset();
        mockSceneManager.reset();
        ImGui::SFML::Shutdown(mockWindow);
    }
};

TEST_F(LoginSceneTest, OnEnterResetsStatusAndSetsSceneManager) {
    loginScene->onEnter(*mockSceneManager);
    EXPECT_CALL(*mockSceneManager, switchTo(testing::_)).Times(0);
}

TEST_F(LoginSceneTest, HandleLoginSuccess) {
    loginScene->onEnter(*mockSceneManager); 

    EXPECT_CALL(mockAuthManager, mockableAttemptLogin(testing::StrEq("test@example.com"), testing::StrEq("password"), testing::_))
        .WillOnce(testing::InvokeArgument<2>(true, "Login successful")); 

    EXPECT_CALL(*mockSceneManager, switchTo(SceneType::CharacterSelection)).Times(1);

    loginScene->handleLogin("test@example.com", "password");
}

TEST_F(LoginSceneTest, HandleLoginFailure) {
    loginScene->onEnter(*mockSceneManager);

    EXPECT_CALL(mockAuthManager, mockableAttemptLogin(testing::StrEq("test@example.com"), testing::StrEq("wrongpassword"), testing::_))
        .WillOnce(testing::InvokeArgument<2>(false, "Login failed")); 

    EXPECT_CALL(*mockSceneManager, switchTo(testing::_)).Times(0); 

    loginScene->handleLogin("test@example.com", "wrongpassword");
}

TEST_F(LoginSceneTest, RenderDoesNotCrash) {
    try {
        loginScene->onEnter(*mockSceneManager);
        ImGui::SFML::Update(mockWindow, sf::seconds(0.016f));
        
        loginScene->render(mockWindow);
        
        ImGui::EndFrame(); 
        ImGui::Render();
    } catch (const std::exception& e) {
        FAIL() << "Render method threw an exception: " << e.what();
    } catch (...) {
        FAIL() << "Render method threw an unknown exception.";
    }
}

TEST_F(LoginSceneTest, OnExitIsCallable) {
    try {
        loginScene->onEnter(*mockSceneManager);
        loginScene->onExit(*mockSceneManager);
    } catch (const std::exception& e) {
        FAIL() << "onExit method threw an exception: " << e.what();
    } catch (...) {
        FAIL() << "onExit method threw an unknown exception.";
    }
}

TEST_F(LoginSceneTest, HandleEventIsCallable) {
    sf::Event dummyEvent = sf::Event::Closed{};
    try {
        loginScene->onEnter(*mockSceneManager);
        loginScene->handleEvent(dummyEvent, *mockSceneManager);
    } catch (const std::exception& e) {
        FAIL() << "handleEvent method threw an exception: " << e.what();
    } catch (...) {
        FAIL() << "handleEvent method threw an unknown exception.";
    }
}

TEST_F(LoginSceneTest, UpdateIsCallable) {
    sf::Time dummyTime = sf::seconds(0.016f);
    try {
        loginScene->onEnter(*mockSceneManager);
        loginScene->update(dummyTime, *mockSceneManager);
    } catch (const std::exception& e) {
        FAIL() << "update method threw an exception: " << e.what();
    } catch (...) {
        FAIL() << "update method threw an unknown exception.";
    }
}

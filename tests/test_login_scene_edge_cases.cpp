#include "gtest/gtest.h"
#include "gmock/gmock.h"
#include "scenes/LoginScene/LoginScene.h"
#include <SFML/Graphics.hpp>
#include "imgui.h"
#include "imgui-SFML.h"
#include "mocks/MockRenderWindow.h"
#include "mocks/MockAuthManager.h"
#include "mocks/MockSceneManager.h"

class LoginSceneEdgeCasesTest : public ::testing::Test {
protected:
    MockRenderWindow mockWindow;
    MockAuthManager mockAuthManager;
    std::unique_ptr<MockSceneManager> mockSceneManager;
    std::unique_ptr<LoginScene> loginScene;

    LoginSceneEdgeCasesTest() {
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

TEST_F(LoginSceneEdgeCasesTest, HandleLoginWithEmptyCredentials) {
    loginScene->onEnter(*mockSceneManager);

    EXPECT_CALL(mockAuthManager, mockableAttemptLogin(testing::StrEq(""), testing::StrEq(""), testing::_))
        .WillOnce(testing::InvokeArgument<2>(false, "Login failed with empty credentials"));

    EXPECT_CALL(*mockSceneManager, switchTo(testing::_)).Times(0);

    loginScene->handleLogin("", "");
}

TEST_F(LoginSceneEdgeCasesTest, HandleLoginBeforeOnEnter) {
    EXPECT_CALL(mockAuthManager, mockableAttemptLogin(testing::StrEq("test@example.com"), testing::StrEq("password"), testing::_))
        .WillOnce(testing::InvokeArgument<2>(true, "Login successful"));
    EXPECT_CALL(*mockSceneManager, switchTo(testing::_)).Times(0);
    ASSERT_NO_THROW(loginScene->handleLogin("test@example.com", "password"));
}

TEST_F(LoginSceneEdgeCasesTest, OnEnterCalledMultipleTimes) {
    loginScene->onEnter(*mockSceneManager);
    loginScene->onEnter(*mockSceneManager);
    EXPECT_CALL(*mockSceneManager, switchTo(testing::_)).Times(0);
}

TEST_F(LoginSceneEdgeCasesTest, RenderWithEmptyStatusMessageWhenShown) {
    loginScene->onEnter(*mockSceneManager);
    try {
        ImGui::SFML::Update(mockWindow, sf::seconds(0.016f));
        loginScene->render(mockWindow);
        ImGui::EndFrame();
        ImGui::Render();
    } catch (const std::exception& e) {
        FAIL() << "Render method threw an exception with empty status: " << e.what();
    } catch (...) {
        FAIL() << "Render method threw an unknown exception with empty status.";
    }
}

TEST_F(LoginSceneEdgeCasesTest, RenderWithLongStatusMessage) {
    loginScene->onEnter(*mockSceneManager);
    std::string longMessage(500, 'L');
    EXPECT_CALL(mockAuthManager, mockableAttemptLogin(testing::_, testing::_, testing::_))
        .WillOnce(testing::InvokeArgument<2>(false, longMessage));
    
    loginScene->handleLogin("user", "pass");

    try {
        ImGui::SFML::Update(mockWindow, sf::seconds(0.016f));
        loginScene->render(mockWindow);
        ImGui::EndFrame();
        ImGui::Render();
    } catch (const std::exception& e) {
        FAIL() << "Render method threw an exception with long status: " << e.what();
    } catch (...) {
        FAIL() << "Render method threw an unknown exception with long status.";
    }
}

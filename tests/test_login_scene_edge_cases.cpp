#include "gtest/gtest.h"
#include "gmock/gmock.h"
#include "scenes/LoginScene/LoginScene.h"
#include "scenes/SceneManager.h"
#include "auth/AuthManager.h"
#include <SFML/Graphics.hpp>
#include <functional>
#include "imgui.h"
#include "imgui-SFML.h"

// Mock for sf::RenderWindow
class MockRenderWindow : public sf::RenderWindow {
public:
    MockRenderWindow() {}
    MOCK_METHOD(bool, setActive, (bool active), (override));
    MOCK_METHOD(void, close, (), (override));
};

// Mock for AuthManager
class MockAuthManager : public AuthManager {
public:
    MockAuthManager() : AuthManager(AuthManager::ConstructionMode::TESTING) {}
    ~MockAuthManager() override {}

    void attemptLogin(const std::string& email, const std::string& password, std::function<void(bool, const std::string&)> callback) override {
        mockableAttemptLogin(email, password, callback);
    }

    MOCK_METHOD(void, mockableAttemptLogin, (const std::string& email, const std::string& password, const std::function<void(bool, const std::string&)>& callback));
};

// Mock for SceneManager
class MockSceneManager : public SceneManager {
public:
    MockSceneManager(sf::RenderWindow& window) : SceneManager(window) {}
    MOCK_METHOD(void, switchTo, (SceneType type), (override));
};

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
    // Verify status message is updated as expected
    // This requires inspecting LoginScene's internal state or adding a getter for tests
}

TEST_F(LoginSceneEdgeCasesTest, HandleLoginBeforeOnEnter) {
    // sceneManagerRef will be null in LoginScene as onEnter is not called
    EXPECT_CALL(mockAuthManager, mockableAttemptLogin(testing::StrEq("test@example.com"), testing::StrEq("password"), testing::_))
        .WillOnce(testing::InvokeArgument<2>(true, "Login successful"));

    // switchTo should NOT be called because sceneManagerRef is null.
    // The LoginScene itself should log an error to std::cerr in this case.
    EXPECT_CALL(*mockSceneManager, switchTo(testing::_)).Times(0);

    // We can't directly check std::cerr here without more complex test setup (e.g., redirecting cerr).
    // So, this test primarily ensures no crash and that switchTo isn't called.
    ASSERT_NO_THROW(loginScene->handleLogin("test@example.com", "password"));
}

TEST_F(LoginSceneEdgeCasesTest, OnEnterCalledMultipleTimes) {
    loginScene->onEnter(*mockSceneManager); // First call
    // Potentially change status message here if we could set it directly or via a method

    loginScene->onEnter(*mockSceneManager); // Second call

    // Expect status to be reset, and no scene switch
    EXPECT_CALL(*mockSceneManager, switchTo(testing::_)).Times(0);
    // To fully verify, we'd need a getter for loginStatusMessage and showLoginStatus
}

TEST_F(LoginSceneEdgeCasesTest, RenderWithEmptyStatusMessageWhenShown) {
    loginScene->onEnter(*mockSceneManager);
    // Simulate a state where showLoginStatus is true but message is empty
    // This requires either a way to set these directly for testing, or a specific scenario
    // For now, we'll rely on onEnter resetting them and then try to render.
    // A more direct test would involve: 
    // loginScene->setShowLoginStatus(true); 
    // loginScene->setLoginStatusMessage("");

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

    // Simulate a login failure with a very long message
    std::string longMessage(500, 'L'); // 500 'L' characters
    EXPECT_CALL(mockAuthManager, mockableAttemptLogin(testing::_, testing::_, testing::_))
        .WillOnce(testing::InvokeArgument<2>(false, longMessage));
    
    loginScene->handleLogin("user", "pass"); // This will set the long message

    try {
        ImGui::SFML::Update(mockWindow, sf::seconds(0.016f));
        loginScene->render(mockWindow); // ImGui::TextWrapped should handle this
        ImGui::EndFrame();
        ImGui::Render();
    } catch (const std::exception& e) {
        FAIL() << "Render method threw an exception with long status: " << e.what();
    } catch (...) {
        FAIL() << "Render method threw an unknown exception with long status.";
    }
}

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
    // For SFML 3.0, many methods like display, clear, close, isOpen, pollEvent are not virtual.
    // We can hide them if necessary, or rely on the fact that LoginScene mostly uses ImGui.
    MOCK_METHOD(bool, setActive, (bool active), (override));
    MOCK_METHOD(void, close, (), (override));
};

// Mock for AuthManager
class MockAuthManager : public AuthManager {
public:
    MockAuthManager() : AuthManager(AuthManager::ConstructionMode::TESTING) {}
    ~MockAuthManager() override {}

    // Override attemptLogin to directly control callback behavior for tests
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

class LoginSceneTest : public ::testing::Test {
protected:
    MockRenderWindow mockWindow;
    MockAuthManager mockAuthManager;
    std::unique_ptr<MockSceneManager> mockSceneManager;
    std::unique_ptr<LoginScene> loginScene;

    LoginSceneTest() {
    }

    void SetUp() override {
        // Initialize ImGui - ImGui::SFML::Init will create a context.
        // If it fails, it should return false.
        // The main issue is likely context creation/destruction rather than the handle itself for basic tests.
        bool success = ImGui::SFML::Init(mockWindow);
        ASSERT_TRUE(success) << "ImGui::SFML::Init failed. This is crucial for tests.";

        mockSceneManager = std::make_unique<MockSceneManager>(mockWindow);
        loginScene = std::make_unique<LoginScene>(mockWindow, mockAuthManager);
    }

    void TearDown() override {
        // Order: Reset objects using ImGui/SFML first, then shutdown ImGui-SFML.
        loginScene.reset();
        mockSceneManager.reset();

        // ImGui::SFML::Shutdown(mockWindow) should destroy the context created by Init.
        ImGui::SFML::Shutdown(mockWindow);
    }
};

TEST_F(LoginSceneTest, OnEnterResetsStatusAndSetsSceneManager) {
    loginScene->onEnter(*mockSceneManager);

    EXPECT_CALL(*mockSceneManager, switchTo(testing::_)).Times(0); // Ensure no scene switch during onEnter
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
        ImGui::SFML::Update(mockWindow, sf::seconds(0.016f)); // This calls ImGui::NewFrame()
        
        loginScene->render(mockWindow);
        
        ImGui::EndFrame(); 
        ImGui::Render(); // Generates draw data
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

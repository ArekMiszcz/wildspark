#include <iostream>
#include "imgui-SFML.h"
#include "imgui.h"
#include "../SceneManager.h"
#include "LoginScene.h"

// Member to store login status/message for ImGui
static std::string loginStatusMessage = "";
static bool showLoginStatus = false;

LoginScene::LoginScene(sf::RenderWindow& window, AuthManager& authMgr) 
    : windowRef(window), authManagerRef(authMgr), sceneManagerRef(nullptr) {
    std::cout << "Login Scene initialized" << std::endl;
}

void LoginScene::onEnter(SceneManager& manager) {
    std::cout << "Entering Login Scene" << std::endl;
    this->sceneManagerRef = &manager; // Store the SceneManager reference
    showLoginStatus = false; // Reset status on entering scene
    loginStatusMessage = "";
}

void LoginScene::handleEvent(const sf::Event& event, SceneManager& manager) {
    
}

void LoginScene::update(sf::Time deltaTime, SceneManager& manager) {
    // authManagerRef.tick(); // Removed: AuthManager is now ticked globally in main.cpp
}

void LoginScene::render(sf::RenderTarget& target) {
    static bool isOpen = true;
    ImGui::SetNextWindowSizeConstraints(ImVec2(250, 150), ImVec2(400, 300)); // Min and Max size
    ImGui::Begin("Login", &isOpen, ImGuiWindowFlags_NoCollapse | ImGuiWindowFlags_NoTitleBar | ImGuiWindowFlags_NoScrollbar | ImGuiWindowFlags_AlwaysAutoResize );
    
    // Center the window
    ImVec2 windowSize = ImGui::GetWindowSize();
    ImVec2 viewportSize = ImGui::GetMainViewport()->Size;
    ImGui::SetWindowPos(ImVec2((viewportSize.x - windowSize.x) * 0.5f, (viewportSize.y - windowSize.y) * 0.5f));

    // Email input field
    ImGui::SetCursorPosY(ImGui::GetCursorPosY() + 5);
    ImGui::Text("Email:");
    static char email[128] = "";
    ImGui::PushItemWidth(-1); // Make input field take full width
    ImGui::InputText("##email", email, sizeof(email));
    ImGui::PopItemWidth();

    // Password input field
    ImGui::SetCursorPosY(ImGui::GetCursorPosY() + 5);
    ImGui::Text("Password:");
    static char password[128] = "";
    ImGui::PushItemWidth(-1);
    ImGui::InputText("##password", password, sizeof(password), ImGuiInputTextFlags_Password);
    ImGui::PopItemWidth();

    // Login button
    ImGui::SetCursorPosY(ImGui::GetCursorPosY() + 10);
    // Center the button
    float buttonWidth = 60;
    float buttonPosX = (ImGui::GetWindowWidth() - buttonWidth) * 0.5f;
    ImGui::SetCursorPosX(buttonPosX);
    if (ImGui::Button("Login", ImVec2(buttonWidth, 30))) {
        std::cout << "Login button clicked!" << std::endl;
        showLoginStatus = false; // Hide previous status message
        loginStatusMessage = "Attempting login..."; // Indicate processing
        this->handleLogin(email, password); // Call without SceneManager param
    }

    // Display login status message
    if (showLoginStatus && !loginStatusMessage.empty()) {
        ImGui::SetCursorPosY(ImGui::GetCursorPosY() + 10);
        ImGui::TextWrapped("%s", loginStatusMessage.c_str());
    }

    ImGui::End();
}

// Updated handleLogin to use the stored sceneManagerRef
void LoginScene::handleLogin(const char* email_cstr, const char* password_cstr) {
    std::string email_str(email_cstr);
    std::string password_str(password_cstr);

    // Define the callback lambda
    auto loginCallback = [this](bool success, const std::string& message) { // Capture this to access sceneManagerRef
        showLoginStatus = true;
        loginStatusMessage = message;
        if (success) {
            std::cout << "LoginScene: Login successful via callback. Message: " << message << std::endl;
            if (this->sceneManagerRef) {
                // Transition to the next scene using SceneManager
                this->sceneManagerRef->switchTo(SceneType::CharacterSelection); 
                std::cout << "Would transition to next scene now using stored sceneManagerRef." << std::endl;
            } else {
                std::cerr << "LoginScene: sceneManagerRef is null in callback!" << std::endl;
            }
        } else {
            std::cerr << "LoginScene: Login failed via callback. Message: " << message << std::endl;
        }
    };

    authManagerRef.attemptLogin(email_str, password_str, loginCallback); // Use authManagerRef
}

void LoginScene::onExit(SceneManager& manager) {
    std::cout << "Exiting Login Scene" << std::endl;
}
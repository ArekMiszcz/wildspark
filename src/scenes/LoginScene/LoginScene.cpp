// Copyright 2025 WildSpark Authors

#include <iostream>
#include <string>
#include "imgui-SFML.h"
#include "imgui.h"
#include "../SceneManager.h"
#include "LoginScene.h"

LoginScene::LoginScene(sf::RenderWindow& window, AuthManager& authMgr)
    : windowRef(window),
      authManagerRef(authMgr),
      sceneManagerRef(nullptr),
      loginStatusMessage(""),
      showLoginStatus(false) {
}

void LoginScene::onEnter(SceneManager& manager) {
    this->sceneManagerRef = &manager;
    this->showLoginStatus = false;
    this->loginStatusMessage = "";
}

void LoginScene::handleEvent(const sf::Event& event, SceneManager& manager) {}

void LoginScene::update(sf::Time deltaTime, SceneManager& manager) {}

void LoginScene::render(sf::RenderTarget& target) {
    static bool isOpen = true;
    ImGui::SetNextWindowSizeConstraints(ImVec2(250, 150), ImVec2(400, 300));
    ImGui::Begin("Login", &isOpen,
                 ImGuiWindowFlags_NoCollapse | ImGuiWindowFlags_NoTitleBar |
                 ImGuiWindowFlags_NoScrollbar | ImGuiWindowFlags_AlwaysAutoResize);

    // Center the window
    ImVec2 windowSize = ImGui::GetWindowSize();
    ImVec2 viewportSize = ImGui::GetMainViewport()->Size;
    ImGui::SetWindowPos(ImVec2((viewportSize.x - windowSize.x) * 0.5f, (viewportSize.y - windowSize.y) * 0.5f));

    // Email input field
    ImGui::SetCursorPosY(ImGui::GetCursorPosY() + 5);
    ImGui::Text("Email:");
    static char email[128] = "";
    ImGui::PushItemWidth(-1);
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
        this->showLoginStatus = false;
        this->loginStatusMessage = "Attempting login...";
        this->handleLogin(email, password);
    }

    // Display login status message
    if (this->showLoginStatus && !this->loginStatusMessage.empty()) {
        ImGui::SetCursorPosY(ImGui::GetCursorPosY() + 10);
        ImGui::TextWrapped("%s", this->loginStatusMessage.c_str());
    }

    ImGui::End();
}

void LoginScene::handleLogin(const char* email_cstr, const char* password_cstr) {
    std::string email_str(email_cstr);
    std::string password_str(password_cstr);

    // Define the callback lambda
    auto loginCallback = [this](bool success, const std::string& message) {
        this->showLoginStatus = true;
        this->loginStatusMessage = message;
        if (success) {
            if (this->sceneManagerRef) {
                this->sceneManagerRef->switchTo(SceneType::CharacterSelection);
            } else {
                std::cerr << "LoginScene: sceneManagerRef is null in callback!" << std::endl;
            }
        } else {
            std::cerr << "LoginScene: Login failed via callback. Message: " << message << std::endl;
        }
    };

    authManagerRef.attemptLogin(email_str, password_str, loginCallback);
}

void LoginScene::onExit(SceneManager& manager) {}

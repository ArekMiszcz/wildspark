#include "CharacterCreationScene.h"
#include <iostream>
// Include ImGui if UI is added for character creation fields
// #include "imgui.h"
// #include "imgui-SFML.h"
#include "../SceneManager.h" // For scene transitions

CharacterCreationScene::CharacterCreationScene(sf::RenderWindow& window, AuthManager& authMgr)
    : windowRef(window), authManagerRef(authMgr), sceneManagerRef(nullptr) { // Updated constructor
    std::cout << "CharacterCreationScene initialized" << std::endl;
}

void CharacterCreationScene::onEnter(SceneManager& manager) {
    std::cout << "Entering CharacterCreationScene" << std::endl;
    this->sceneManagerRef = &manager;
    // Initialize AccountManager here if needed, similar to CharacterSelectionScene
    // NakamaClient* nakamaClientPtr = dynamic_cast<NakamaClient*>(authManagerRef.authClient);
    // if (nakamaClientPtr && nakamaClientPtr->client && nakamaClientPtr->session) {
    //     accountManager = std::make_unique<AccountManager>(nakamaClientPtr->client, nakamaClientPtr->session);
    // }
}

void CharacterCreationScene::handleEvent(const sf::Event& event, SceneManager& manager) {
    std::cout << "Handling event in CharacterCreationScene" << std::endl;
    // if (ImGui::Button("Save Character")) { ... }
}

void CharacterCreationScene::update(sf::Time deltaTime, SceneManager& manager) {
    // authManagerRef.tick(); // Not needed here, ticked globally
    std::cout << "Updating CharacterCreationScene" << std::endl;
}

void CharacterCreationScene::render(sf::RenderTarget& target) {
    std::cout << "Rendering CharacterCreationScene" << std::endl;
    // ImGui::Begin("Character Creation");
    // ImGui::InputText("Name", ...);
    // ImGui::End();

    // Temporary: Button to go back to selection or to game (if char created)
    // This logic will be more fleshed out when creation is implemented.
    // if (ImGui::Button("Back to Character Selection")) {
    //     if (sceneManagerRef) {
    //         sceneManagerRef->switchTo(SceneType::CharacterSelection);
    //     }
    // }
}

void CharacterCreationScene::onExit(SceneManager& manager) {
    std::cout << "Exiting CharacterCreationScene" << std::endl;
}
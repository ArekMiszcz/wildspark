#include "CharacterSelectionScene.h"
#include <iostream>

CharacterSelectionScene::CharacterSelectionScene(sf::RenderWindow& window) : windowRef(window) {
    std::cout << "CharacterSelectionScene initialized" << std::endl;
}

void CharacterSelectionScene::onEnter(SceneManager& manager) {
    std::cout << "Entering CharacterSelectionScene" << std::endl;
    this->sceneManagerRef = &manager; // Store the SceneManager reference
}

void CharacterSelectionScene::handleEvent(const sf::Event& event, SceneManager& manager) {
    std::cout << "Handling event in CharacterSelectionScene" << std::endl;
}

void CharacterSelectionScene::update(sf::Time deltaTime, SceneManager& manager) {
    authManager.tick(); // Call tick for AuthManager (and underlying NakamaClient)
}

void CharacterSelectionScene::render(sf::RenderTarget& target) {
    std::cout << "Rendering CharacterSelectionScene" << std::endl;
}

void CharacterSelectionScene::onExit(SceneManager& manager) {
    std::cout << "Exiting CharacterSelectionScene" << std::endl;
}
#include "CharacterCreationScene.h"
#include <iostream>

CharacterCreationScene::CharacterCreationScene(sf::RenderWindow& window) : windowRef(window) {
    std::cout << "CharacterCreationScene initialized" << std::endl;
}

void CharacterCreationScene::onEnter(SceneManager& manager) {
    std::cout << "Entering CharacterCreationScene" << std::endl;
}

void CharacterCreationScene::handleEvent(const sf::Event& event, SceneManager& manager) {
    std::cout << "Handling event in CharacterCreationScene" << std::endl;
}

void CharacterCreationScene::update(sf::Time deltaTime, SceneManager& manager) {
    std::cout << "Updating CharacterCreationScene" << std::endl;
}

void CharacterCreationScene::render(sf::RenderTarget& target) {
    std::cout << "Rendering CharacterCreationScene" << std::endl;
}

void CharacterCreationScene::onExit(SceneManager& manager) {
    std::cout << "Exiting CharacterCreationScene" << std::endl;
}
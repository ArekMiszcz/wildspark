#pragma once
#include "../Scene.h"
#include <SFML/Graphics.hpp>
#include "../../auth/AuthManager.h"

// Forward declare SceneManager to avoid circular dependency if Scene.h includes SceneManager.h
class SceneManager;

class CharacterCreationScene : public Scene {
public:
    CharacterCreationScene(sf::RenderWindow& window, AuthManager& authManager);

    void onEnter(SceneManager& manager) override;
    void handleEvent(const sf::Event& event, SceneManager& manager) override;
    void update(sf::Time deltaTime, SceneManager& manager) override;
    void render(sf::RenderTarget& target) override;
    void onExit(SceneManager& manager) override;

private:
    sf::RenderWindow& windowRef;
    AuthManager& authManagerRef;
    SceneManager* sceneManagerRef = nullptr; // Store a reference to the SceneManager
};
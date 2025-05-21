#pragma once
#include "../Scene.h"
#include <SFML/Graphics.hpp>

class LoginScene : public Scene {
public:
    LoginScene(sf::RenderWindow& window);

    void onEnter(SceneManager& manager) override;
    void handleEvent(const sf::Event& event, SceneManager& manager) override;
    void update(sf::Time deltaTime, SceneManager& manager) override;
    void render(sf::RenderTarget& target) override;

private:
    sf::RenderWindow& windowRef;

    void setupUI();
};
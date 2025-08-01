#include <SFML/Window.hpp>
#include <SFML/Graphics.hpp>
#include "scenes/SceneManager.h"
#include "scenes/LoginScene/LoginScene.h"
#include "scenes/CharacterScene/CharacterSelectionScene.h"
#include "scenes/CharacterScene/CharacterCreationScene.h"
#include "scenes/GameScene/GameScene.h"
#include "auth/AuthManager.h"
#include "account/AccountManager.h" // Added for AccountManager
#include "input/InputManager.h" // Include InputManager

int main() {
    sf::RenderWindow window(sf::VideoMode({ 800, 600 }, 24), "SFML Game with Scenes");
    window.setFramerateLimit(60);

    AuthManager authManager;
    InputManager inputManager; // Create an InputManager instance

    AccountManager accountManager(authManager);

    SceneManager sceneManager = SceneManager(window);

    // Adding scenes to the scene manager
    sceneManager.addScene(SceneType::Login, std::make_unique<LoginScene>(window, authManager));
    sceneManager.addScene(SceneType::CharacterSelection, std::make_unique<CharacterSelectionScene>(window, authManager, accountManager));
    sceneManager.addScene(SceneType::CharacterCreation, std::make_unique<CharacterCreationScene>(window, authManager, accountManager)); 
    sceneManager.addScene(SceneType::Game, std::make_unique<GameScene>(window, authManager, inputManager)); 

    // Switch to the login scene
    sceneManager.switchTo(SceneType::Login);

    sf::Clock clock;

    while (window.isOpen()) {
        sf::Time deltaTime = clock.restart();

        while (const std::optional oEvent = window.pollEvent()) {
            if (oEvent.has_value()) {
                const sf::Event event = oEvent.value();

                if (event.is<sf::Event::Closed>()) {
                    window.close();
                }

                sceneManager.handleEvent(window, event); 

                // Handling window resize
                if (const auto* resized = event.getIf<sf::Event::Resized>()) {
                    sf::Vector2<float> size = {static_cast<float>(resized->size.x), static_cast<float>(resized->size.y)};
                    sf::FloatRect visibleArea({0, 0}, size);
                    window.setView(sf::View(visibleArea));
                }
            }
        }

        sceneManager.update(window, deltaTime);
        authManager.tick();

        // Render the scene manager
        window.clear(sf::Color(30, 30, 30));
        sceneManager.render(window);
        
        // Update InputManager after all game logic and rendering for the current frame
        inputManager.update();

        window.display();
    }

    sceneManager.shutdown();

    return 0;
}
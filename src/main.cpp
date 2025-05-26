#include <SFML/Window.hpp>
#include <SFML/Graphics.hpp>
#include "scenes/SceneManager.h"
#include "scenes/LoginScene/LoginScene.h"
#include "scenes/CharacterScene/CharacterSelectionScene.h"
#include "scenes/CharacterScene/CharacterCreationScene.h"

int main() {
    sf::RenderWindow window(sf::VideoMode({ 800, 600 }, 24), "SFML Game with Scenes");
    window.setFramerateLimit(60);

    SceneManager sceneManager = SceneManager(window);

    // Adding scenes to the scene manager
    sceneManager.addScene(SceneType::Login, std::make_unique<LoginScene>(window));
    sceneManager.addScene(SceneType::CharacterSelection, std::make_unique<CharacterSelectionScene>(window));
    sceneManager.addScene(SceneType::CharacterCreation, std::make_unique<CharacterCreationScene>(window));

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

                // Pasing event to the scene manager
                sceneManager.handleEvent(window, event);

                // Handling window resize
                if (const auto* resized = event.getIf<sf::Event::Resized>()) {
                    sf::Vector2<float> size = {static_cast<float>(resized->size.x), static_cast<float>(resized->size.y)};
                    sf::FloatRect visibleArea({0, 0}, size);
                    window.setView(sf::View(visibleArea));
                }
            }
        }

        // Update the scene manager
        sceneManager.update(window, deltaTime);

        // Render the scene manager
        window.clear(sf::Color(30, 30, 30)); // Ciemne tło
        sceneManager.render(window);
        window.display();
    }

    sceneManager.shutdown();

    return 0;
}
#include <SFML/Window.hpp>
#include <SFML/Graphics.hpp>
#include "scenes/SceneManager.h"
#include "scenes/LoginScene/LoginScene.h"

int main() {
    sf::RenderWindow window(sf::VideoMode({ 800, 600 }, 24), "SFML Game with Scenes");
    window.setFramerateLimit(60);

    SceneManager sceneManager = SceneManager(window);

    // Tworzenie i dodawanie scen
    sceneManager.addScene(SceneType::Login, std::make_unique<LoginScene>(window));

    // Ustawienie sceny początkowej
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

                // Przekaż zdarzenia do menedżera scen, który przekaże je aktywnej scenie
                sceneManager.handleEvent(window, event);

                // Jeśli okno zmieniło rozmiar, możemy poinformować menedżera lub bezpośrednio scenę
                if (const auto* resized = event.getIf<sf::Event::Resized>()) {
                    sf::Vector2<float> size = {static_cast<float>(resized->size.x), static_cast<float>(resized->size.y)};
                    sf::FloatRect visibleArea({0, 0}, size);
                    window.setView(sf::View(visibleArea));
                }
            }
        }

        // Aktualizacja logiki
        sceneManager.update(window, deltaTime);

        // Renderowanie
        window.clear(sf::Color(30, 30, 30)); // Ciemne tło
        sceneManager.render(window);
        window.display();
    }

    sceneManager.shutdown();

    return 0;
}
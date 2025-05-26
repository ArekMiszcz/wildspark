#pragma once // Use pragma once for header guard
#include <SFML/Graphics.hpp>
#include <nakama-cpp/Nakama.h> 
#include <nakama-cpp/NTypes.h>
#include "../Scene.h"
#include "../../auth/AuthManager.h"
#include "../../world/WorldMap.h"
#include "../../world/WorldRenderer.h" // Corrected include path and ensure it's used
#include "../../graphics/Camera.h" // Include Camera.h
#include "../../input/InputManager.h" // Ensure this path is correct and file exists

// Forward declare SceneManager to avoid circular dependency
class SceneManager;
class InputManager; // Forward declaration for the reference member

class GameScene : public Scene {
public:
    GameScene(sf::RenderWindow& window, AuthManager& authManager, InputManager& inputManager); // Use InputManager here
    ~GameScene() override;

    void onEnter(SceneManager& manager) override;
    void onExit(SceneManager& manager) override;
    void handleEvent(const sf::Event& event, SceneManager& manager) override;
    void update(sf::Time deltaTime, SceneManager& manager) override;
    void render(sf::RenderTarget& target) override;

private:
    sf::RenderWindow& windowRef;
    AuthManager& authManagerRef;
    Nakama::NRtClientPtr rtClient; 
    sf::Font font; 
    float m_zoomSensitivity; // Added for mouse wheel zoom sensitivity

    WorldMap m_worldMap;
    WorldRenderer m_worldRenderer;
    InputManager& m_inputManager; // Reference to InputManager
    Camera m_camera;
};

// Removed #endif // GAMESCENE_H as #pragma once is used

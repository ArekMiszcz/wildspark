#include "GameScene.h"
#include "../SceneManager.h" 
#include "../../world/WorldRenderer.h" 
#include "../../input/InputManager.h" // Ensure InputManager is included
#include <iostream> 
#include <SFML/Graphics/Font.hpp> // Ensure Font is included before Text
#include <SFML/Graphics/Text.hpp> 
#include <SFML/System/String.hpp> // For sf::String
#include "../../graphics/Camera.h" // Ensure Camera is included

GameScene::GameScene(sf::RenderWindow& window, AuthManager& authManager, InputManager& inputManager)
    : windowRef(window),
      authManagerRef(authManager),
      m_inputManager(inputManager), // Initialize InputManager reference
      rtClient(nullptr),
      m_worldMap(50, 50),
      m_worldRenderer(m_worldMap),
      m_camera(window, 300.0f),
      m_zoomSensitivity(0.1f) { // MODIFIED: Reduced zoom sensitivity
    // Attempt to load a common system font as a fallback first
    // You should ensure a font file (e.g., "arial.ttf") is distributed with your game
    // and placed in a known relative path to the executable.
    if (!font.openFromFile("/usr/share/fonts/truetype/dejavu/DejaVuSans.ttf")) { // Common on Linux
        if (!font.openFromFile("arial.ttf")) { // Fallback to arial.ttf in execution dir
             std::cerr << "Failed to load font for GameScene! Place arial.ttf or ensure system font path is correct." << std::endl;
            // Consider throwing an exception or setting an error flag
        }
    }

    // Configure default input mappings for GameScene related actions
    m_inputManager.mapActionToKey("camera_move_up", sf::Keyboard::Key::W);
    m_inputManager.mapActionToKey("camera_move_down", sf::Keyboard::Key::S);
    m_inputManager.mapActionToKey("camera_move_left", sf::Keyboard::Key::A);
    m_inputManager.mapActionToKey("camera_move_right", sf::Keyboard::Key::D);
    m_inputManager.mapActionToKey("escape_menu", sf::Keyboard::Key::Escape);

    // Map mouse wheel for zooming using the correct SFML 3 enum
    m_inputManager.mapActionToMouseWheel("camera_zoom_wheel", sf::Mouse::Wheel::Vertical); // Corrected enum
}

GameScene::~GameScene() {
    // Destructor
    if (rtClient && rtClient->isConnected()) {
        rtClient->disconnect();
    }
}

void GameScene::onEnter(SceneManager& manager) {
    std::cout << "Entering GameScene" << std::endl;
    rtClient = authManagerRef.getRtClient();

    if (rtClient) {
        std::cout << "rtClient obtained." << std::endl;
        // Setup rtClient listeners if necessary
        // rtClient->setListener(&myRtClientListener);
        if (!rtClient->isConnected()) {
            std::cout << "rtClient is not connected. Connection might be handled by AuthManager or need explicit call here." << std::endl;
        }
    } else {
        std::cerr << "Failed to get rtClient from AuthManager! GameScene may not function correctly." << std::endl;
        // manager.switchTo(SceneType::Login); // Example: Go back to login
    }
}

void GameScene::onExit(SceneManager& manager) {
    std::cout << "Exiting GameScene" << std::endl;
    if (rtClient && rtClient->isConnected()) {
        // Consider if disconnecting here is appropriate or if AuthManager handles it
        // rtClient->disconnect(); 
    }
    // rtClient->removeListener(); // Clean up listeners
}

void GameScene::handleEvent(const sf::Event& event, SceneManager& manager) {
    // Pass all events to the InputManager first
    m_inputManager.handleEvent(event);

    // Specific non-action based event handling (e.g., window close)
    if (event.is<sf::Event::Closed>()) {
        windowRef.close();
        return; // Early exit if window is closed
    }
    // Check for escape action from InputManager
    if (m_inputManager.isActionPressed("escape_menu")) {
        std::cout << "Escape action pressed via InputManager" << std::endl;
        // manager.switchTo(SceneType::CharacterSelection); // Or some other menu
    }
}

void GameScene::update(sf::Time deltaTime, SceneManager& manager) {
    std::cout << "GameScene::update() CALLED" << std::endl; // Unconditional log
    if (rtClient) {
        rtClient->tick();
    }

    // Update camera movement based on InputManager states
    m_camera.setMovingUp(m_inputManager.isActionActive("camera_move_up"));
    m_camera.setMovingDown(m_inputManager.isActionActive("camera_move_down"));
    m_camera.setMovingLeft(m_inputManager.isActionActive("camera_move_left"));
    m_camera.setMovingRight(m_inputManager.isActionActive("camera_move_right"));

    // Handle mouse wheel zoom
    float wheelDelta = m_inputManager.getMouseWheelDelta("camera_zoom_wheel");
    if (wheelDelta != 0) {
        std::cout << "GameScene: wheelDelta = " << wheelDelta << std::endl; // Log wheelDelta
        float zoomFactor = 1.0f - (wheelDelta * m_zoomSensitivity);
        std::cout << "GameScene: zoomFactor = " << zoomFactor << std::endl; // Log zoomFactor
        
        // Get current mouse position in window coordinates
        sf::Vector2i mousePixelPos = sf::Mouse::getPosition(windowRef);
        // Convert mouse position to world coordinates before zooming
        sf::Vector2f mouseWorldPosBeforeZoom = windowRef.mapPixelToCoords(mousePixelPos, m_camera.getView());

        m_camera.zoom(zoomFactor);

        // Convert the same mouse position to world coordinates after zooming
        sf::Vector2f mouseWorldPosAfterZoom = windowRef.mapPixelToCoords(mousePixelPos, m_camera.getView());

        // Calculate the difference and move the camera to keep the mouse position fixed
        sf::Vector2f offset = mouseWorldPosBeforeZoom - mouseWorldPosAfterZoom;
        m_camera.move(offset.x, offset.y);
    }

    m_camera.update(deltaTime);
    
    // InputManager::update() should be called once per frame, typically in the main game loop
    // after all event processing and before rendering, to clear the just pressed/released states.
    // This is crucial. For now, it's commented out, assuming it will be called in the main loop.
    // m_inputManager.update(); 
}

void GameScene::render(sf::RenderTarget& target) {
    // Render game elements
    if (font.getInfo().family.empty()) { // Check if font actually loaded
        // If font loading failed, we cannot reliably draw text.
        // Consider logging an error or drawing a simple shape as an indicator.
        // For now, we'll just print to cerr and return to avoid crashing.
        std::cerr << "Render: Font not loaded, cannot draw text in GameScene." << std::endl;
        return;
    }

    // Apply the game camera
    m_camera.applyTo(target);

    // Render the world map using WorldRenderer (it will now be drawn relative to the camera)
    m_worldRenderer.render(target);

    // --- Render other game elements that should be affected by the camera here ---

    // Reset to default view for UI elements that should not move with the camera
    target.setView(target.getDefaultView());

    sf::Text text(font, "Game Scene"); // Default construct
    text.setCharacterSize(30); // Set the character size

    text.setFillColor(sf::Color::White);
    text.setPosition(sf::Vector2f(10.f, 10.f)); 
    target.draw(text);
}

#include "GameScene.h"
#include "../SceneManager.h" 
#include "../../input/InputManager.h" // Ensure InputManager is included
#include <iostream> 
#include <cmath> 

// Constructor matches GameScene.h
GameScene::GameScene(sf::RenderWindow& window, AuthManager& authManager, InputManager& inputManager, Nakama::NClientPtr nakamaClient, Nakama::NSessionPtr session)
    : Scene(), 
      windowRef(window),
      authManagerRef(authManager),
      m_inputManager(inputManager),
      m_worldMap(50, 50), 
      m_worldRenderer(m_worldMap),
      m_camera(windowRef, 300.0f), 
      m_networking(std::make_unique<Networking>(nakamaClient)) { // Pass only nakamaClient to constructor
    std::cout << "GameScene created." << std::endl;
    m_inputManager.mapActionToKey("camera_move_up", sf::Keyboard::Key::W);
    m_inputManager.mapActionToKey("camera_move_down", sf::Keyboard::Key::S);
    m_inputManager.mapActionToKey("camera_move_left", sf::Keyboard::Key::A);
    m_inputManager.mapActionToKey("camera_move_right", sf::Keyboard::Key::D);
    m_inputManager.mapActionToMouseButton("player_move", sf::Mouse::Button::Right); // Add this line
}

GameScene::~GameScene() {
    std::cout << "GameScene destroyed." << std::endl;
}

void GameScene::onEnter(SceneManager& sceneManager) {
    std::cout << "Entering GameScene." << std::endl;
    this->sceneManager = &sceneManager; 

    Nakama::NSessionPtr session = authManagerRef.getNakamaSessionPtr();

    if (!session) {
        std::cerr << "GameScene::onEnter: Nakama session is null. Cannot initialize networking." << std::endl;
        // Optionally, switch back to LoginScene or show an error
        // manager.switchTo(SceneType::Login); 
        // For now, just log and return, preventing further execution in this scene.
        // A more robust solution might involve a specific error state or scene.
        return; 
    }

    if (m_networking) {
        if (!m_networking->initialize(session)) {
            std::cerr << "GameScene::onEnter: Failed to initialize Networking with session." << std::endl;
            // Handle initialization failure, e.g., switch to an error scene or back to login
            // manager.switchTo(SceneType::Login); // Example
            return; 
        }
        
        m_networking->setPlayerStateUpdateCallback(
            [this](const std::string& playerId, const sf::Vector2f& position, unsigned int lastProcessedSequence) {
                // std::cout << "GameScene: Player state update received for player ID: " << playerId 
                //           << " at position (" << position.x << ", " << position.y 
                //           << ") with sequence number: " << lastProcessedSequence << std::endl;
                this->handlePlayerStateUpdate(playerId, position, lastProcessedSequence);
            }
        );

        m_networking->setInputAckCallback( // Make sure to set the Input ACK callback
            [this](const std::string& playerId, unsigned int inputSequence, bool approved, const sf::Vector2f& serverPosition) {
                this->handleInputAck(playerId, inputSequence, approved, serverPosition);
            }
        );

    } else {
        std::cerr << "GameScene::onEnter: m_networking is null. This should not happen if constructor was called." << std::endl;
        return; 
    }

    m_localPlayer = std::make_unique<Player>("local_player_id", sf::Color::Black); // Placeholder ID
    // It's better to set the local player's ID from the session after it's confirmed.
    if (session) {
        m_localPlayer->setId(session->getUserId());
         std::cout << "GameScene: Local player ID set to: " << session->getUserId() << std::endl;
    }
    m_localPlayer->setPosition(sf::Vector2f(100.f, 100.f)); // Explicitly set initial position

    // The listMatches call should now be safe as Networking is initialized.
    if (m_networking) { 
        m_networking->listMatches(
            [this](const std::vector<Nakama::NMatch>& matches) {
                if (!matches.empty()) {
                    std::cout << "Found " << matches.size() << " matches. Joining the first one: " << matches[0].matchId << std::endl;
                    m_networking->joinMatch(matches[0].matchId,
                        [this, matchId = matches[0].matchId](bool success) { // Capture matchId for logging
                            if (success) {
                                std::cout << "Successfully joined match: " << matchId << std::endl;
                                // Potentially send a spawn request or initial state here if needed
                            } else {
                                std::cerr << "Failed to join match: " << matchId << ". Check server logs and ensure a match is running." << std::endl;
                                // Handle failure: e.g., show error to user, try again, or return to a previous scene.
                                // this->sceneManager->switchTo(SceneType::Login); // Example
                            }
                        }
                    );
                } else {
                    std::cout << "No matches found. Ensure a match is created on the server." << std::endl;
                    // Handle no matches found: e.g., allow user to create one, refresh, or return.
                    // this->sceneManager->switchTo(SceneType::Login); // Example
                }
            },
            [this](const Nakama::NError& error) { // Capture this for sceneManager access if needed
                std::cerr << "Error listing matches: " << error.message << std::endl;
                // Handle error listing matches: e.g., show error to user or return.
                // this->sceneManager->switchTo(SceneType::Login); // Example
            }
        );
    }
}

void GameScene::onExit(SceneManager& sceneManager) { 
    std::cout << "Exiting GameScene." << std::endl;
    if (m_networking && !m_networking->getCurrentMatchId().empty()) {
        // TODO: Implement m_networking->leaveMatch(m_networking->getCurrentMatchId());
    }
}

void GameScene::handleEvent(const sf::Event& event, SceneManager& manager) { 
    m_inputManager.handleEvent(event); 

    if (event.is<sf::Event::Closed>()) { 
        windowRef.close();
        return;
    }

    // Player input is now handled in update based on InputManager state
}

void GameScene::update(sf::Time deltaTime, SceneManager& manager) { 
    if (m_networking) {
        m_networking->tick(); 
    }

    m_camera.setMovingUp(m_inputManager.isActionActive("camera_move_up"));
    m_camera.setMovingDown(m_inputManager.isActionActive("camera_move_down"));
    m_camera.setMovingLeft(m_inputManager.isActionActive("camera_move_left"));
    m_camera.setMovingRight(m_inputManager.isActionActive("camera_move_right"));
    m_camera.update(deltaTime);

    if (m_localPlayer && m_networking) {
        // Check if the right mouse button is currently held down
        if (m_inputManager.isActionActive("player_move")) {
            sf::Vector2i mousePos = sf::Mouse::getPosition(windowRef);
            sf::Vector2f worldPos = windowRef.mapPixelToCoords(mousePos, m_camera.getView());
            sf::Vector2f newDirection = worldPos - m_localPlayer->getPosition();
            float length = std::sqrt(newDirection.x * newDirection.x + newDirection.y * newDirection.y);
            if (length > 0) {
                newDirection /= length;
            }
            m_localPlayer->setTargetDirection(newDirection);
            // Send update if direction changed or simply on each frame the button is held
            // For continuous movement, we might want to send updates regularly in `update` 
            // or only when the direction significantly changes.
            // For now, let's send an update if the direction is non-zero.
            if (newDirection.x != 0.f || newDirection.y != 0.f) {
                 m_networking->sendPlayerUpdate(m_localPlayer->getDirection(), m_localPlayer->getSpeed(), m_localPlayer->getNextSequenceNumber());
                 std::cout << "GameScene: Player movement command sent. Direction: (" << m_localPlayer->getDirection().x << ", " << m_localPlayer->getDirection().y << ") Seq: " << m_localPlayer->getNextSequenceNumber() -1 << std::endl;
            }
        } else {
            // If mouse is not pressed, stop movement by setting zero direction
            if (m_localPlayer->getDirection().x != 0.f || m_localPlayer->getDirection().y != 0.f) {
                m_localPlayer->setTargetDirection({0.f, 0.f});
                // Send an update indicating stopped movement (zero velocity)
                m_networking->sendPlayerUpdate(m_localPlayer->getDirection(), m_localPlayer->getSpeed(), m_localPlayer->getNextSequenceNumber());
                std::cout << "GameScene: Player stopped. Sending zero direction. Seq: " << m_localPlayer->getNextSequenceNumber() -1 << std::endl;
            }
        }
    }

    if (m_localPlayer) {
        m_localPlayer->update(deltaTime);
    }
    
    // Placeholder for other players map update
    for (auto& pair : m_otherPlayers) {
        pair.second->update(deltaTime);
    }

    m_inputManager.update();
}

void GameScene::render(sf::RenderTarget& target) { 
    m_camera.applyTo(target); 
    m_worldRenderer.render(target); 

    if (m_localPlayer) {
        m_localPlayer->render(target);
    }

    // Render other players
    for (const auto& pair : m_otherPlayers) {
        pair.second->render(target);
    }

    target.setView(target.getDefaultView()); 
    // TODO: Render screen-space UI (e.g. font loading, text drawing)
}

// Add the new handler method implementation
void GameScene::handlePlayerStateUpdate(const std::string& playerId, const sf::Vector2f& position, unsigned int lastProcessedSequence) {
    if (m_localPlayer && playerId == m_localPlayer->getId()) {
        m_localPlayer->handleServerUpdate(position, lastProcessedSequence);
    } else {
        // Handle updates for other players
        auto it = m_otherPlayers.find(playerId);
        if (it == m_otherPlayers.end()) {
            // New player, create and store
            std::cout << "GameScene: New player detected with ID: " << playerId << std::endl;
            auto newPlayer = std::make_unique<Player>(playerId, sf::Color::Red); // Example: other players are red
            newPlayer->setPosition(position); // Set initial position from server
            newPlayer->handleServerUpdate(position, lastProcessedSequence); // Also call handleServerUpdate for consistency
            m_otherPlayers[playerId] = std::move(newPlayer);
        } else {
            // Existing player, update
            it->second->handleServerUpdate(position, lastProcessedSequence);
        }
    }
}

void GameScene::handleInputAck(const std::string& playerId, unsigned int inputSequence, bool approved, const sf::Vector2f& serverPosition) {
    if (m_localPlayer && playerId == m_localPlayer->getId()) {
        std::cout << "GameScene: Input ACK received for local player. ID: " << playerId 
                  << ", Seq: " << inputSequence 
                  << ", Approved: " << (approved ? "Yes" : "No") 
                  << ", ServerPos: (" << serverPosition.x << ", " << serverPosition.y << ")" << std::endl;
        m_localPlayer->handleServerAck(inputSequence, approved, serverPosition);
    } else if (m_localPlayer && playerId != m_localPlayer->getId()) {
        // ACK for another player, usually not processed by other clients directly unless for specific game logic.
        // std::cout << "GameScene: Received Input ACK for another player: " << playerId << std::endl;
    } else if (!m_localPlayer) {
        std::cerr << "GameScene: Received Input ACK but local player is null. PlayerID: " << playerId << std::endl;
    }
}

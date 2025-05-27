#include <iostream>
#include "imgui-SFML.h"
#include "SceneManager.h"

SceneManager::SceneManager(sf::RenderWindow& window) : currentScene(nullptr), currentSceneType(SceneType::None), requestedSceneType(SceneType::None) {
    this->window = &window;

    // Initialize the window or any other resources if needed
    if (!ImGui::SFML::Init(window)) {
        std::cerr << "Failed to initialize ImGui-SFML" << std::endl;
        throw std::runtime_error("Failed to initialize ImGui-SFML");
    }

    std::cout << "SceneManager initialized" << std::endl;
}

SceneManager::~SceneManager() {
    // Ensure ImGui is shutdown if this SceneManager instance initialized it
    // However, ImGui::SFML::Shutdown(); should ideally be called once at the end of the application.
    // For tests, it's handled in TearDown.
    scenes.clear();
    std::cout << "SceneManager destroyed" << std::endl;
}

void SceneManager::addScene(SceneType type, std::unique_ptr<Scene> scene) {
    std::cout << "[SM_DEBUG] addScene: Attempting to add scene type: " << static_cast<int>(type) << std::endl;
    
    // Check for null scene
    if (!scene) {
        std::cerr << "[SM_DEBUG] addScene: ERROR - Attempted to add null scene for type " << static_cast<int>(type) << std::endl;
        return;
    }
    
    scene->sceneManager = this;
    scenes[type] = std::move(scene);
    std::cout << "[SM_DEBUG] addScene: Scene added. Map size: " << scenes.size() << ". Verifying find for type " << static_cast<int>(type) << "..." << std::endl;
    auto it = scenes.find(type);
    if (it != scenes.end()) {
        std::cout << "[SM_DEBUG] addScene: Verification successful: Scene type " << static_cast<int>(type) << " found in map." << std::endl;
    } else {
        std::cerr << "[SM_DEBUG] addScene: VERIFICATION FAILED: Scene type " << static_cast<int>(type) << " NOT found in map immediately after adding." << std::endl;
    }
}

void SceneManager::removeScene(SceneType type) {
    std::cout << "[SM_DEBUG] removeScene: Attempting to remove scene type: " << static_cast<int>(type) << std::endl;
    
    // Check if the scene exists
    auto it = scenes.find(type);
    if (it == scenes.end()) {
        std::cerr << "[SM_DEBUG] removeScene: Scene type " << static_cast<int>(type) << " not found in map." << std::endl;
        return;
    }
    
    // If we're removing the current scene, reset current scene pointers
    if (currentSceneType == type) {
        if (currentScene) {
            std::cout << "[SM_DEBUG] removeScene: Removing current active scene. Calling onExit." << std::endl;
            currentScene->onExit(*this);
        }
        currentScene = nullptr;
        currentSceneType = SceneType::None;
    }
    
    // Remove the scene from the map
    scenes.erase(it);
    std::cout << "[SM_DEBUG] removeScene: Scene removed. Map size: " << scenes.size() << std::endl;
}

void SceneManager::switchTo(SceneType type) {
    std::cout << "[SM_DEBUG] switchTo: Requested scene type: " << static_cast<int>(type) << std::endl;
    requestedSceneType = type;
}

SceneType SceneManager::getCurrentSceneType() const {
    return currentSceneType;
}

size_t SceneManager::getSceneCount() const {
    return scenes.size();
}

void SceneManager::processSceneSwitch() {
    std::cout << "[SM_DEBUG] processSceneSwitch: Called. requestedSceneType: " << static_cast<int>(requestedSceneType)
              << ", currentSceneType: " << static_cast<int>(currentSceneType) << std::endl;

    if (requestedSceneType != SceneType::None && requestedSceneType != currentSceneType) {
        std::cout << "[SM_DEBUG] processSceneSwitch: Condition for switching met." << std::endl;
        if (currentScene) {
            std::cout << "[SM_DEBUG] processSceneSwitch: Exiting current scene type " << static_cast<int>(currentSceneType) << std::endl;
            currentScene->onExit(*this);
        }

        auto it = scenes.find(requestedSceneType);
        if (it != scenes.end()) {
            std::cout << "[SM_DEBUG] processSceneSwitch: Scene type " << static_cast<int>(requestedSceneType) << " found in map. Switching..." << std::endl;
            currentScene = it->second.get();
            currentSceneType = requestedSceneType;
            currentScene->onEnter(*this);
            std::cout << "[SM_DEBUG] processSceneSwitch: Switched to scene: " << static_cast<int>(currentSceneType) << std::endl;
        } else {
            std::cerr << "[SM_DEBUG] processSceneSwitch: ERROR - Scene type " << static_cast<int>(requestedSceneType) << " NOT found in map." << std::endl;
            std::cerr << "[SM_DEBUG] processSceneSwitch: Current map contents (size " << scenes.size() << "):" << std::endl;
            for(const auto& pair : scenes) {
                std::cerr << "[SM_DEBUG] processSceneSwitch: Map has key: " << static_cast<int>(pair.first) << " -> points to scene: " << (pair.second ? "valid" : "invalid/null") << std::endl;
            }
            // Fallback if scene not found
            currentScene = nullptr;
            currentSceneType = SceneType::None; // This is why it reverts to None if find fails
            std::cerr << "[SM_DEBUG] processSceneSwitch: Reset currentScene to nullptr and currentSceneType to None due to find failure." << std::endl;
        }
        requestedSceneType = SceneType::None; // Reset request
        std::cout << "[SM_DEBUG] processSceneSwitch: requestedSceneType reset to None." << std::endl;
    } else {
        std::cout << "[SM_DEBUG] processSceneSwitch: Condition for switching NOT met (requested: " 
                  << static_cast<int>(requestedSceneType) << ", current: " << static_cast<int>(currentSceneType) << ")." << std::endl;
    }
}

void SceneManager::handleEvent(sf::RenderWindow& window, const sf::Event& event) {
    processSceneSwitch();
    if (currentScene) {
        currentScene->handleEvent(event, *this);
    }
    ImGui::SFML::ProcessEvent(window, event);
}

void SceneManager::update(sf::RenderWindow& window, sf::Time deltaTime) {
    processSceneSwitch();
    if (currentScene) {
        currentScene->update(deltaTime, *this);
    }
    ImGui::SFML::Update(window, deltaTime);
}

void SceneManager::render(sf::RenderWindow& target) {
    if (currentScene) {
        currentScene->render(target);
    }
    ImGui::SFML::Render(target);
}

void SceneManager::shutdown() {
    ImGui::SFML::Shutdown();
    std::cout << "SceneManager ImGui::SFML::Shutdown() called" << std::endl;
}
#include <iostream>
#include "imgui-SFML.h"
#include "imgui.h"
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
    scenes.clear();
}

void SceneManager::addScene(SceneType type, std::unique_ptr<Scene> scene) {
    scene->sceneManager = this;
    scenes[type] = std::move(scene);
}

void SceneManager::switchTo(SceneType type) {
    requestedSceneType = type;
}

SceneType SceneManager::getCurrentSceneType() const {
    return currentSceneType;
}

void SceneManager::processSceneSwitch() {
    if (requestedSceneType != SceneType::None && requestedSceneType != currentSceneType) {
        if (currentScene) {
            currentScene->onExit(*this);
        }

        auto it = scenes.find(requestedSceneType);
        if (it != scenes.end()) {
            currentScene = it->second.get();
            currentSceneType = requestedSceneType;
            currentScene->onEnter(*this);
            std::cout << "Switched to scene: " << static_cast<int>(currentSceneType) << std::endl;
        } else {
            std::cerr << "Error: Tried to switch to a non-existent scene type: " << static_cast<int>(requestedSceneType) << std::endl;
            currentScene = nullptr;
            currentSceneType = SceneType::None;
        }
        requestedSceneType = SceneType::None;
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
    std::cout << "SceneManager shutdown" << std::endl;
}
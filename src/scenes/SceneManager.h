#pragma once
#include <SFML/Graphics.hpp>
#include <memory>
#include <map>
#include "Scene.h"

class SceneManager {
    public:
        SceneManager(sf::RenderWindow& window);
        ~SceneManager();

        void addScene(SceneType type, std::unique_ptr<Scene> scene);
        void removeScene(SceneType type);
        virtual void switchTo(SceneType type); // MODIFIED - Made virtual

        SceneType getCurrentSceneType() const;
        size_t getSceneCount() const;

        void handleEvent(sf::RenderWindow& window, const sf::Event& event);
        void update(sf::RenderWindow& window, sf::Time deltaTime);
        void render(sf::RenderWindow& window);
        void shutdown();

        sf::RenderWindow* window = nullptr;

    private:
        std::map<SceneType, std::unique_ptr<Scene>> scenes;
        Scene* currentScene = nullptr;
        SceneType currentSceneType = SceneType::None;
        SceneType requestedSceneType = SceneType::None;

        void processSceneSwitch();
};
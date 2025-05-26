#pragma once
#include "../Scene.h"
#include <SFML/Graphics.hpp>
#include "../../auth/AuthManager.h"
#include "../../account/AccountManager.h"
#include <nakama-cpp/Nakama.h>

// Forward declare SceneManager to avoid circular dependency if Scene.h includes SceneManager.h
class SceneManager;

class CharacterSelectionScene : public Scene {
public:
    CharacterSelectionScene(sf::RenderWindow& window, AuthManager& authManager);

    void onEnter(SceneManager& manager) override;
    void handleEvent(const sf::Event& event, SceneManager& manager) override;
    void update(sf::Time deltaTime, SceneManager& manager) override;
    void render(sf::RenderTarget& target) override;
    void onExit(SceneManager& manager) override;

private:
    void handleCharacterListResponse(Nakama::NStorageObjectListPtr characterList);
    void handleErrorResponse(const Nakama::NError& error);
    void initializeAccountManager();

    sf::RenderWindow& windowRef;
    AuthManager& authManagerRef;
    std::unique_ptr<AccountManager> accountManager;
    SceneManager* sceneManagerRef = nullptr;
    std::vector<Nakama::NStorageObject> characters;
    std::string statusMessage;
    bool isLoading = false;
};
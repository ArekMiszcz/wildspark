#pragma once
#include "../Scene.h"
#include <SFML/Graphics.hpp>
#include "../../auth/AuthManager.h"
#include <nakama-cpp/NTypes.h>
#include <vector>
#include <string>

// Forward declare SceneManager to avoid circular dependency if Scene.h includes SceneManager.h
class SceneManager;
class AccountManager;

class CharacterSelectionScene : public Scene {
public:
    CharacterSelectionScene(sf::RenderWindow& window, AuthManager& authManager, AccountManager& accountManager);

    void onEnter(SceneManager& manager) override;
    void handleEvent(const sf::Event& event, SceneManager& manager) override;
    void update(sf::Time deltaTime, SceneManager& manager) override;
    void render(sf::RenderTarget& target) override;
    void onExit(SceneManager& manager) override;

    // Public methods for UI actions
    void selectCharacterAction(const std::string& characterId);
    void createCharacterAction();
    void backToLoginAction();

private:
    void handleCharacterListResponse(Nakama::NStorageObjectListPtr characterList);
    void handleErrorResponse(const Nakama::NError& error);

    sf::RenderWindow& windowRef;
    AuthManager& authManagerRef;
    AccountManager& accountManagerRef;
    SceneManager* sceneManagerRef = nullptr;
    std::vector<Nakama::NStorageObject> characters;
    std::string statusMessage;
    bool isLoading = false;
};
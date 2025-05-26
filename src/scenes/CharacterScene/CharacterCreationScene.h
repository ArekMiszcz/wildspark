#pragma once
#include "../Scene.h"
#include <SFML/Graphics.hpp>
#include "../../auth/AuthManager.h"
#include "../../account/AccountManager.h"
#include <nakama-cpp/NTypes.h>
#include <memory>

// Forward declare SceneManager to avoid circular dependency if Scene.h includes SceneManager.h
class SceneManager;

class CharacterCreationScene : public Scene {
public:
    CharacterCreationScene(sf::RenderWindow& window, AuthManager& authManager);
    ~CharacterCreationScene();

    void onEnter(SceneManager& manager) override;
    void handleEvent(const sf::Event& event, SceneManager& manager) override;
    void update(sf::Time deltaTime, SceneManager& manager) override;
    void render(sf::RenderTarget& target) override;
    void onExit(SceneManager& manager) override;

private:
    void initializeAccountManager();
    void handleSaveCharacterSuccess(const Nakama::NStorageObjectAcks& acks);
    void handleSaveCharacterError(const Nakama::NError& error);
    void attemptSaveCharacter();

    sf::RenderWindow& windowRef;
    AuthManager& authManagerRef;
    std::unique_ptr<AccountManager> accountManager;
    SceneManager* sceneManagerRef = nullptr;

    // UI state
    char characterName[128] = "";
    int selectedSexIndex = 0;
    const char* sexOptions[2] = {"Male", "Female"};
    std::string statusMessage = "";
    bool isSaving = false;
};
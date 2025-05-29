#pragma once
#include "../Scene.h"
#include <SFML/Graphics.hpp>
#include "../../auth/AuthManager.h"
#include <nakama-cpp/NTypes.h>

// Forward declare SceneManager to avoid circular dependency if Scene.h includes SceneManager.h
class SceneManager;
class AccountManager;

class CharacterCreationSceneTest;
class CharacterCreationSceneEdgeCasesTest;

class CharacterCreationScene : public Scene {
public:
    CharacterCreationScene(sf::RenderWindow& window, AuthManager& authManager, AccountManager& accountManager); // Modified constructor
    ~CharacterCreationScene();

    void onEnter(SceneManager& manager) override;
    void handleEvent(const sf::Event& event, SceneManager& manager) override;
    void update(sf::Time deltaTime, SceneManager& manager) override;
    void render(sf::RenderTarget& target) override;
    void onExit(SceneManager& manager) override;

    // Public methods for UI actions
    void saveCharacterAction();
    void backToSelectionAction();

    friend class CharacterCreationSceneTest;
    friend class CharacterCreationSceneEdgeCasesTest;

private:
    void handleSaveCharacterSuccess(const Nakama::NStorageObjectAcks& acks);
    void handleSaveCharacterError(const Nakama::NError& error);

    sf::RenderWindow& windowRef;
    AuthManager& authManagerRef;
    AccountManager& accountManagerRef;
    SceneManager* sceneManagerRef = nullptr;

    // UI state
    char characterName[128] = "";
    int selectedSexIndex = 0;
    const char* sexOptions[2] = {"Male", "Female"};
    std::string statusMessage = "";
    bool isSaving = false;
};
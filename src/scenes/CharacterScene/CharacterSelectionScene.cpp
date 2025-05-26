#include "CharacterSelectionScene.h"
#include <iostream>
#include "imgui.h" // For ImGui
#include "imgui-SFML.h" // For ImGui SFML integration
#include "../SceneManager.h" // For scene transitions
#include "../../auth/clients/NakamaClient.h" // For dynamic_cast to get NClientPtr and NSessionPtr

CharacterSelectionScene::CharacterSelectionScene(sf::RenderWindow& window, AuthManager& authMgr)
    : windowRef(window), authManagerRef(authMgr), accountManager(nullptr), isLoading(false) {
    std::cout << "CharacterSelectionScene initialized" << std::endl;
}

void CharacterSelectionScene::initializeAccountManager() {
    NakamaClient* nakamaClientPtr = dynamic_cast<NakamaClient*>(authManagerRef.authClient);
    if (nakamaClientPtr && nakamaClientPtr->client && nakamaClientPtr->session) {
        accountManager = std::make_unique<AccountManager>(nakamaClientPtr->client, nakamaClientPtr->session);
        std::cout << "AccountManager initialized successfully in CharacterSelectionScene." << std::endl;
    } else {
        statusMessage = "Error: Nakama client/session not available. Cannot initialize AccountManager.";
        std::cerr << statusMessage << std::endl;
        // Optionally, switch to an error scene or back to login
        // if (sceneManagerRef) sceneManagerRef->switchTo(SceneType::Login);
    }
}

void CharacterSelectionScene::onEnter(SceneManager& manager) {
    std::cout << "Entering CharacterSelectionScene" << std::endl;
    this->sceneManagerRef = &manager;
    initializeAccountManager(); // Initialize AccountManager here

    characters.clear();
    statusMessage = "Loading characters...";
    isLoading = true;

    if (accountManager) {
        accountManager->listCharacters(
            [this](Nakama::NStorageObjectListPtr characterList) {
                this->handleCharacterListResponse(characterList);
            },
            [this](const Nakama::NError& error) {
                this->handleErrorResponse(error);
            }
        );
    } else {
        statusMessage = "AccountManager not initialized. Cannot load characters.";
        isLoading = false;
        std::cerr << statusMessage << std::endl;
    }
}

void CharacterSelectionScene::handleCharacterListResponse(Nakama::NStorageObjectListPtr characterList) {
    isLoading = false;
    if (characterList && !characterList->objects.empty()) {
        characters = characterList->objects;
        statusMessage = "Select a character:";
        std::cout << "Successfully fetched " << characters.size() << " characters." << std::endl;
        for (const auto& character : characters) {
            std::cout << "Character: " << character.key << ", Value: " << character.value << std::endl;
        }
    } else {
        statusMessage = "No characters found. Please create a character.";
        std::cout << "No characters found or empty list." << std::endl;
    }
}

void CharacterSelectionScene::handleErrorResponse(const Nakama::NError& error) {
    isLoading = false;
    statusMessage = "Error fetching characters: " + error.message;
    std::cerr << "Error fetching characters: " << error.message << std::endl;
}

void CharacterSelectionScene::handleEvent(const sf::Event& event, SceneManager& manager) {
    // Event handling for character selection can be added here
    // For now, ImGui buttons will handle clicks
}

void CharacterSelectionScene::update(sf::Time deltaTime, SceneManager& manager) {
    // The authManagerRef.tick() is important for Nakama client to process responses
    authManagerRef.tick(); 
}

void CharacterSelectionScene::render(sf::RenderTarget& target) {
    ImGui::SetNextWindowSize(ImVec2(400, 300), ImGuiCond_FirstUseEver);
    ImGui::Begin("Character Selection", nullptr, ImGuiWindowFlags_AlwaysAutoResize);

    ImGui::Text("%s", statusMessage.c_str());

    if (isLoading) {
        ImGui::Text("Loading...");
    } else if (accountManager) { // Only show character list if accountManager is valid
        if (!characters.empty()) {
            for (const auto& character : characters) {
                std::string characterName = "Character ID: " + character.key;
                // TODO: Parse character.value (JSON) to get character name, class, etc.

                if (ImGui::Button(characterName.c_str())) {
                    std::cout << "Selected character: " << character.key << std::endl;
                    if (sceneManagerRef) {
                        sceneManagerRef->switchTo(SceneType::Game);
                    }
                }
            }
        } else {
             if (ImGui::Button("Create Character")) {
                if (sceneManagerRef) {
                    sceneManagerRef->switchTo(SceneType::CharacterCreation);
                }
            }
        }
    } else {
        ImGui::Text("Could not load character data. AccountManager failed to initialize.");
    }
    
    if (ImGui::Button("Back to Login")) {
        if (sceneManagerRef) {
            sceneManagerRef->switchTo(SceneType::Login);
        }
    }

    ImGui::End();
}

void CharacterSelectionScene::onExit(SceneManager& manager) {
    std::cout << "Exiting CharacterSelectionScene" << std::endl;
    accountManager.reset(); // Release the AccountManager
}
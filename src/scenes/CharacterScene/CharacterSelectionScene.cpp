// Copyright 2025 WildSpark Authors

#include "CharacterSelectionScene.h"
#include <iostream>
#include <string>
#include "imgui.h"
#include "imgui-SFML.h"
#include "../SceneManager.h"
#include "../../account/AccountManager.h"

CharacterSelectionScene::CharacterSelectionScene(sf::RenderWindow& window, AuthManager& authMgr, AccountManager& accMgr)
    : windowRef(window), authManagerRef(authMgr), accountManagerRef(accMgr), isLoading(false) {
    std::cout << "CharacterSelectionScene initialized" << std::endl;
}

void CharacterSelectionScene::onEnter(SceneManager& manager) {
    std::cout << "Entering CharacterSelectionScene" << std::endl;
    this->sceneManagerRef = &manager;

    characters.clear();
    statusMessage = "Loading characters...";
    isLoading = true;

    // Use the injected accountManagerRef
    accountManagerRef.listCharacters(
        [this](Nakama::NStorageObjectListPtr characterList) {
            this->handleCharacterListResponse(characterList);
        },
        [this](const Nakama::NError& error) {
            this->handleErrorResponse(error);
        });
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
    // For now, ImGui buttons will handle clicks via the new action methods
}

void CharacterSelectionScene::update(sf::Time deltaTime, SceneManager& manager) {
    authManagerRef.tick();
}

void CharacterSelectionScene::render(sf::RenderTarget& target) {
    ImGui::SetNextWindowSize(ImVec2(400, 300), ImGuiCond_FirstUseEver);
    ImGui::Begin("Character Selection", nullptr, ImGuiWindowFlags_AlwaysAutoResize);

    ImGui::Text("%s", statusMessage.c_str());

    if (isLoading) {
        ImGui::Text("Loading...");
    } else {
        if (!characters.empty()) {
            for (const auto& character : characters) {
                std::string characterName = "Character ID: " + character.key;

                if (ImGui::Button(characterName.c_str())) {
                    selectCharacterAction(character.key);
                }
            }
        } else {
             if (ImGui::Button("Create Character")) {
                createCharacterAction();
            }
        }
    }

    if (ImGui::Button("Back to Login")) {
        backToLoginAction();
    }

    ImGui::End();
}

void CharacterSelectionScene::onExit(SceneManager& manager) {
    std::cout << "Exiting CharacterSelectionScene" << std::endl;
}

void CharacterSelectionScene::selectCharacterAction(const std::string& characterId) {
    std::cout << "Selected character: " << characterId << std::endl;
    if (characterId.empty()) {
        std::cerr << "Invalid character ID: empty string" << std::endl;
        statusMessage = "Invalid character ID selected.";
        return;
    }

    bool found = false;
    for (const auto& character : characters) {
        if (character.key == characterId) {
            found = true;
            break;
        }
    }

    if (!found) {
        std::cerr << "Invalid character ID: " << characterId << " not found." << std::endl;
        statusMessage = "Selected character not found.";
        return;
    }

    if (sceneManagerRef) {
        sceneManagerRef->switchTo(SceneType::Game);
    }
}

void CharacterSelectionScene::createCharacterAction() {
    if (sceneManagerRef) {
        sceneManagerRef->switchTo(SceneType::CharacterCreation);
    }
}

void CharacterSelectionScene::backToLoginAction() {
    if (sceneManagerRef) {
        sceneManagerRef->switchTo(SceneType::Login);
    }
}

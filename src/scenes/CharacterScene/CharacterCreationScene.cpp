// Copyright 2025 WildSpark Authors

#include "CharacterCreationScene.h"
#include <iostream>
#include <cstring>
#include <algorithm>
#include <string>
#include "imgui.h"
#include "imgui-SFML.h"
#include "../SceneManager.h"
#include "../../account/AccountManager.h"

CharacterCreationScene::CharacterCreationScene(sf::RenderWindow& window, AuthManager& authMgr, AccountManager& accMgr)
    : windowRef(window), authManagerRef(authMgr), accountManagerRef(accMgr), sceneManagerRef(nullptr), isSaving(false) {
    std::cout << "CharacterCreationScene initialized" << std::endl;
    memset(characterName, 0, sizeof(characterName));
}

CharacterCreationScene::~CharacterCreationScene() {
    std::cout << "CharacterCreationScene destroyed" << std::endl;
}

void CharacterCreationScene::onEnter(SceneManager& manager) {
    sceneManagerRef = &manager;
    statusMessage = "";
    isSaving = false;
    memset(characterName, 0, sizeof(characterName));
    selectedSexIndex = 0;
    std::cout << "Entering CharacterCreationScene" << std::endl;
}

void CharacterCreationScene::saveCharacterAction() {
    if (isSaving) return;

    std::string nameStr(characterName);
    nameStr.erase(0, nameStr.find_first_not_of(" \t\n\r\f\v"));
    nameStr.erase(nameStr.find_last_not_of(" \t\n\r\f\v") + 1);

    if (nameStr.empty()) {
        statusMessage = "Character name cannot be empty or only spaces.";
        return;
    }

    std::string originalNameStr(characterName);
    if (nameStr.empty() && std::all_of(originalNameStr.begin(), originalNameStr.end(), ::isspace)) {
        statusMessage = "Character name cannot consist only of spaces.";
        return;
    }

    isSaving = true;
    statusMessage = "Saving character...";

    std::string sexStr(sexOptions[selectedSexIndex]);

    accountManagerRef.saveCharacter(nameStr, sexStr,
        [this](const Nakama::NStorageObjectAcks& acks) {
            this->handleSaveCharacterSuccess(acks);
        },
        [this](const Nakama::NError& error) {
            this->handleSaveCharacterError(error);
        });
}

void CharacterCreationScene::backToSelectionAction() {
    if (sceneManagerRef) {
        sceneManagerRef->switchTo(SceneType::CharacterSelection);
    }
}

void CharacterCreationScene::handleSaveCharacterSuccess(const Nakama::NStorageObjectAcks& acks) {
    isSaving = false;
    if (!acks.empty()) {
        // Assuming single object write, check first ack
        std::cout << "Character saved successfully! Key: " << acks[0].key << std::endl;
        statusMessage = "Character saved successfully!";
        if (sceneManagerRef) {
            sceneManagerRef->switchTo(SceneType::CharacterSelection);
        }
    } else {
        statusMessage = "Character saved, but no acknowledgment received.";
        std::cerr << "Character saved, but no acknowledgment received." << std::endl;
    }
}

void CharacterCreationScene::handleSaveCharacterError(const Nakama::NError& error) {
    isSaving = false;
    statusMessage = "Error saving character: " + error.message;
    std::cerr << "Error saving character: " << error.message << std::endl;
}

void CharacterCreationScene::handleEvent(const sf::Event& event, SceneManager& manager) {
    // ImGui::SFML::ProcessEvent(event); // Already handled by SceneManager
}

void CharacterCreationScene::update(sf::Time deltaTime, SceneManager& manager) {
    // authManagerRef.tick(); // Global tick in main.cpp
}

void CharacterCreationScene::render(sf::RenderTarget& target) {
    ImGui::SetNextWindowSize(ImVec2(400, 300), ImGuiCond_FirstUseEver);
    ImGui::Begin("Character Creation", nullptr, ImGuiWindowFlags_AlwaysAutoResize);

    ImGui::Text("%s", statusMessage.c_str());

    if (isSaving) {
        ImGui::Text("Processing...");
    } else {
        ImGui::InputText("Name", characterName, sizeof(characterName));
        if (ImGui::BeginCombo("Sex", sexOptions[selectedSexIndex])) {
            for (int i = 0; i < IM_ARRAYSIZE(sexOptions); i++) {
                const bool is_selected = (selectedSexIndex == i);
                if (ImGui::Selectable(sexOptions[i], is_selected))
                    selectedSexIndex = i;
                if (is_selected)
                    ImGui::SetItemDefaultFocus();
            }
            ImGui::EndCombo();
        }

        if (ImGui::Button("Save Character")) {
            saveCharacterAction();
        }
        ImGui::SameLine();
        if (ImGui::Button("Back to Selection")) {
            backToSelectionAction();
        }
    }

    ImGui::End();
}

void CharacterCreationScene::onExit(SceneManager& manager) {
    std::cout << "Exiting CharacterCreationScene" << std::endl;
}

#include "CharacterCreationScene.h"
#include <iostream>
#include "imgui.h"
#include "imgui-SFML.h"
#include "../SceneManager.h"
#include "../../auth/clients/NakamaClient.h" // For dynamic_cast for AccountManager init

CharacterCreationScene::CharacterCreationScene(sf::RenderWindow& window, AuthManager& authMgr)
    : windowRef(window), authManagerRef(authMgr), accountManager(nullptr), sceneManagerRef(nullptr), isSaving(false) {
    std::cout << "CharacterCreationScene initialized" << std::endl;
    characterName[0] = '\0'; // Ensure buffer is initially empty
}

CharacterCreationScene::~CharacterCreationScene() {
    std::cout << "CharacterCreationScene destroyed" << std::endl;
    // accountManager unique_ptr will auto-cleanup
}

void CharacterCreationScene::initializeAccountManager() {
    NakamaClient* nakamaClientPtr = dynamic_cast<NakamaClient*>(authManagerRef.authClient);
    if (nakamaClientPtr && nakamaClientPtr->client && nakamaClientPtr->session) {
        accountManager = std::make_unique<AccountManager>(nakamaClientPtr->client, nakamaClientPtr->session);
        std::cout << "AccountManager initialized successfully in CharacterCreationScene." << std::endl;
    } else {
        statusMessage = "Error: Nakama client/session not available. Cannot initialize AccountManager.";
        std::cerr << statusMessage << std::endl;
    }
}

void CharacterCreationScene::onEnter(SceneManager& manager) {
    std::cout << "Entering CharacterCreationScene" << std::endl;
    this->sceneManagerRef = &manager;
    initializeAccountManager();
    characterName[0] = '\0'; // Clear name field on enter
    selectedSexIndex = 0;
    statusMessage = "Create your character:";
    isSaving = false;
}

void CharacterCreationScene::attemptSaveCharacter() {
    if (!accountManager) {
        statusMessage = "Error: AccountManager not ready.";
        return;
    }
    if (std::string(characterName).empty()) {
        statusMessage = "Character name cannot be empty.";
        return;
    }

    isSaving = true;
    statusMessage = "Saving character...";
    std::string nameStr(characterName);
    std::string sexStr(sexOptions[selectedSexIndex]);

    accountManager->saveCharacter(nameStr, sexStr,
        [this](const Nakama::NStorageObjectAcks& acks) {
            this->handleSaveCharacterSuccess(acks);
        },
        [this](const Nakama::NError& error) {
            this->handleSaveCharacterError(error);
        }
    );
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
            attemptSaveCharacter();
        }
        ImGui::SameLine();
        if (ImGui::Button("Back to Selection")) {
            if (sceneManagerRef) {
                sceneManagerRef->switchTo(SceneType::CharacterSelection);
            }
        }
    }

    ImGui::End();
}

void CharacterCreationScene::onExit(SceneManager& manager) {
    std::cout << "Exiting CharacterCreationScene" << std::endl;
    accountManager.reset(); // Clean up AccountManager
}
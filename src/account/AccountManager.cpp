// Copyright 2025 WildSpark Authors

#include <iostream>
#include <string>
#include <vector>
#include "AccountManager.h"
#include "../auth/AuthManager.h"

AccountManager::AccountManager(AuthManager& authManager)
    : authManagerRef(authManager) {
    std::cout << "AccountManager initialized." << std::endl;
}

AccountManager::~AccountManager() {
    std::cout << "AccountManager destroyed." << std::endl;
}

void AccountManager::listCharacters(
    std::function<void(Nakama::NStorageObjectListPtr)> successCallback,
    std::function<void(const Nakama::NError&)> errorCallback
) {
    Nakama::NClientPtr client = authManagerRef.getNakamaClientPtr();
    Nakama::NSessionPtr session = authManagerRef.getNakamaSessionPtr();

    if (!client || !session) {
        if (errorCallback) {
            Nakama::NError error;
            error.message = "Nakama client or session not available in AccountManager.";
            error.code = Nakama::ErrorCode::Unknown;
            errorCallback(error);
        }
        return;
    }

    client->listUsersStorageObjects(
        session,
        characterCollection,
        session->getUserId(),
        100,
        "",
        successCallback,
        errorCallback);

    std::cout << "AccountManager: Requesting character list from collection '"
              << characterCollection
              << "' for user " << session->getUserId()
              << std::endl;
}

void AccountManager::saveCharacter(
    const std::string& name,
    const std::string& sex,
    std::function<void(const Nakama::NStorageObjectAcks&)> successCallback,
    std::function<void(const Nakama::NError&)> errorCallback
) {
    Nakama::NClientPtr client = authManagerRef.getNakamaClientPtr();
    Nakama::NSessionPtr session = authManagerRef.getNakamaSessionPtr();

    if (!client || !session) {
        if (errorCallback) {
            Nakama::NError error;
            error.message = "Nakama client or session not available in AccountManager.";
            error.code = Nakama::ErrorCode::Unknown;
            errorCallback(error);
        }
        return;
    }

    std::string characterDataJson = "{\"name\":\"" + name + "\",\"sex\":\"" + sex + "\"}";

    std::vector<Nakama::NStorageObjectWrite> objectsToWrite;
    Nakama::NStorageObjectWrite newCharacter;
    newCharacter.collection = characterCollection;
    newCharacter.key = name;
    newCharacter.value = characterDataJson;
    objectsToWrite.push_back(newCharacter);

    std::cout << "AccountManager: Attempting to save character '" << name
              << "' with data: " << characterDataJson << std::endl;

    client->writeStorageObjects(session, objectsToWrite, successCallback, errorCallback);
}

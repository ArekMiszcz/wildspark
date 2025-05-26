#include "AccountManager.h"
#include <iostream> // For debugging, can be removed later

AccountManager::AccountManager(Nakama::NClientPtr client, Nakama::NSessionPtr session)
    : nakamaClient(client), nakamaSession(session) {
    std::cout << "AccountManager initialized." << std::endl;
}

AccountManager::~AccountManager() {
    std::cout << "AccountManager destroyed." << std::endl;
}

void AccountManager::listCharacters(
    std::function<void(Nakama::NStorageObjectListPtr)> successCallback,
    std::function<void(const Nakama::NError&)> errorCallback
) {
    if (!nakamaClient || !nakamaSession) {
        if (errorCallback) {
            Nakama::NError error;
            error.message = "Nakama client or session not available in AccountManager.";
            error.code = Nakama::ErrorCode::Unknown; // Or a more specific error code
            errorCallback(error);
        }
        return;
    }

    nakamaClient->listUsersStorageObjects(
        nakamaSession,
        characterCollection,
        nakamaSession->getUserId(), // List objects for the current user
        100, // Limit for characters, adjust as needed
        "",  // Cursor for pagination, empty for the first page
        successCallback,
        errorCallback
    );
    std::cout << "AccountManager: Requesting character list from collection '" << characterCollection << "' for user " << nakamaSession->getUserId() << std::endl;
}

void AccountManager::saveCharacter(
    const std::string& name,
    const std::string& sex,
    std::function<void(const Nakama::NStorageObjectAcks&)> successCallback,
    std::function<void(const Nakama::NError&)> errorCallback
) {
    if (!nakamaClient || !nakamaSession) {
        if (errorCallback) {
            Nakama::NError error;
            error.message = "Nakama client or session not available in AccountManager.";
            error.code = Nakama::ErrorCode::Unknown;
            errorCallback(error);
        }
        return;
    }

    std::string characterDataJson = "{\"name\":\"" + name + "\", \"sex\":\"" + sex + "\"}";

    std::vector<Nakama::NStorageObjectWrite> objectsToWrite;
    Nakama::NStorageObjectWrite newCharacter;
    newCharacter.collection = characterCollection;
    newCharacter.key = name;
    newCharacter.value = characterDataJson;
    objectsToWrite.push_back(newCharacter);

    std::cout << "AccountManager: Attempting to save character '" << name << "' with data: " << characterDataJson << std::endl;

    nakamaClient->writeStorageObjects(nakamaSession, objectsToWrite, successCallback, errorCallback);
} 
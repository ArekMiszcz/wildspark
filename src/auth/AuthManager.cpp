// Copyright 2025 WildSpark Authors

#include "AuthManager.h"
#include <iostream>
#include <string>
#include "clients/NakamaClient.h"
#include "../vendor/dotenv-cpp/dotenv.h"

AuthManager::AuthManager(ConstructionMode mode) : authClient(nullptr) {
    if (mode == ConstructionMode::NORMAL) {
        try {
            dotenv::init();
        } catch (const std::exception& e) {
            std::cerr << "AuthManager: Exception during dotenv::init(): " << e.what() << std::endl;
        }

        authClient = new NakamaClient();
    }
}

AuthManager::~AuthManager() {
    delete authClient;
}

void AuthManager::attemptLogin(const std::string& email, const std::string& password, LoginResultCallback callback) {
    if (authClient) {
        authClient->connect(email, password, callback);
    } else {
        std::cerr << "AuthManager: authClient is null!" << std::endl;
        if (callback) {
            callback(false, "Internal error: Auth client not available.");
        }
    }
}

void AuthManager::tick() {
    NakamaClient* nakamaClientPtr = dynamic_cast<NakamaClient*>(authClient);
    if (nakamaClientPtr) {
        nakamaClientPtr->tick();
    } else if (authClient) {
        // If it's some other AuthClient that also needs ticking, handle here or add to interface.
    }
}

Nakama::NRtClientPtr AuthManager::getRtClient() {
    NakamaClient* nakamaClientPtr = dynamic_cast<NakamaClient*>(authClient);
    if (nakamaClientPtr) {
        return nakamaClientPtr->getRtClient();
    }
    return nullptr;
}

Nakama::NClientPtr AuthManager::getNakamaClientPtr() {
    NakamaClient* nakamaClientPtr = dynamic_cast<NakamaClient*>(authClient);
    if (nakamaClientPtr) {
        return nakamaClientPtr->client;
    }
    return nullptr;
}

Nakama::NSessionPtr AuthManager::getNakamaSessionPtr() {
    NakamaClient* nakamaClientPtr = dynamic_cast<NakamaClient*>(authClient);
    if (nakamaClientPtr) {
        return nakamaClientPtr->session;
    }
    return nullptr;
}

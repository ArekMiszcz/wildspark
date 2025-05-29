#include "AuthManager.h"
#include <iostream> // Keep for std::cerr, or replace with a dedicated logger
#include "../vendor/dotenv-cpp/dotenv.h" // Include dotenv.h
#include "clients/NakamaClient.h"

AuthManager::AuthManager(ConstructionMode mode) : authClient(nullptr) {
    if (mode == ConstructionMode::NORMAL) {
        // Load .env file
        // Note: dotenv::init() might throw an exception if .env is not found or is malformed.
        // Depending on desired behavior, you might want to wrap this in a try-catch.
        try {
            dotenv::init(); // Correct function to load .env
        } catch (const std::exception& e) {
            std::cerr << "AuthManager: Exception during dotenv::init(): " << e.what() << std::endl;
            // Decide if this is a fatal error for NORMAL mode or if the application can proceed
            // For now, we'll log and continue, as NakamaClient might use default values or environment variables.
        }
        
        // Initialize Nakama client
        authClient = new NakamaClient(); // This line creates NakamaClient
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
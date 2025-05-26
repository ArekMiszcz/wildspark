#include "AuthManager.h"
#include <iostream> // For std::cout, remove in real implementation
#include "../vendor/dotenv-cpp/dotenv.h" // Include dotenv.h
#include "clients/NakamaClient.h"

AuthManager::AuthManager() {
    // Load .env file. Assuming .env is in the parent directory of the executable (e.g., project root)
    // Adjust the path "../.env" if your executable is in a different location relative to .env
    dotenv::init("../../.env"); 
    // Instantiate NakamaClient and assign to AuthClient pointer
    // This relies on NakamaClient inheriting from AuthClient
    authClient = new NakamaClient(); 
    std::cout << "AuthManager initialized and .env loaded. NakamaClient created." << std::endl;
}

AuthManager::~AuthManager() {
    delete authClient; // This will call NakamaClient's destructor
}

void AuthManager::attemptLogin(const std::string& email, const std::string& password, LoginResultCallback callback) {
    std::cout << "AuthManager: Attempting login for " << email << std::endl;
    if (authClient) {
        authClient->connect(email, password, callback); // Pass the callback along
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
    return nullptr; // Or handle error appropriately
}
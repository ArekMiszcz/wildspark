#include "AuthManager.h"
#include <iostream> // For std::cout, remove in real implementation
#include <cstdlib> // For std::getenv
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
    // Downcast needed if NakamaClient has specific tick, or AuthClient interface needs tick()
    // For now, assuming NakamaClient specific tick logic not exposed via AuthClient interface directly
    // If AuthClient had a virtual tick(), we could call authClient->tick()
    // Casting to NakamaClient to call its specific tick method.
    // This is safe if we are sure authClient is always a NakamaClient.
    NakamaClient* nakamaClientPtr = dynamic_cast<NakamaClient*>(authClient);
    if (nakamaClientPtr) {
        nakamaClientPtr->tick();
    } else if (authClient) {
        // If it's some other AuthClient that also needs ticking, handle here or add to interface.
        // For now, only NakamaClient has a tick we know about.
    }
} 
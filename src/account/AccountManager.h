#pragma once

#include <nakama-cpp/Nakama.h>
#include <nakama-cpp/NTypes.h>
#include <nakama-cpp/NClientInterface.h>
#include <functional>
#include <string>

// Forward declare AuthManager to avoid circular dependency if AuthManager.h includes AccountManager.h
class AuthManager;

class AccountManager {
public:
    // Constructor now takes a reference to AuthManager
    AccountManager(AuthManager& authManager);
    virtual ~AccountManager(); // Ensure virtual destructor

    // Method to list characters (storage objects)
    virtual void listCharacters(
        std::function<void(Nakama::NStorageObjectListPtr)> successCallback,
        std::function<void(const Nakama::NError&)> errorCallback
    );

    // Method to save a new character
    virtual void saveCharacter(
        const std::string& name,
        const std::string& sex,
        std::function<void(const Nakama::NStorageObjectAcks&)> successCallback,
        std::function<void(const Nakama::NError&)> errorCallback
    );

    // We can add other account-related methods here later,
    // e.g., getAccountDetails, updateAccount, etc.

private:
    // Store a reference to AuthManager
    AuthManager& authManagerRef;
    const std::string characterCollection = "characters"; // Define collection name
};
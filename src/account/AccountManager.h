#pragma once

#include <nakama-cpp/Nakama.h>
#include <functional>
#include <string>

class AccountManager {
public:
    AccountManager(Nakama::NClientPtr client, Nakama::NSessionPtr session);
    ~AccountManager();

    // Method to list characters (storage objects)
    void listCharacters(
        std::function<void(Nakama::NStorageObjectListPtr)> successCallback,
        std::function<void(const Nakama::NError&)> errorCallback
    );

    // We can add other account-related methods here later,
    // e.g., getAccountDetails, updateAccount, etc.

private:
    Nakama::NClientPtr nakamaClient;
    Nakama::NSessionPtr nakamaSession;
    const std::string characterCollection = "characters"; // Define collection name
}; 
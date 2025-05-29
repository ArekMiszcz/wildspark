#pragma once

#include "gmock/gmock.h"
#include "account/AccountManager.h"
#include <nakama-cpp/NTypes.h>

// Forward declare AuthManager to avoid circular dependency if MockAuthManager includes this file
class AuthManager;

class MockAccountManager : public AccountManager {
public:
    MockAccountManager(AuthManager& authManager) 
        : AccountManager(authManager) {}

    MOCK_METHOD(void, listCharacters, (std::function<void(Nakama::NStorageObjectListPtr)> successCallback, std::function<void(const Nakama::NError&)> errorCallback), (override));
    MOCK_METHOD(void, saveCharacter, (const std::string& name, const std::string& sex, std::function<void(const Nakama::NStorageObjectAcks&)> successCallback, std::function<void(const Nakama::NError&)> errorCallback), (override));
};

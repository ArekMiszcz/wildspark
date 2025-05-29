#pragma once

#include "gmock/gmock.h"
#include "auth/AuthManager.h"
#include <functional>

// Mock for AuthManager
class MockAuthManager : public AuthManager {
public:
    MockAuthManager() : AuthManager(AuthManager::ConstructionMode::TESTING) {}
    ~MockAuthManager() override {}

    void attemptLogin(const std::string& email, const std::string& password, std::function<void(bool, const std::string&)> callback) override {
        mockableAttemptLogin(email, password, callback);
    }

    MOCK_METHOD(void, mockableAttemptLogin, (const std::string& email, const std::string& password, const std::function<void(bool, const std::string&)>& callback));
};

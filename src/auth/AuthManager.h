#pragma once
#include <string>
#include "AuthClient.h"
#include <nakama-cpp/Nakama.h> 

class AuthManager {
public:
    AuthManager();
    ~AuthManager();

    AuthClient* authClient; // This will be a NakamaClient instance

    // Changed signature to accept a callback
    void attemptLogin(const std::string& email, const std::string& password, LoginResultCallback callback);
    void tick(); // To pass through to NakamaClient
    Nakama::NRtClientPtr getRtClient(); // <-- ADD THIS

private:
    // Potentially store auth tokens, user data, etc. here
};
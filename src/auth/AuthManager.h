#pragma once
#include <string>
#include "AuthClient.h"
#include <nakama-cpp/Nakama.h> 

class AuthManager {
public:
    enum class ConstructionMode { NORMAL, TESTING };
    AuthManager(ConstructionMode mode = ConstructionMode::NORMAL);
    virtual ~AuthManager();
    AuthClient* authClient; 

    virtual void attemptLogin(const std::string& email, const std::string& password, LoginResultCallback callback);
    virtual void tick(); 
    Nakama::NRtClientPtr getRtClient();

    Nakama::NClientPtr getNakamaClientPtr();
    Nakama::NSessionPtr getNakamaSessionPtr();

private:
    // Potentially store auth tokens, user data, etc. here
};
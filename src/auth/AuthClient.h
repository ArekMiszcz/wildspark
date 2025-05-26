#pragma once
#include <string>
#include <functional> // For std::function

// Callback type for login results
using LoginResultCallback = std::function<void(bool success, const std::string& message)>;

class AuthClient
{
    public:
        AuthClient() = default;
        virtual ~AuthClient() = default;

        virtual void connect(const std::string& email, const std::string& password, LoginResultCallback callback) = 0;
        virtual void disconnect() = 0;
        virtual std::string getSessionToken() = 0;
};
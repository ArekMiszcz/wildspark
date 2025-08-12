// Copyright 2025 WildSpark Authors

#include "NakamaClient.h"
#include <nakama-cpp/NError.h>
#include <nakama-cpp/Nakama.h>
#include <nakama-cpp/realtime/NRtDefaultClientListener.h>
#include <iostream>
#include <string>
#include "../../vendor/dotenv-cpp/dotenv.h"

NakamaClient::NakamaClient() : AuthClient() {
    Nakama::NLogger::initWithConsoleSink(Nakama::NLogLevel::Debug);
    parameters.serverKey = dotenv::getenv("NAKAMA_SERVER_KEY", "defaultkey");
    parameters.host = dotenv::getenv("NAKAMA_SERVER_HOST", "127.0.0.1");
    parameters.port = Nakama::DEFAULT_PORT;
    client = Nakama::createDefaultClient(parameters);
    _isRunning = true;
    std::cout << "NakamaClient initialized" << std::endl;
}

NakamaClient::~NakamaClient() {
    _isRunning = false;
    if (session) {
        // Consider graceful logout if applicable
        // client->disconnect(session, nullptr); // Example, might need session
    }
    std::cout << "NakamaClient destroyed" << std::endl;
}

void NakamaClient::tick() {
    if (client && _isRunning) {
        client->tick();
    }
}

void NakamaClient::connect(const std::string& email, const std::string& password, LoginResultCallback callback) {
    if (!client) {
        if (callback) callback(false, "Nakama client not initialized.");
        return;
    }

    std::string username = email;
    bool create = true;
    Nakama::NStringMap vars;

    auto successCallback = [this, callback](Nakama::NSessionPtr newSession) {
        std::cout << "Nakama: Login successful. Session token: " << newSession->getAuthToken() << std::endl;
        this->session = newSession;
        if (callback) callback(true, "Login successful.");
    };

    auto errorCallback = [callback](const Nakama::NError& error) {
        std::string errorMessage = error.message;

        if (error.code == Nakama::ErrorCode::ConnectionError) {
            errorMessage = "Connection error. Check internet connection and try again.";
        }

        if (callback) callback(false, "Login failed: " + errorMessage);
    };

    std::string deviceId = dotenv::getenv("NAKAMA_DEVICE_ID");
    if (deviceId.empty()) {
         deviceId = email;
         std::cout << "Warning: NAKAMA_DEVICE_ID not set, using email as fallback (not recommended)." << std::endl;
    }

    client->authenticateEmail(
        email,
        password,
        username,
        create,
        vars,
        successCallback,
        errorCallback);

    std::cout << "NakamaClient: Authentication request sent for " << email << std::endl;
}

std::string NakamaClient::getSessionToken() {
    if (session) {
        return session->getAuthToken();
    }
    return "";
}

Nakama::NRtClientPtr NakamaClient::getRtClient() {
    if (!rtClient) {
        rtClient = client->createRtClient();
        if (session) {
            rtClient->connect(session, false);
            std::cout << "NakamaClient: Real-time client connected with session token: "
                      << session->getAuthToken() << std::endl;
        }
    }
    return rtClient;
}

void NakamaClient::disconnect() {
    _isRunning = false;
    if (client && session) {
        client->disconnect();
        session = nullptr;
        if (rtClient) {
            rtClient->disconnect();
            rtClient = nullptr;
        }
    } else {
        std::cout << "NakamaClient: Not connected or no session to disconnect." << std::endl;
    }
}

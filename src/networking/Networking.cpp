// Copyright 2025 WildSpark Authors

#include "Networking.h"
#include <iostream>
#include <vector>
#include <memory>
#include <string>
#include <nlohmann/json.hpp>

InternalRtListener::InternalRtListener(Networking* networkingService) : m_networkingService(networkingService) {}

void InternalRtListener::onConnect() {
    std::cout << "Socket connected" << std::endl;
    if (m_networkingService) {
        m_networkingService->completePendingMatchJoin();
    }
}

void InternalRtListener::onDisconnect(const Nakama::NRtClientDisconnectInfo& info) {
    std::cout << "Socket disconnected. Code: " << info.code << ", Reason: " << info.reason << std::endl;
}

void InternalRtListener::onError(const Nakama::NRtError& error) {
    std::cerr << "Socket error: " << error.message << " (Code: " << static_cast<int>(error.code) << ")" << std::endl;
}

void InternalRtListener::onMatchData(const Nakama::NMatchData& data) {
    std::cout << "Received match data from user: " << data.presence.userId
              << " with op code: " << data.opCode
              << ", data: " << std::string(data.data.begin(), data.data.end()) << std::endl;


    if (m_networkingService) {
        try {
            std::string jsonDataStr(data.data.begin(), data.data.end());
            nlohmann::json gameMessage = nlohmann::json::parse(jsonDataStr);
            std::string messageType = gameMessage.value("type", "");
            nlohmann::json messageData = gameMessage.value("data", nlohmann::json::object());


            if (data.opCode == 2 && messageType == "world_update") {
                if (m_networkingService->m_onPlayerStateUpdateCallback) {
                    if (messageData.contains("players") && messageData["players"].is_object()) {
                        for (auto& [playerId, playerDataJson] : messageData["players"].items()) {
                            if (playerDataJson.contains("position")) {
                                float x = playerDataJson["position"].value("x", 0.0f);
                                float y = playerDataJson["position"].value("y", 0.0f);
                                m_networkingService->m_onPlayerStateUpdateCallback(playerId, sf::Vector2f(x, y), 0);
                            }
                        }
                    }
                }
            } else if (data.opCode == 4 && messageType == "input_ack") {
                 if (m_networkingService->m_onInputAckCallback) {
                    std::string ackPlayerId = messageData.value("playerId", "");
                    unsigned int inputSequence = messageData.value("inputSequence", 0u);
                    bool approved = messageData.value("approved", false);
                    float x = messageData.value("x", 0.0f);
                    float y = messageData.value("y", 0.0f);

                    if (!ackPlayerId.empty()) {
                        m_networkingService->m_onInputAckCallback(
                            ackPlayerId,
                            inputSequence,
                            approved,
                            sf::Vector2f(x, y));
                    }
                }
            }
        } catch (const nlohmann::json::parse_error& e) {
            std::cerr << "Failed to parse match data JSON: " << e.what()
                      << " Raw data: " << std::string(data.data.begin(), data.data.end()) << std::endl;
        }
    }
}

Networking::Networking(Nakama::NClientPtr nakamaClientPtr)
    : m_nakamaClientPtr(nakamaClientPtr), m_sessionPtr(nullptr), m_rtClient(nullptr), m_listener(nullptr),
      m_onPlayerStateUpdateCallback(nullptr), m_onInputAckCallback(nullptr) {
    if (!m_nakamaClientPtr) {
        std::cerr << "Networking Constructor Error: Nakama::NClientPtr is null!" << std::endl;
    }
    std::cout << "Networking instance created (session not yet initialized)." << std::endl;
}

Networking::~Networking() {
    if (m_rtClient && m_rtClient->isConnected()) {
        m_rtClient->disconnect();
    }
    std::cout << "Networking instance destroyed." << std::endl;
}

bool Networking::initialize(Nakama::NSessionPtr sessionPtr) {
    if (!sessionPtr) {
        std::cerr << "Networking Initialize Error: NSessionPtr is null!" << std::endl;
        return false;
    }
    if (!m_nakamaClientPtr) {
        std::cerr << "Networking Initialize Error: NClientPtr is null "
                  << "(should have been set in constructor)!" << std::endl;
        return false;
    }
    m_sessionPtr = sessionPtr;
    m_listener = std::make_shared<InternalRtListener>(this);
    std::cout << "Networking initialized with session and listener." << std::endl;
    return true;
}

void Networking::listMatches(std::function<void(const std::vector<Nakama::NMatch>&)> successCallback,
                           std::function<void(const Nakama::NError&)> errorCallback) {
    if (!m_nakamaClientPtr || !m_sessionPtr || !m_listener) {
        std::cerr << "ListMatches Error: Networking not properly initialized "
                  << "(client, session, or listener is null)." << std::endl;
        if (errorCallback) {
            errorCallback(Nakama::NError("Networking not initialized", Nakama::ErrorCode::InternalError));
        }
        return;
    }

    auto successFn = [successCallback](Nakama::NMatchListPtr matchList) {
        if (successCallback) {
            if (matchList && !matchList->matches.empty()) {
                successCallback(matchList->matches);
            } else {
                successCallback({});
            }
        }
    };

    m_nakamaClientPtr->listMatches(
        m_sessionPtr,
        Nakama::opt::nullopt,                       // min_size
        Nakama::opt::nullopt,                       // max_size
        Nakama::opt::optional<int32_t>(20),         // limit
        Nakama::opt::nullopt,                       // label
        Nakama::opt::nullopt,                       // query
        Nakama::opt::optional<bool>(true),          // authoritative
        successFn,
        errorCallback);
}

void Networking::joinMatch(const std::string& matchId,
                           std::function<void(bool)> callback) {
    if (!m_nakamaClientPtr || !m_sessionPtr || !m_listener) {
        std::cerr << "JoinMatch Error: Networking not properly initialized "
                  << "(client, session, or listener is null)." << std::endl;
        if (callback) callback(false);
        return;
    }

    if (!m_rtClient) {
        if (!m_nakamaClientPtr) {
             std::cerr << "Networking: m_nakamaClientPtr is null, cannot create rtClient." << std::endl;
             if (callback) callback(false);
             return;
        }
        m_rtClient = m_nakamaClientPtr->createRtClient();
        if (m_rtClient) {
            if (!m_listener) {
                std::cerr << "Networking: m_listener is null, cannot set listener for rtClient." << std::endl;
                if (callback) callback(false);
                return;
            }
            m_rtClient->setListener(m_listener.get());
            std::cout << "Networking: Created new rtClient." << std::endl;
        } else {
            std::cerr << "Networking: Failed to create rtClient." << std::endl;
            if (callback) callback(false);
            return;
        }
    }

    if (!m_rtClient->isConnected()) {
        m_pendingMatchId = matchId;
        m_pendingJoinCallback = callback;
        m_rtClient->connect(m_sessionPtr, true);
    } else {
        m_rtClient->joinMatch(matchId, {},
            [this, matchId, callback](Nakama::NMatch match) {
                std::cout << "Successfully joined match: " << match.matchId << std::endl;
                m_currentMatchId = match.matchId;
                if (callback) callback(true);
            },
            [this, callback](const Nakama::NRtError& err) {
                std::cerr << "Failed to join match: " << err.message << std::endl;
                if (callback) callback(false);
            });
    }
}

void Networking::completePendingMatchJoin() {
    if (!m_pendingMatchId.empty() && m_pendingJoinCallback) {
        std::string matchIdToJoin = m_pendingMatchId;
        auto joinCallback = m_pendingJoinCallback;

        m_pendingMatchId.clear();
        m_pendingJoinCallback = nullptr;

        if (m_rtClient && m_rtClient->isConnected()) {
            m_rtClient->joinMatch(matchIdToJoin, {},
                [this, matchIdToJoin, joinCallback](Nakama::NMatch match) {
                    std::cout << "Successfully joined match via onConnect: " << match.matchId << std::endl;
                    m_currentMatchId = match.matchId;
                    if (joinCallback) joinCallback(true);
                },
                [this, joinCallback](const Nakama::NRtError& err) {
                    std::cerr << "Failed to join match via onConnect: " << err.message << std::endl;
                    if (joinCallback) joinCallback(false);
                });
        } else {
            std::cerr << "rtClient not connected or null when trying to complete pending match join." << std::endl;
            if (joinCallback) joinCallback(false);
        }
    }
}

void Networking::tick() {
    if (m_rtClient) {
        m_rtClient->tick();
    }
}

void Networking::sendPlayerUpdate(const sf::Vector2f& direction, float speed, unsigned int sequenceNumber) {
    if (!m_rtClient || !m_rtClient->isConnected() || m_currentMatchId.empty()) {
        std::cerr << "Networking::sendPlayerUpdate: Not connected to a match or rtClient is invalid." << std::endl;
        return;
    }
    if (!m_sessionPtr) {
        std::cerr << "Networking::sendPlayerUpdate: Session pointer is null." << std::endl;
        return;
    }

    int64_t opCode = 1;

    nlohmann::json payload;
    payload["playerId"] = m_sessionPtr->getUserId();
    payload["action"] = "move";
    payload["inputSequence"] = sequenceNumber;
    payload["velocityX"] = direction.x * speed;
    payload["velocityY"] = direction.y * speed;

    std::string data = payload.dump();

    Nakama::NBytes bytesData(data.begin(), data.end());

    m_rtClient->sendMatchData(m_currentMatchId, opCode, bytesData);
}

void Networking::setPlayerStateUpdateCallback(PlayerStateUpdateCallback callback) {
    m_onPlayerStateUpdateCallback = callback;
}

void Networking::setInputAckCallback(InputAckCallback callback) {
    m_onInputAckCallback = callback;
}

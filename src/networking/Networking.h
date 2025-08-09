#ifndef NETWORKING_H
#define NETWORKING_H

#include <vector>
#include <string>
#include <memory> // Required for std::shared_ptr
#include <functional> // Required for std::function

#include <nakama-cpp/Nakama.h> // General Nakama include
#include <nakama-cpp/realtime/NRtClientListenerInterface.h> // For the listener interface
#include <nakama-cpp/realtime/NRtClientDisconnectInfo.h> // For disconnect info
#include <nakama-cpp/realtime/rtdata/NMatchData.h> // Corrected path for NMatchData
#include <nakama-cpp/NError.h> // For NError (used by NRtError)

#include <SFML/System/Vector2.hpp> // Added for sf::Vector2f
#include <SFML/Graphics/Font.hpp> // Added for sf::Font

class NakamaClient; // Forward declaration

// Forward declare Networking class for the listener
class Networking;

class InternalRtListener : public Nakama::NRtClientListenerInterface {
public:
    InternalRtListener(Networking* networkingService);

    void onConnect() override;
    void onDisconnect(const Nakama::NRtClientDisconnectInfo& info) override;
    void onError(const Nakama::NRtError& error) override;
    void onMatchData(const Nakama::NMatchData& data) override;

    // Empty implementations for other *virtual* methods from NRtClientListenerInterface
    // Remove override for non-virtual or non-existent base methods
    void onChannelMessage(const Nakama::NChannelMessage& /*message*/) override {}
    // void onChannelPresenceEvent(const Nakama::NChannelPresenceEvent& /*event*/) override {} // Likely not virtual in base or name mismatch
    void onMatchmakerMatched(Nakama::NMatchmakerMatchedPtr /*matched*/) override {}
    // void onMatchPresenceEvent(const Nakama::NMatchPresenceEvent& /*event*/) override {} // Likely not virtual in base or name mismatch
    void onNotifications(const Nakama::NNotificationList& /*notifications*/) override {}
    // void onStatusPresenceEvent(const Nakama::NStatusPresenceEvent& /*event*/) override {} // Likely not virtual in base or name mismatch
    // void onStreamPresenceEvent(const Nakama::NStreamPresenceEvent& /*event*/) override {} // Likely not virtual in base or name mismatch
    void onStreamData(const Nakama::NStreamData& /*data*/) override {};

    // Add the missing pure virtual functions from NRtClientListenerInterface if any, or ensure all are covered.
    // Based on typical Nakama SDK structure, these might be the ones actually needing override:
    // (The ones above are common, but let's assume the compiler errors point to specific missing overrides)
    // If NChannelPresenceEvent, NMatchPresenceEvent etc. are indeed part of the interface and virtual,
    // they should be here. If not, they should be removed.
    // For now, commenting out the ones that caused specific "does not override" errors.

private:
    Networking* m_networkingService; // Pointer to Networking to call its methods or access members
};


class TestableNetworking; // Forward declaration for the friend class

class Networking {
public:
    friend class InternalRtListener; 
    friend class TestableNetworking; // Grant access to TestableNetworking

    Networking(Nakama::NClientPtr nakamaClientPtr); // SessionPtr removed
    ~Networking();

    // Initialize with a session, typically after login
    bool initialize(Nakama::NSessionPtr sessionPtr);

    void listMatches(
        std::function<void(const std::vector<Nakama::NMatch>&)> successCallback,
        std::function<void(const Nakama::NError&)> errorCallback
    );
    void joinMatch(
        const std::string& matchId,
        std::function<void(bool)> callback
    );
    void tick();

    // void listMatches(); // This was a duplicate or problematic overload, removed.
    // void joinMatch(const std::string& matchId); // This was a duplicate or problematic overload, removed.
    
    void setCurrentMatchId(const std::string& matchId);
    Nakama::NRtClientPtr getRtClient() { return m_rtClient; } 
    std::string getCurrentMatchId() const { return m_currentMatchId; }

    // Method for listener to call after RT client connects
    void completePendingMatchJoin(); 

    // Send player movement update
    void sendPlayerUpdate(const sf::Vector2f& direction, float speed, unsigned int sequenceNumber);

    // Callback for player state updates from the server
    using PlayerStateUpdateCallback = std::function<void(const std::string& playerId, const sf::Vector2f& position, unsigned int lastProcessedSequence)>;
    void setPlayerStateUpdateCallback(PlayerStateUpdateCallback callback);

    // Callback for input acknowledgments from the server
    using InputAckCallback = std::function<void(const std::string& playerId, unsigned int inputSequence, bool approved, const sf::Vector2f& serverPosition)>;
    void setInputAckCallback(InputAckCallback callback);

private:
    Nakama::NClientPtr m_nakamaClientPtr;
    Nakama::NSessionPtr m_sessionPtr;      
    Nakama::NRtClientPtr m_rtClient;
    std::shared_ptr<InternalRtListener> m_listener;
    std::string m_currentMatchId;

    // For handling match joining after RT client connection
    std::string m_pendingMatchId;
    std::function<void(bool)> m_pendingJoinCallback;

    PlayerStateUpdateCallback m_onPlayerStateUpdateCallback; // Callback instance
    InputAckCallback m_onInputAckCallback; // Callback instance for input ACKs

    void connect_rt_client(std::function<void()> onSuccess = nullptr, std::function<void(const Nakama::NRtError&)> onError = nullptr);
    void processPendingJoin();
};

#endif // NETWORKING_H

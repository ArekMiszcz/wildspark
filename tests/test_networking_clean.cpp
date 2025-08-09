#include <gtest/gtest.h>
#include "mocks/MockRtClient.h"
#include "MockNClientFull.h"
#include "nakama-cpp/NSessionInterface.h"
#include "nakama-cpp/NTypes.h"
#include "nakama-cpp/realtime/NRtClientListenerInterface.h"
#include "nakama-cpp/realtime/NRtClientInterface.h"
#include "nakama-cpp/realtime/rtdata/NParty.h"
#include "nakama-cpp/realtime/rtdata/NPartyClose.h"
#include "nakama-cpp/realtime/rtdata/NPartyData.h"
#include "nakama-cpp/realtime/rtdata/NPartyJoinRequest.h"
#include "nakama-cpp/realtime/rtdata/NPartyLeader.h"
#include "nakama-cpp/realtime/rtdata/NPartyMatchmakerTicket.h"
#include "nakama-cpp/realtime/rtdata/NPartyPresenceEvent.h"
#include "nakama-cpp/realtime/rtdata/NStatusPresenceEvent.h"
#include "nakama-cpp/realtime/rtdata/NStreamPresenceEvent.h"
#include "nakama-cpp/realtime/rtdata/NStreamData.h"
#include "nakama-cpp/realtime/NRtClientDisconnectInfo.h"
#include "nakama-cpp/realtime/rtdata/NRtError.h"
#include "nakama-cpp/NClientInterface.h"
#include "nakama-cpp/data/NMatchList.h"
#include "networking/Networking.h"
#include <SFML/System/Vector2.hpp>
#include <nlohmann/json.hpp>

// Mock implementation for NSessionInterface
class MockSession : public Nakama::NSessionInterface {
public:
    MOCK_METHOD(const std::string&, getAuthToken, (), (const, override));
    MOCK_METHOD(const std::string&, getRefreshToken, (), (const, override));
    MOCK_METHOD(const std::string&, getUserId, (), (const, override));
    MOCK_METHOD(const std::string&, getUsername, (), (const, override));
    MOCK_METHOD(bool, isExpired, (Nakama::NTimestamp now), (const, override));
    MOCK_METHOD(bool, isRefreshExpired, (Nakama::NTimestamp now), (const, override));
    MOCK_METHOD(const Nakama::NStringMap&, getVariables, (), (const, override));
    MOCK_METHOD(Nakama::NTimestamp, getCreateTime, (), (const, override));
    MOCK_METHOD(Nakama::NTimestamp, getExpireTime, (), (const, override));
    MOCK_METHOD(bool, isExpired, (), (const, override));
    MOCK_METHOD(bool, isRefreshExpired, (), (const, override));
    MOCK_METHOD(bool, isCreated, (), (const, override));
    MOCK_METHOD(std::string, getVariable, (const std::string& name), (const, override));
};

// Mock implementation for NRtClientListenerInterface
class MockRtClientListener : public Nakama::NRtClientListenerInterface {
public:
    MOCK_METHOD(void, onConnect, (), (override));
    MOCK_METHOD(void, onDisconnect, (const Nakama::NRtClientDisconnectInfo& info), (override));
    MOCK_METHOD(void, onError, (const Nakama::NRtError& error), (override));
    MOCK_METHOD(void, onChannelMessage, (const Nakama::NChannelMessage& message), (override));
    MOCK_METHOD(void, onChannelPresence, (const Nakama::NChannelPresenceEvent& presence), (override));
    MOCK_METHOD(void, onMatchmakerMatched, (Nakama::NMatchmakerMatchedPtr matched), (override));
    MOCK_METHOD(void, onMatchData, (const Nakama::NMatchData& matchData), (override));
    MOCK_METHOD(void, onMatchPresence, (const Nakama::NMatchPresenceEvent& matchPresence), (override));
    MOCK_METHOD(void, onNotifications, (const Nakama::NNotificationList& notifications), (override));
    MOCK_METHOD(void, onParty, (const Nakama::NParty& party), (override));
    MOCK_METHOD(void, onPartyClosed, (const Nakama::NPartyClose& partyCloseEvent), (override));
    MOCK_METHOD(void, onPartyData, (const Nakama::NPartyData& partyData), (override));
    MOCK_METHOD(void, onPartyJoinRequest, (const Nakama::NPartyJoinRequest& partyJoinRequest), (override));
    MOCK_METHOD(void, onPartyLeader, (const Nakama::NPartyLeader& partyLeader), (override));
    MOCK_METHOD(void, onPartyMatchmakerTicket, (const Nakama::NPartyMatchmakerTicket& ticket), (override));
    MOCK_METHOD(void, onPartyPresence, (const Nakama::NPartyPresenceEvent& presenceEvent), (override));
    MOCK_METHOD(void, onStatusPresence, (const Nakama::NStatusPresenceEvent& presence), (override));
    MOCK_METHOD(void, onStreamData, (const Nakama::NStreamData& data), (override));
    MOCK_METHOD(void, onStreamPresence, (const Nakama::NStreamPresenceEvent& presence), (override));
};

// Helper class to access protected/private members for testing
class TestableNetworking : public Networking {
public:
    TestableNetworking(Nakama::NClientPtr nakamaClientPtr) : Networking(nakamaClientPtr) {}

    // Expose m_listener for testing InternalRtListener behavior
    InternalRtListener* getInternalListener() {
        return m_listener.get();
    }

    // Expose m_rtClient for specific tests if absolutely necessary
    Nakama::NRtClientPtr getProtectedRtClient() {
        return m_rtClient;
    }
    void setProtectedRtClient(Nakama::NRtClientPtr client) {
        m_rtClient = client;
    }

    // Expose m_pendingMatchId for specific tests
    std::string getPendingMatchId() const {
        return m_pendingMatchId;
    }
    
    void callCompletePendingMatchJoin() {
        completePendingMatchJoin();
    }
    
    // Expose pending callback for testing
    std::function<void(bool)>& getPendingJoinCallback() {
        return m_pendingJoinCallback;
    }
    
    void setPendingJoinCallback(std::function<void(bool)> callback) {
        m_pendingJoinCallback = callback;
    }
    
    // Helper to set current match ID for testing
    void setCurrentMatchIdForTest(const std::string& matchId) {
        m_currentMatchId = matchId;
    }
};

TEST(NetworkingTest, InitializeNetworking) {
    auto mockNClient = std::make_shared<MockNClientFull>();
    auto mockSession = std::make_shared<MockSession>();
    std::string userIdStr = "test_user";

    Networking networking(mockNClient);

    EXPECT_CALL(*mockSession, getUserId()).WillRepeatedly(testing::ReturnRef(userIdStr));

    bool initialized = networking.initialize(mockSession);
    ASSERT_TRUE(initialized);
}

TEST(NetworkingTest, InitializeWithNullSession) {
    auto mockNClient = std::make_shared<MockNClientFull>();
    Networking networking(mockNClient);
    bool initialized = networking.initialize(nullptr);
    ASSERT_FALSE(initialized);
}

TEST(NetworkingTest, ListMatchesSuccess) {
    auto mockNClient = std::make_shared<MockNClientFull>();
    auto mockSession = std::make_shared<MockSession>();
    std::string userIdStr = "test_user";

    Networking networking(mockNClient);
    EXPECT_CALL(*mockSession, getUserId()).WillRepeatedly(testing::ReturnRef(userIdStr));
    networking.initialize(mockSession);

    auto matchList = std::make_shared<Nakama::NMatchList>();
    Nakama::NMatch match_item; 
    match_item.matchId = "match123";
    matchList->matches.push_back(match_item);

    EXPECT_CALL(*mockNClient, listMatches(
        testing::_, testing::_, testing::_, testing::_, testing::_, testing::_, testing::_, testing::_, testing::_
    )).WillOnce(testing::Invoke([matchList](
        Nakama::NSessionPtr /*session_arg*/,
        const Nakama::opt::optional<int32_t>& /*min_size_arg*/,
        const Nakama::opt::optional<int32_t>& /*max_size_arg*/,
        const Nakama::opt::optional<int32_t>& /*limit_arg*/,
        const Nakama::opt::optional<std::string>& /*label_arg*/,
        const Nakama::opt::optional<std::string>& /*query_arg*/,
        const Nakama::opt::optional<bool>& /*authoritative_arg*/,
        std::function<void(Nakama::NMatchListPtr)> successCallback_arg,
        Nakama::ErrorCallback /*errorCallback_arg*/
    ) {
        successCallback_arg(matchList);
    }));

    bool callbackCalled = false;
    networking.listMatches(
        [&callbackCalled, &match_item](const std::vector<Nakama::NMatch>& matches_vec) { 
            callbackCalled = true;
            ASSERT_FALSE(matches_vec.empty());
            ASSERT_EQ(matches_vec[0].matchId, "match123");
        },
        [](const Nakama::NError& /*error*/) { 
            FAIL() << "Error callback should not have been called.";
        }
    );

    ASSERT_TRUE(callbackCalled);
}

TEST(NetworkingTest, ListMatchesError) {
    auto mockNClient = std::make_shared<MockNClientFull>();
    auto mockSession = std::make_shared<MockSession>();
    std::string userIdStr = "test_user";

    Networking networking(mockNClient);
    EXPECT_CALL(*mockSession, getUserId()).WillRepeatedly(testing::ReturnRef(userIdStr));
    networking.initialize(mockSession);

    EXPECT_CALL(*mockNClient, listMatches(
        testing::_, testing::_, testing::_, testing::_, testing::_, testing::_, testing::_, testing::_, testing::_
    )).WillOnce(testing::Invoke([](
        Nakama::NSessionPtr /*session*/,
        const Nakama::opt::optional<int32_t>& /*min_size*/,
        const Nakama::opt::optional<int32_t>& /*max_size*/,
        const Nakama::opt::optional<int32_t>& /*limit*/,
        const Nakama::opt::optional<std::string>& /*label*/,
        const Nakama::opt::optional<std::string>& /*query*/,
        const Nakama::opt::optional<bool>& /*authoritative*/,
        std::function<void(Nakama::NMatchListPtr)> /*successCallback*/,
        Nakama::ErrorCallback errorCallback
    ) {
        errorCallback(Nakama::NError("Simulated error", Nakama::ErrorCode::Unknown));
    }));

    bool errorCallbackCalled = false;
    networking.listMatches(
        [](const std::vector<Nakama::NMatch>& /*matches*/) { 
            FAIL() << "Success callback should not have been called.";
        },
        [&errorCallbackCalled](const Nakama::NError& error) { 
            errorCallbackCalled = true;
            ASSERT_EQ(error.message, "Simulated error");
        }
    );

    ASSERT_TRUE(errorCallbackCalled);
}

TEST(NetworkingTest, JoinMatchWhenNotConnected) {
    auto mockNClient = std::make_shared<MockNClientFull>();
    auto mockSession = std::make_shared<MockSession>();
    auto mockRtClient = std::make_shared<MockRtClient>();
    std::string userIdStr = "test_user";
    std::string matchId = "test_match_id";

    EXPECT_CALL(*mockSession, getUserId()).WillRepeatedly(testing::ReturnRef(userIdStr));
    EXPECT_CALL(*mockNClient, createRtClient()).WillOnce(testing::Return(mockRtClient));
    
    // Expect connect to be called on the rtClient
    EXPECT_CALL(*mockRtClient, connect(testing::_, testing::Eq(true), testing::Eq(Nakama::NRtClientProtocol::Json))).Times(1);
    // joinMatch should not be called directly yet, as it's pending connection
    EXPECT_CALL(*mockRtClient, joinMatch(testing::_, testing::_, testing::_, testing::_)).Times(0);

    TestableNetworking networking(mockNClient);
    networking.initialize(mockSession);
    
    bool callbackCalled = false;
    networking.joinMatch(matchId, [&callbackCalled](bool success) {
        callbackCalled = true; 
        ASSERT_TRUE(success);
    });

    // At this point, connect should have been called, and matchId stored.
    ASSERT_FALSE(callbackCalled); 
    ASSERT_EQ(networking.getPendingMatchId(), matchId);

    // Simulate the onConnect event which then triggers the actual join
    InternalRtListener* listener = networking.getInternalListener();
    ASSERT_NE(listener, nullptr);

    // Mock the joinMatch call that happens inside completePendingMatchJoin
    Nakama::NMatch joinedMatchData;
    joinedMatchData.matchId = matchId;

    EXPECT_CALL(*mockRtClient, isConnected()).WillRepeatedly(testing::Return(true));
    EXPECT_CALL(*mockRtClient, joinMatch(matchId, testing::_, testing::_, testing::_))
        .WillOnce(testing::Invoke([&joinedMatchData](
            const std::string& /*matchId_arg*/,
            const Nakama::NStringMap& /*metadata_arg*/,
            std::function<void(const Nakama::NMatch&)> successCb,
            Nakama::RtErrorCallback /*errorCb_arg*/
        ) {
            successCb(joinedMatchData);
        }));
    
    listener->onConnect();

    ASSERT_TRUE(callbackCalled);
    ASSERT_EQ(networking.getCurrentMatchId(), matchId);
}

TEST(NetworkingTest, TickCallsRtClientTick) {
    auto mockNClient = std::make_shared<MockNClientFull>();
    auto mockSession = std::make_shared<MockSession>();
    auto mockRtClient = std::make_shared<MockRtClient>();

    TestableNetworking networking(mockNClient);
    networking.initialize(mockSession);
    networking.setProtectedRtClient(mockRtClient);

    EXPECT_CALL(*mockRtClient, tick()).Times(1);
    networking.tick();

    // Test with no rtClient
    networking.setProtectedRtClient(nullptr);
    EXPECT_CALL(*mockRtClient, tick()).Times(0);
    networking.tick(); 
}

TEST(NetworkingTest, SendPlayerUpdate) {
    auto mockNClient = std::make_shared<MockNClientFull>();
    auto mockSession = std::make_shared<MockSession>();
    auto mockRtClient = std::make_shared<MockRtClient>();

    TestableNetworking networking(mockNClient);
    networking.initialize(mockSession);
    networking.setProtectedRtClient(mockRtClient);
    
    std::string userId = "player123";
    std::string matchId = "match_for_updates";
    networking.setCurrentMatchIdForTest(matchId);

    EXPECT_CALL(*mockSession, getUserId()).WillRepeatedly(testing::ReturnRef(userId));
    EXPECT_CALL(*mockRtClient, isConnected()).WillOnce(testing::Return(true));
    
    sf::Vector2f direction(1.0f, 0.5f);
    float speed = 100.0f;
    unsigned int sequenceNumber = 1;

    EXPECT_CALL(*mockRtClient, sendMatchData(matchId, 1, testing::_, testing::_)).Times(1);

    networking.sendPlayerUpdate(direction, speed, sequenceNumber);
}

TEST(NetworkingTest, SendPlayerUpdateNotConnected) {
    auto mockNClient = std::make_shared<MockNClientFull>();
    auto mockSession = std::make_shared<MockSession>();
    auto mockRtClient = std::make_shared<MockRtClient>();

    TestableNetworking networking(mockNClient);
    networking.initialize(mockSession);
    networking.setProtectedRtClient(mockRtClient);
    networking.setCurrentMatchIdForTest("some_match");

    EXPECT_CALL(*mockRtClient, isConnected()).WillOnce(testing::Return(false));
    EXPECT_CALL(*mockRtClient, sendMatchData(testing::_, testing::_, testing::_, testing::_)).Times(0);

    networking.sendPlayerUpdate(sf::Vector2f(1,0), 10, 1);
}

TEST(NetworkingTest, SendPlayerUpdateNoMatchId) {
    auto mockNClient = std::make_shared<MockNClientFull>();
    auto mockSession = std::make_shared<MockSession>();
    auto mockRtClient = std::make_shared<MockRtClient>();

    TestableNetworking networking(mockNClient);
    networking.initialize(mockSession);
    networking.setProtectedRtClient(mockRtClient);

    EXPECT_CALL(*mockRtClient, isConnected()).WillOnce(testing::Return(true));
    EXPECT_CALL(*mockRtClient, sendMatchData(testing::_, testing::_, testing::_, testing::_)).Times(0);

    networking.sendPlayerUpdate(sf::Vector2f(1,0), 10, 1);
}

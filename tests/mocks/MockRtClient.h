// Copyright 2025 WildSpark Authors

#pragma once
#include <gmock/gmock.h>

#include <string>
#include <vector>

#include "nakama-cpp/NSessionInterface.h"
#include "nakama-cpp/NTypes.h"
#include "nakama-cpp/realtime/NRtClientInterface.h"
#include "nakama-cpp/realtime/NRtClientListenerInterface.h"

class MockRtClient : public Nakama::NRtClientInterface {
 public:
  MockRtClient() = default;
  ~MockRtClient() override = default;

  MOCK_METHOD(void, tick, ());
  MOCK_METHOD(Nakama::NRtTransportPtr, getTransport, (), (const));
  MOCK_METHOD(void, setListener,
              (Nakama::NRtClientListenerInterface * listener));
  MOCK_METHOD(void, setUserData, (void* userData));
  MOCK_METHOD(void*, getUserData, (), (const));
  MOCK_METHOD(void, setHeartbeatIntervalMs, (Nakama::opt::optional<int> ms));
  MOCK_METHOD(Nakama::opt::optional<int>, getHeartbeatIntervalMs, ());
  MOCK_METHOD(void, connect,
              (Nakama::NSessionPtr session, bool createStatus,
               Nakama::NRtClientProtocol protocol));
  MOCK_METHOD(std::future<void>, connectAsync,
              (Nakama::NSessionPtr session, bool createStatus,
               Nakama::NRtClientProtocol protocol));
  MOCK_METHOD(bool, isConnecting, (), (const));
  MOCK_METHOD(bool, isConnected, (), (const));
  MOCK_METHOD(void, disconnect, ());
  MOCK_METHOD(std::future<void>, disconnectAsync, ());

  MOCK_METHOD(void, joinChat,
              (const std::string& target, Nakama::NChannelType type,
               const Nakama::opt::optional<bool>& persistence,
               const Nakama::opt::optional<bool>& hidden,
               std::function<void(Nakama::NChannelPtr)> successCallback,
               Nakama::RtErrorCallback errorCallback));

  MOCK_METHOD(void, leaveChat,
              (const std::string& channelId,
               std::function<void()> successCallback,
               Nakama::RtErrorCallback errorCallback));

  MOCK_METHOD(
      void, writeChatMessage,
      (const std::string& channelId, const std::string& content,
       std::function<void(const Nakama::NChannelMessageAck&)> successCallback,
       Nakama::RtErrorCallback errorCallback));

  MOCK_METHOD(
      void, updateChatMessage,
      (const std::string& channelId, const std::string& messageId,
       const std::string& content,
       std::function<void(const Nakama::NChannelMessageAck&)> successCallback,
       Nakama::RtErrorCallback errorCallback));

  MOCK_METHOD(
      void, removeChatMessage,
      (const std::string& channelId, const std::string& messageId,
       std::function<void(const Nakama::NChannelMessageAck&)> successCallback,
       Nakama::RtErrorCallback errorCallback));

  MOCK_METHOD(void, createMatch,
              (std::function<void(const Nakama::NMatch&)> successCallback,
               Nakama::RtErrorCallback errorCallback));

  MOCK_METHOD(void, joinMatch,
              (const std::string& matchId, const Nakama::NStringMap& metadata,
               std::function<void(const Nakama::NMatch&)> successCallback,
               Nakama::RtErrorCallback errorCallback));

  MOCK_METHOD(void, joinMatchByToken,
              (const std::string& token,
               std::function<void(const Nakama::NMatch&)> successCallback,
               Nakama::RtErrorCallback errorCallback));

  MOCK_METHOD(void, leaveMatch,
              (const std::string& matchId,
               std::function<void()> successCallback,
               Nakama::RtErrorCallback errorCallback));

  MOCK_METHOD(
      void, addMatchmaker,
      (const Nakama::opt::optional<int32_t>& minCount,
       const Nakama::opt::optional<int32_t>& maxCount,
       const Nakama::opt::optional<std::string>& query,
       const Nakama::NStringMap& stringProperties,
       const Nakama::NStringDoubleMap& numericProperties,
       const Nakama::opt::optional<int32_t>& countMultiple,
       std::function<void(const Nakama::NMatchmakerTicket&)> successCallback,
       Nakama::RtErrorCallback errorCallback));

  MOCK_METHOD(void, removeMatchmaker,
              (const std::string& ticket, std::function<void()> successCallback,
               Nakama::RtErrorCallback errorCallback));

  MOCK_METHOD(void, sendMatchData,
              (const std::string& matchId, int64_t opCode,
               const Nakama::NBytes& data,
               const std::vector<Nakama::NUserPresence>& presences));

  MOCK_METHOD(void, followUsers,
              (const std::vector<std::string>& userIds,
               std::function<void(const Nakama::NStatus&)> successCallback,
               Nakama::RtErrorCallback errorCallback));

  MOCK_METHOD(void, unfollowUsers,
              (const std::vector<std::string>& userIds,
               std::function<void()> successCallback,
               Nakama::RtErrorCallback errorCallback));

  MOCK_METHOD(void, updateStatus,
              (const std::string& status, std::function<void()> successCallback,
               Nakama::RtErrorCallback errorCallback));

  MOCK_METHOD(void, rpc,
              (const std::string& id,
               const Nakama::opt::optional<std::string>& payload,
               std::function<void(const Nakama::NRpc&)> successCallback,
               Nakama::RtErrorCallback errorCallback));

  MOCK_METHOD(void, acceptPartyMember,
              (const std::string& partyId, Nakama::NUserPresence& presence,
               std::function<void()> successCallback,
               Nakama::RtErrorCallback errorCallback));

  MOCK_METHOD(void, addMatchmakerParty,
              (const std::string& partyId, const std::string& query,
               int32_t minCount, int32_t maxCount,
               const Nakama::NStringMap& stringProperties,
               const Nakama::NStringDoubleMap& numericProperties,
               const Nakama::opt::optional<int32_t>& countMultiple,
               std::function<void(const Nakama::NPartyMatchmakerTicket&)>
                   successCallback,
               Nakama::RtErrorCallback errorCallback));

  MOCK_METHOD(void, closeParty,
              (const std::string& partyId,
               std::function<void()> successCallback,
               Nakama::RtErrorCallback errorCallback));

  MOCK_METHOD(void, createParty,
              (bool open, int maxSize,
               std::function<void(const Nakama::NParty&)> successCallback,
               Nakama::RtErrorCallback errorCallback));

  MOCK_METHOD(void, joinParty,
              (const std::string& partyId,
               std::function<void()> successCallback,
               Nakama::RtErrorCallback errorCallback));

  MOCK_METHOD(void, leaveParty,
              (const std::string& partyId,
               std::function<void()> successCallback,
               Nakama::RtErrorCallback errorCallback));

  MOCK_METHOD(
      void, listPartyJoinRequests,
      (const std::string& partyId,
       std::function<void(const Nakama::NPartyJoinRequest&)> successCallback,
       Nakama::RtErrorCallback errorCallback));

  MOCK_METHOD(void, promotePartyMember,
              (const std::string& partyId, Nakama::NUserPresence& partyMember,
               std::function<void()> successCallback,
               Nakama::RtErrorCallback errorCallback));

  MOCK_METHOD(void, removeMatchmakerParty,
              (const std::string& partyId, const std::string& ticket,
               std::function<void()> successCallback,
               Nakama::RtErrorCallback errorCallback));

  MOCK_METHOD(void, removePartyMember,
              (const std::string& partyId, Nakama::NUserPresence& presence,
               std::function<void()> successCallback,
               Nakama::RtErrorCallback errorCallback));

  MOCK_METHOD(void, sendPartyData,
              (const std::string& partyId, int64_t opCode,
               Nakama::NBytes& data));

  MOCK_METHOD(std::future<Nakama::NChannelPtr>, joinChatAsync,
              (const std::string& target, Nakama::NChannelType type,
               const Nakama::opt::optional<bool>& persistence,
               const Nakama::opt::optional<bool>& hidden));

  MOCK_METHOD(std::future<void>, leaveChatAsync,
              (const std::string& channelId));

  MOCK_METHOD(std::future<Nakama::NChannelMessageAck>, writeChatMessageAsync,
              (const std::string& channelId, const std::string& content));

  MOCK_METHOD(std::future<Nakama::NChannelMessageAck>, updateChatMessageAsync,
              (const std::string& channelId, const std::string& messageId,
               const std::string& content));

  MOCK_METHOD(std::future<void>, removeChatMessageAsync,
              (const std::string& channelId, const std::string& messageId));

  MOCK_METHOD(std::future<Nakama::NMatch>, createMatchAsync, ());

  MOCK_METHOD(std::future<Nakama::NMatch>, joinMatchAsync,
              (const std::string& matchId, const Nakama::NStringMap& metadata));

  MOCK_METHOD(std::future<Nakama::NMatch>, joinMatchByTokenAsync,
              (const std::string& token));

  MOCK_METHOD(std::future<void>, leaveMatchAsync, (const std::string& matchId));

  MOCK_METHOD(std::future<Nakama::NMatchmakerTicket>, addMatchmakerAsync,
              (const Nakama::opt::optional<int32_t>& minCount,
               const Nakama::opt::optional<int32_t>& maxCount,
               const Nakama::opt::optional<std::string>& query,
               const Nakama::NStringMap& stringProperties,
               const Nakama::NStringDoubleMap& numericProperties,
               const Nakama::opt::optional<int32_t>& countMultiple));

  MOCK_METHOD(std::future<void>, removeMatchmakerAsync,
              (const std::string& ticket));

  MOCK_METHOD(std::future<void>, sendMatchDataAsync,
              (const std::string& matchId, std::int64_t opCode,
               const Nakama::NBytes& data,
               const std::vector<Nakama::NUserPresence>& presences));

  MOCK_METHOD(std::future<Nakama::NStatus>, followUsersAsync,
              (const std::vector<std::string>& userIds));

  MOCK_METHOD(std::future<void>, unfollowUsersAsync,
              (const std::vector<std::string>& userIds));

  MOCK_METHOD(std::future<void>, updateStatusAsync,
              (const std::string& status));

  MOCK_METHOD(std::future<Nakama::NRpc>, rpcAsync,
              (const std::string& id,
               const Nakama::opt::optional<std::string>& payload));

  MOCK_METHOD(std::future<void>, acceptPartyMemberAsync,
              (const std::string& partyId, Nakama::NUserPresence& presence));

  MOCK_METHOD(std::future<Nakama::NPartyMatchmakerTicket>,
              addMatchmakerPartyAsync,
              (const std::string& partyId, const std::string& query,
               int32_t minCount, int32_t maxCount,
               const Nakama::NStringMap& stringProperties,
               const Nakama::NStringDoubleMap& numericProperties,
               const Nakama::opt::optional<int32_t>& countMultiple));

  MOCK_METHOD(std::future<void>, closePartyAsync, (const std::string& partyId));

  MOCK_METHOD(std::future<Nakama::NParty>, createPartyAsync,
              (bool open, int maxSize));

  MOCK_METHOD(std::future<void>, joinPartyAsync, (const std::string& partyId));

  MOCK_METHOD(std::future<void>, leavePartyAsync, (const std::string& partyId));

  MOCK_METHOD(std::future<Nakama::NPartyJoinRequest>,
              listPartyJoinRequestsAsync, (const std::string& partyId));

  MOCK_METHOD(std::future<void>, promotePartyMemberAsync,
              (const std::string& partyId, Nakama::NUserPresence& partyMember));

  MOCK_METHOD(std::future<void>, removeMatchmakerPartyAsync,
              (const std::string& partyId, const std::string& ticket));

  MOCK_METHOD(std::future<void>, removePartyMemberAsync,
              (const std::string& partyId, Nakama::NUserPresence& presence));

  MOCK_METHOD(std::future<void>, sendPartyDataAsync,
              (const std::string& partyId, int64_t opCode,
               Nakama::NBytes& data));
};

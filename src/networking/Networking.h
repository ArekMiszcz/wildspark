// Copyright 2025 WildSpark Authors

#ifndef NETWORKING_NETWORKING_H_
#define NETWORKING_NETWORKING_H_

#include <nakama-cpp/NError.h>
#include <nakama-cpp/Nakama.h>
#include <nakama-cpp/realtime/NRtClientDisconnectInfo.h>
#include <nakama-cpp/realtime/NRtClientListenerInterface.h>
#include <nakama-cpp/realtime/rtdata/NMatchData.h>

#include <functional>
#include <memory>
#include <string>
#include <vector>

#include <SFML/Graphics/Font.hpp>
#include <SFML/System/Vector2.hpp>

class NakamaClient;  // Forward declaration
class Networking;     // Forward declaration for listener reference

class InternalRtListener : public Nakama::NRtClientListenerInterface {
 public:
  explicit InternalRtListener(Networking* networkingService);

  void onConnect() override;
  void onDisconnect(const Nakama::NRtClientDisconnectInfo& info) override;
  void onError(const Nakama::NRtError& error) override;
  void onMatchData(const Nakama::NMatchData& data) override;

  void onChannelMessage(const Nakama::NChannelMessage& /*message*/) override {}
  void onMatchmakerMatched(Nakama::NMatchmakerMatchedPtr /*matched*/) override {}
  void onNotifications(const Nakama::NNotificationList& /*notifications*/) override {}
  void onStreamData(const Nakama::NStreamData& /*data*/) override {}

 private:
  Networking* m_networkingService;
};

class TestableNetworking;  // Forward declaration

class Networking {
 public:
  friend class InternalRtListener;
  friend class TestableNetworking;

  explicit Networking(Nakama::NClientPtr nakamaClientPtr);
  ~Networking();

  bool initialize(Nakama::NSessionPtr sessionPtr);

  void listMatches(std::function<void(const std::vector<Nakama::NMatch>&)> successCallback,
                   std::function<void(const Nakama::NError&)> errorCallback);
  void joinMatch(const std::string& matchId, std::function<void(bool)> callback);
  void tick();

  void setCurrentMatchId(const std::string& matchId);
  Nakama::NRtClientPtr getRtClient() { return m_rtClient; }
  std::string getCurrentMatchId() const { return m_currentMatchId; }

  void completePendingMatchJoin();

  void sendPlayerUpdate(const sf::Vector2f& direction, float speed, unsigned int sequenceNumber);
  void sendPlayerAction(const int objectId, const std::string& action, unsigned int sequenceNumber);

  using PlayerStateUpdateCallback = std::function<
      void(const std::string& playerId, const sf::Vector2f& position, unsigned int lastProcessedSequence)>;
  void setPlayerStateUpdateCallback(PlayerStateUpdateCallback callback);

  using InputAckCallback = std::function<
      void(const std::string& playerId, unsigned int inputSequence, bool approved, const sf::Vector2f& serverPosition)>;
  void setInputAckCallback(InputAckCallback callback);

 private:
  Nakama::NClientPtr m_nakamaClientPtr;
  Nakama::NSessionPtr m_sessionPtr;
  Nakama::NRtClientPtr m_rtClient;
  std::shared_ptr<InternalRtListener> m_listener;
  std::string m_currentMatchId;

  std::string m_pendingMatchId;
  std::function<void(bool)> m_pendingJoinCallback;

  PlayerStateUpdateCallback m_onPlayerStateUpdateCallback;
  InputAckCallback m_onInputAckCallback;

  void connect_rt_client(std::function<void()> onSuccess = nullptr,
                         std::function<void(const Nakama::NRtError&)> onError = nullptr);
  void processPendingJoin();
};

#endif  // NETWORKING_NETWORKING_H_

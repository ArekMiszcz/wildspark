// Copyright 2025 WildSpark Authors

#pragma once

#include <nakama-cpp/Nakama.h>

#include <string>

#include "../AuthClient.h"

class NakamaClient : public AuthClient {
 public:
  NakamaClient();
  ~NakamaClient() override;

  Nakama::NClientPtr client;
  Nakama::NSessionPtr session;
  Nakama::NRtClientPtr rtClient;
  Nakama::NRtClientPtr getRtClient();

  std::string getSessionToken() override;
  void connect(const std::string& email, const std::string& password,
               LoginResultCallback callback) override;
  void disconnect() override;
  void tick();

 private:
  Nakama::NClientParameters parameters;
  bool _isRunning = false;
};

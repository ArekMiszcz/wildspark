// Copyright 2025 WildSpark Authors

#pragma once

#include <nakama-cpp/NTypes.h>
#include <gmock/gmock.h>

#include <functional>
#include <string>

#include "account/AccountManager.h"

class AuthManager;

class MockAccountManager : public AccountManager {
 public:
  explicit MockAccountManager(AuthManager& authManager)
      : AccountManager(authManager) {}

  MOCK_METHOD(
      void, listCharacters,
      (std::function<void(Nakama::NStorageObjectListPtr)> successCallback,
       std::function<void(const Nakama::NError&)> errorCallback),
      (override));
  MOCK_METHOD(
      void, saveCharacter,
      (const std::string& name, const std::string& sex,
       std::function<void(const Nakama::NStorageObjectAcks&)> successCallback,
       std::function<void(const Nakama::NError&)> errorCallback),
      (override));
};

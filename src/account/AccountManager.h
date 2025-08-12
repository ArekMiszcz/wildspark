// Copyright 2025 WildSpark Authors

#pragma once

#include <nakama-cpp/Nakama.h>
#include <nakama-cpp/NTypes.h>
#include <nakama-cpp/NClientInterface.h>
#include <functional>
#include <string>

class AuthManager;

class AccountManager {
 public:
  explicit AccountManager(AuthManager& authManager);
  virtual ~AccountManager();

  virtual void listCharacters(
      std::function<void(Nakama::NStorageObjectListPtr)> successCallback,
      std::function<void(const Nakama::NError&)> errorCallback);

  virtual void saveCharacter(
      const std::string& name,
      const std::string& sex,
      std::function<void(const Nakama::NStorageObjectAcks&)> successCallback,
      std::function<void(const Nakama::NError&)> errorCallback);

 private:
  AuthManager& authManagerRef;
  const std::string characterCollection = "characters";
};

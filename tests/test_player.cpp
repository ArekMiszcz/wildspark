// Copyright 2025 WildSpark Authors

#include <gtest/gtest.h>

#include "SFML/System/Time.hpp"
#include "SFML/System/Vector2.hpp"
#include "world/entities/Player.h"

class PlayerTest : public ::testing::Test {
 protected:
  Player player;

  PlayerTest() : player("test_player_id", sf::Color::Blue, true) {}

  void SetUp() override {
    player.setPosition({100.f, 100.f});
  }
};

TEST_F(PlayerTest, InitialPositionAndId) {
  ASSERT_EQ(player.getId(), "test_player_id");
  ASSERT_EQ(player.getPosition().x, 100.f);
  ASSERT_EQ(player.getPosition().y, 100.f);
}

TEST_F(PlayerTest, SetTargetDirectionAndMove) {
  player.setTargetDirection({1.f, 0.f});
  ASSERT_EQ(player.getDirection().x, 1.f);
  ASSERT_EQ(player.getDirection().y, 0.f);
  player.update(sf::seconds(1.0f));
  EXPECT_NEAR(player.getPosition().x, 100.f + player.getSpeed(), 0.001f);
  EXPECT_NEAR(player.getPosition().y, 100.f, 0.001f);
}

TEST_F(PlayerTest, StopMovement) {
  player.setTargetDirection({1.f, 0.f});
  player.update(sf::seconds(1.0f));
  ASSERT_NE(player.getPosition().x, 100.f);
  player.setTargetDirection({0.f, 0.f});
  ASSERT_EQ(player.getDirection().x, 0.f);
  ASSERT_EQ(player.getDirection().y, 0.f);
  sf::Vector2f before = player.getPosition();
  player.update(sf::seconds(1.0f));
  EXPECT_EQ(player.getPosition().x, before.x);
  EXPECT_EQ(player.getPosition().y, before.y);
}

TEST_F(PlayerTest, HandleServerUpdate) {
  unsigned int initialSequence = player.getNextSequenceNumber();
  player.handleServerUpdate({200.f, 200.f}, initialSequence - 1);
  EXPECT_EQ(player.getPosition().x, 100.f);
  EXPECT_EQ(player.getPosition().y, 100.f);
}

TEST_F(PlayerTest, HandleServerAckApproved) {
  player.setTargetDirection({0.f, 1.f});
  player.update(sf::seconds(0.1f));
  unsigned int seq = player.getNextSequenceNumber() - 1;
  sf::Vector2f serverPos =
      player.getPosition() + sf::Vector2f(0.f, player.getSpeed() * 0.1f);
  player.handleServerAck(seq, true, serverPos);
  EXPECT_EQ(player.getPosition().x, serverPos.x);
  EXPECT_EQ(player.getPosition().y, serverPos.y);
}

TEST_F(PlayerTest, HandleServerAckNotApproved) {
  player.setTargetDirection({1.f, 0.f});
  player.update(sf::seconds(0.1f));
  unsigned int seq = player.getNextSequenceNumber() - 1;
  player.handleServerAck(seq, false, {90.f, 90.f});
  EXPECT_EQ(player.getPosition().x, 90.f);
  EXPECT_EQ(player.getPosition().y, 90.f);
}

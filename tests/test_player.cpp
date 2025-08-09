#include <gtest/gtest.h>
#include "world/entities/Player.h"
#include "SFML/System/Time.hpp"
#include "SFML/System/Vector2.hpp"

// Test fixture for Player tests
class PlayerTest : public ::testing::Test {
protected:
    Player player;

    PlayerTest() : player("test_player_id", sf::Color::Blue) {}

    void SetUp() override {
        // Common setup for each test
        player.setPosition({100.f, 100.f});
        // player.setSpeed(50.f); // Example speed - REMOVED as Player has no setSpeed
    }
};

TEST_F(PlayerTest, InitialPositionAndId) {
    ASSERT_EQ(player.getId(), "test_player_id");
    ASSERT_EQ(player.getPosition().x, 100.f);
    ASSERT_EQ(player.getPosition().y, 100.f);
}

TEST_F(PlayerTest, SetTargetDirectionAndMove) {
    player.setTargetDirection({1.f, 0.f}); // Move right
    ASSERT_EQ(player.getDirection().x, 1.f);
    ASSERT_EQ(player.getDirection().y, 0.f);

    player.update(sf::seconds(1.0f)); // Update for 1 second
    // Expected position: initial_pos + direction * speed * time
    // (100, 100) + (1, 0) * 50 * 1 = (150, 100)
    EXPECT_NEAR(player.getPosition().x, 150.f, 0.001f);
    EXPECT_NEAR(player.getPosition().y, 100.f, 0.001f);
}

TEST_F(PlayerTest, StopMovement) {
    player.setTargetDirection({1.f, 0.f});
    player.update(sf::seconds(1.0f));
    ASSERT_NE(player.getPosition().x, 100.f); // Ensure it moved

    player.setTargetDirection({0.f, 0.f}); // Stop
    ASSERT_EQ(player.getDirection().x, 0.f);
    ASSERT_EQ(player.getDirection().y, 0.f);

    sf::Vector2f positionBeforeStop = player.getPosition();
    player.update(sf::seconds(1.0f)); // Update again
    // Position should not change after stopping
    EXPECT_EQ(player.getPosition().x, positionBeforeStop.x);
    EXPECT_EQ(player.getPosition().y, positionBeforeStop.y);
}

TEST_F(PlayerTest, HandleServerUpdate) {
    unsigned int initialSequence = player.getNextSequenceNumber();
    player.handleServerUpdate({200.f, 200.f}, initialSequence -1 ); // Server confirms a previous state
    
    // Position should be updated directly by server state
    EXPECT_EQ(player.getPosition().x, 200.f);
    EXPECT_EQ(player.getPosition().y, 200.f);
    // Check if pending inputs are reconciled (this might need more detailed testing based on Player's reconciliation logic)
}

TEST_F(PlayerTest, HandleServerAckApproved) {
    // Simulate player intending to move and an input being sent
    player.setTargetDirection({0.f, 1.f}); // e.g., moving down
    player.update(sf::seconds(0.1f)); // Simulate a small time tick, which might internally queue an input
    unsigned int seq = player.getNextSequenceNumber() -1; // Get the sequence number of the input just processed/sent
    // player.addPendingInput({0.f, 1.f}, seq); // Simulate sending an input - REMOVED

    sf::Vector2f serverConfirmedPosition = player.getPosition() + sf::Vector2f(0.f, player.getSpeed() * 0.1f); // Approximate expected server position

    player.handleServerAck(seq, true, serverConfirmedPosition); // Server approves and provides its position

    // Player position should be updated to server's authoritative position
    EXPECT_EQ(player.getPosition().x, serverConfirmedPosition.x);
    EXPECT_EQ(player.getPosition().y, serverConfirmedPosition.y);
    // Test that the acknowledged input is removed from pending inputs (requires Player to expose this or test via side effects)
}

TEST_F(PlayerTest, HandleServerAckNotApproved) {
    // Simulate player intending to move
    player.setTargetDirection({1.f, 0.f});
    player.update(sf::seconds(0.1f)); // Simulate a small time tick
    unsigned int seq = player.getNextSequenceNumber() -1; // Get the sequence number
    // player.addPendingInput({1.f, 0.f}, seq); - REMOVED
    sf::Vector2f originalPosition = player.getPosition();

    player.handleServerAck(seq, false, {90.f, 90.f}); // Server rejects, provides corrected position

    // Player position should be corrected to server's position
    EXPECT_EQ(player.getPosition().x, 90.f);
    EXPECT_EQ(player.getPosition().y, 90.f);
    // Test that the rejected input is removed and potentially player state is reconciled
}

// Add more tests for:
// - Sequence number management (getNextSequenceNumber, incrementing correctly)
// - Client-side prediction and reconciliation logic (if Player.cpp implements it fully)
// - Edge cases for movement, updates, and acknowledgments

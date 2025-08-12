// Copyright 2025 WildSpark Authors

#ifndef WORLD_ENTITIES_PLAYER_H_
#define WORLD_ENTITIES_PLAYER_H_

#include <string>
#include <vector>

#include <SFML/Graphics.hpp>
#include <SFML/Graphics/Font.hpp>
#include <SFML/Graphics/Text.hpp>

class Player {
 public:
  explicit Player(const std::string& id = "local_player", sf::Color color = sf::Color::Green);

  void setId(const std::string& id);
  std::string getId() const;

  void setDirection(const sf::Vector2f& direction);  // Kept for direct setting if needed
  sf::Vector2f getDirection() const;  // This should return m_targetDirection
  float getSpeed() const { return m_speed; }

  void setTargetDirection(const sf::Vector2f& direction);

  // New method to handle input for player movement
  bool handleInput(const sf::Event& event, sf::RenderWindow& window, const sf::View& view);

  void handleServerUpdate(const sf::Vector2f& serverPosition, unsigned int lastProcessedSequenceNumber);
  void handleServerAck(unsigned int inputSequence, bool approved, const sf::Vector2f& serverPosition);
  void update(sf::Time deltaTime);  // For client-side interpolation/prediction if needed
  void render(sf::RenderTarget& target);

  void setPosition(const sf::Vector2f& position);

  sf::Vector2f getPosition() const { return m_position; }

  unsigned int getNextSequenceNumber();  // Added
  static void setLocalPlayerId(const std::string& id);  // Declaration of static method

 private:
  std::string m_id;
  sf::Vector2f m_position;
  sf::Vector2f m_serverAcknowledgedPosition;
  sf::Vector2f m_targetDirection;  // Intended direction from input
  float m_speed;

  sf::CircleShape m_shape;
  sf::Font m_font;  // Font for label and debug text
  sf::Text m_label;
  sf::Text m_debugText;  // For displaying debug info
  bool m_fontLoaded = false;  // Re-added to track if m_font loaded successfully

  unsigned int m_currentSequenceNumber = 0;  // Added
  unsigned int m_lastProcessedSequenceNumber = 0;  // Added for server ACK
  std::vector<sf::Vector2f> m_pendingMoves;  // For client-side prediction
  sf::Vector2f m_serverVerifiedPosition;  // Position confirmed by the server
  bool m_hasServerVerifiedPosition = false;

  static const char* m_localPlayerId;  // Declaration of static member

  // Debugging text
  void initVisuals();
  void updateTextVisuals();  // Added private helper declaration
};

#endif  // WORLD_ENTITIES_PLAYER_H_

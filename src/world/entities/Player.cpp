// Copyright 2025 WildSpark Authors

#include "Player.h"

#include <cmath>
#include <iostream>
#include <sstream>
#include <string>

const float DEFAULT_PLAYER_SPEED = 100.0f;

Player::Player(const std::string& id, sf::Color color, bool isLocalPlayer)
    : m_id(id),
      m_position(0.f, 0.f),
      m_serverAcknowledgedPosition(0.f, 0.f),
      m_targetDirection(0.f, 0.f),
      m_speed(DEFAULT_PLAYER_SPEED),
      m_fontLoaded(false),
      m_currentSequenceNumber(0),
      m_label(m_font, "", 12),
      m_debugText(m_font, "", 10),
      m_isLocalPlayer(isLocalPlayer) {
  initVisuals();
  m_shape.setFillColor(color);
  m_shape.setFillColor(color);
  m_shape.setPosition(m_position);

  if (m_font.openFromFile("/usr/share/fonts/truetype/dejavu/DejaVuSans.ttf")) {
    m_fontLoaded = true;
  } else if (m_font.openFromFile("arial.ttf")) {
    m_fontLoaded = true;
  } else {
    std::cerr << "Player [" << m_id
              << "]: Failed to load font. Text will not be displayed."
              << std::endl;
  }

  if (m_fontLoaded) {
    m_label.setString(m_id);
    m_debugText.setString("Debug Info");
  }

  updateTextVisuals();
}

void Player::initVisuals() {
  m_shape.setRadius(15.f);
  m_shape.setOrigin({m_shape.getRadius(), m_shape.getRadius()});
}

void Player::updateTextVisuals() {
  if (m_fontLoaded) {
    sf::FloatRect labelBounds = m_label.getLocalBounds();
    m_label.setOrigin({labelBounds.position.x + labelBounds.size.x / 2.0f,
                       labelBounds.position.y + labelBounds.size.y / 2.0f});
    m_label.setPosition(
        {m_position.x, m_position.y - m_shape.getRadius() - 10.f});

    sf::FloatRect debugBounds = m_debugText.getLocalBounds();
    m_debugText.setOrigin({debugBounds.position.x, debugBounds.position.y});
    m_debugText.setPosition({m_position.x - m_shape.getRadius(),
                             m_position.y + m_shape.getRadius() + 5.f});
  }
}

void Player::setId(const std::string& id) {
  m_id = id;
  m_label.setString(m_id);
  updateTextVisuals();
}

std::string Player::getId() const { return m_id; }

sf::Vector2f Player::getDirection() const { return m_targetDirection; }

void Player::setPosition(const sf::Vector2f& position) {
  m_position = position;
  m_shape.setPosition(m_position);
  updateTextVisuals();
}

void Player::handleServerUpdate(const sf::Vector2f& serverPosition,
                                unsigned int lastProcessedSequence) {
  m_serverVerifiedPosition = serverPosition;
  m_hasServerVerifiedPosition = true;
  m_lastProcessedSequenceNumber = lastProcessedSequence;

  if (!Player::m_isLocalPlayer) {
    setPosition(serverPosition);
  }

  std::stringstream ss;
  ss << "SrvPos: (" << static_cast<int>(serverPosition.x) << ","
     << static_cast<int>(serverPosition.y) << ")\n"
     << "CliPos: (" << static_cast<int>(m_position.x) << ","
     << static_cast<int>(m_position.y) << ")\n"
     << "Seq: " << m_currentSequenceNumber
     << " Ack: " << m_lastProcessedSequenceNumber;

  if (m_fontLoaded) {
    m_debugText.setString(ss.str());
  }
}

void Player::handleServerAck(unsigned int inputSequence, bool approved,
                             const sf::Vector2f& serverPosition) {
  m_lastProcessedSequenceNumber = inputSequence;
  m_serverVerifiedPosition = serverPosition;
  m_hasServerVerifiedPosition = true;

  if (approved) {
    setPosition(serverPosition);
    m_pendingMoves.clear();
  } else {
    setPosition(serverPosition);
    m_pendingMoves.clear();
  }

  std::stringstream ss;
  ss << "SrvPos: (" << static_cast<int>(m_serverVerifiedPosition.x) << ","
     << static_cast<int>(m_serverVerifiedPosition.y) << ")\n"
     << "CliPos: (" << static_cast<int>(m_position.x) << ","
     << static_cast<int>(m_position.y) << ")\n"
     << "Seq: " << m_currentSequenceNumber
     << " Ack: " << m_lastProcessedSequenceNumber;

  if (m_fontLoaded) {
    m_debugText.setString(ss.str());
  }
}

void Player::update(sf::Time deltaTime) {
  if (Player::m_isLocalPlayer) {
    if (m_targetDirection.x != 0.f || m_targetDirection.y != 0.f) {
      sf::Vector2f moveDelta =
          m_targetDirection * m_speed * deltaTime.asSeconds();
      m_position += moveDelta;
    }
  }
  m_shape.setPosition(m_position);
  updateTextVisuals();
}

void Player::render(sf::RenderTarget& target) {
  target.draw(m_shape);
  if (m_fontLoaded) {
    target.draw(m_label);
    target.draw(m_debugText);
  }
}

unsigned int Player::getNextSequenceNumber() {
  return ++m_currentSequenceNumber;
}

void Player::setTargetDirection(const sf::Vector2f& direction) {
  m_targetDirection = direction;
}

\
#include "Player.h" 
#include <iostream> 
#include <cmath> 
#include <sstream> 

const float DEFAULT_PLAYER_SPEED = 100.0f; 

// Definition of static member
std::string Player::m_localPlayerId = "";

Player::Player(const std::string& id, sf::Color color)
    : m_id(id),
      m_position(0.f, 0.f),
      m_serverAcknowledgedPosition(0.f, 0.f),
      m_targetDirection(0.f, 0.f),
      m_speed(DEFAULT_PLAYER_SPEED),
      m_fontLoaded(false), 
      m_currentSequenceNumber(0),
      m_lastProcessedSequenceNumber(0),
      m_label(m_font, "", 12),      // Corrected: m_font passed first
      m_debugText(m_font, "", 10) // Corrected: m_font passed first
{
    initVisuals();
    m_shape.setFillColor(color);
    m_shape.setPosition(m_position);

    if (m_font.openFromFile("/usr/share/fonts/truetype/dejavu/DejaVuSans.ttf")) {
        m_fontLoaded = true;
    } else if (m_font.openFromFile("arial.ttf")) { 
        m_fontLoaded = true;
    } else {
        std::cerr << "Player [" << m_id << "]: Failed to load font. Text will not be displayed." << std::endl;
    }

    // If font is loaded, set the actual strings. Character size is set by constructor.
    if (m_fontLoaded) {
        m_label.setString(m_id);
        m_debugText.setString("Debug Info");
    }    
    // updateTextVisuals will use the (potentially empty) string if font didn't load,
    // or the correct string if it did. It also relies on m_fontLoaded.
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
        m_label.setPosition({m_position.x, m_position.y - m_shape.getRadius() - 10.f}); 

        sf::FloatRect debugBounds = m_debugText.getLocalBounds();
        m_debugText.setOrigin({debugBounds.position.x, debugBounds.position.y});
        m_debugText.setPosition({m_position.x - m_shape.getRadius(), m_position.y + m_shape.getRadius() + 5.f});
    }
}

void Player::setId(const std::string& id) {
    m_id = id;
    m_label.setString(m_id);
    updateTextVisuals(); 
}

std::string Player::getId() const {
    return m_id;
}

sf::Vector2f Player::getDirection() const {
    return m_targetDirection;
}

/*
bool Player::handleInput(const sf::Event& event, sf::RenderWindow& window, const sf::View& view) {
    if (auto* mouseButtonPressedEvent = event.getIf<sf::Event::MouseButtonPressed>()) {
        if (mouseButtonPressedEvent->button == sf::Mouse::Button::Right) {
            sf::Vector2i mousePos(mouseButtonPressedEvent->position.x, mouseButtonPressedEvent->position.y);
            sf::Vector2f worldPos = window.mapPixelToCoords(mousePos, view);
            sf::Vector2f newDirection = worldPos - getPosition();
            float length = std::sqrt(newDirection.x * newDirection.x + newDirection.y * newDirection.y);
            if (length > 0) {
                newDirection /= length;
            }
            setTargetDirection(newDirection);
            return true; 
        }
    }
    return false; 
}
*/

void Player::setPosition(const sf::Vector2f& position) {
    m_position = position;
    m_shape.setPosition(m_position); 
    updateTextVisuals();
}

void Player::handleServerUpdate(const sf::Vector2f& serverPosition, unsigned int lastProcessedSequence) {
    m_serverVerifiedPosition = serverPosition;
    m_hasServerVerifiedPosition = true;
    m_lastProcessedSequenceNumber = lastProcessedSequence; 

    if (getId() != Player::m_localPlayerId) { // Compare against the static local player ID
         setPosition(serverPosition);
    }

    std::stringstream ss;
    ss << "SrvPos: (" << static_cast<int>(serverPosition.x) << "," << static_cast<int>(serverPosition.y) << ")\n"
       << "CliPos: (" << static_cast<int>(m_position.x) << "," << static_cast<int>(m_position.y) << ")\n"
       << "Seq: " << m_currentSequenceNumber << " Ack: " << m_lastProcessedSequenceNumber;
    if(m_fontLoaded) { 
        m_debugText.setString(ss.str());
    }
}

void Player::handleServerAck(unsigned int inputSequence, bool approved, const sf::Vector2f& serverPosition) {
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
    ss << "SrvPos: (" << static_cast<int>(m_serverVerifiedPosition.x) << "," << static_cast<int>(m_serverVerifiedPosition.y) << ")\n"
       << "CliPos: (" << static_cast<int>(m_position.x) << "," << static_cast<int>(m_position.y) << ")\n"
       << "Seq: " << m_currentSequenceNumber << " Ack: " << m_lastProcessedSequenceNumber;
    if(m_fontLoaded) { 
        m_debugText.setString(ss.str());
    }
}

void Player::update(sf::Time deltaTime) {
    if (getId() == Player::m_localPlayerId) { // Compare against the static local player ID
        if (m_targetDirection.x != 0.f || m_targetDirection.y != 0.f) {
            sf::Vector2f moveDelta = m_targetDirection * m_speed * deltaTime.asSeconds();
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

// Definition of static method
void Player::setLocalPlayerId(const std::string& id) {
    Player::m_localPlayerId = id;
}

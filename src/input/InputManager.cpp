\
#include "InputManager.h"
#include <variant> // Required for std::get_if

InputManager::InputManager() {}

void InputManager::mapActionToKey(const std::string& action, sf::Keyboard::Key key) {
    m_actionInputMap[action] = key;
}

void InputManager::mapActionToMouseButton(const std::string& action, sf::Mouse::Button button) {
    m_actionInputMap[action] = button;
}

void InputManager::handleEvent(const sf::Event& event) {
    if (event.is<sf::Event::KeyPressed>()) {
        const auto* keyPressed = event.getIf<sf::Event::KeyPressed>();
        if (keyPressed) {
            if (m_activeKeys.find(keyPressed->code) == m_activeKeys.end()) {
                m_pressedKeys.insert(keyPressed->code);
            }
            m_activeKeys.insert(keyPressed->code);
        }
    } else if (event.is<sf::Event::KeyReleased>()) {
        const auto* keyReleased = event.getIf<sf::Event::KeyReleased>();
        if (keyReleased) {
            m_activeKeys.erase(keyReleased->code);
            m_releasedKeys.insert(keyReleased->code);
        }
    } else if (event.is<sf::Event::MouseButtonPressed>()) {
        const auto* mousePressed = event.getIf<sf::Event::MouseButtonPressed>();
        if (mousePressed) {
            if (m_activeMouseButtons.find(mousePressed->button) == m_activeMouseButtons.end()) {
                m_pressedMouseButtons.insert(mousePressed->button);
            }
            m_activeMouseButtons.insert(mousePressed->button);
        }
    } else if (event.is<sf::Event::MouseButtonReleased>()) {
        const auto* mouseReleased = event.getIf<sf::Event::MouseButtonReleased>();
        if (mouseReleased) {
            m_activeMouseButtons.erase(mouseReleased->button);
            m_releasedMouseButtons.insert(mouseReleased->button);
        }
    }
}

bool InputManager::isActionActive(const std::string& action) const {
    auto it = m_actionInputMap.find(action);
    if (it != m_actionInputMap.end()) {
        const InputVariant& input = it->second;
        if (const sf::Keyboard::Key* key = std::get_if<sf::Keyboard::Key>(&input)) {
            return m_activeKeys.count(*key);
        }
        if (const sf::Mouse::Button* button = std::get_if<sf::Mouse::Button>(&input)) {
            return m_activeMouseButtons.count(*button);
        }
    }
    return false;
}

bool InputManager::isActionPressed(const std::string& action) const {
    auto it = m_actionInputMap.find(action);
    if (it != m_actionInputMap.end()) {
        const InputVariant& input = it->second;
        if (const sf::Keyboard::Key* key = std::get_if<sf::Keyboard::Key>(&input)) {
            return m_pressedKeys.count(*key);
        }
        if (const sf::Mouse::Button* button = std::get_if<sf::Mouse::Button>(&input)) {
            return m_pressedMouseButtons.count(*button);
        }
    }
    return false;
}

bool InputManager::isActionReleased(const std::string& action) const {
    auto it = m_actionInputMap.find(action);
    if (it != m_actionInputMap.end()) {
        const InputVariant& input = it->second;
        if (const sf::Keyboard::Key* key = std::get_if<sf::Keyboard::Key>(&input)) {
            return m_releasedKeys.count(*key);
        }
        if (const sf::Mouse::Button* button = std::get_if<sf::Mouse::Button>(&input)) {
            return m_releasedMouseButtons.count(*button);
        }
    }
    return false;
}

void InputManager::update() {
    m_pressedKeys.clear();
    m_releasedKeys.clear();
    m_pressedMouseButtons.clear();
    m_releasedMouseButtons.clear();
}

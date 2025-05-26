\
#include "InputManager.h"
#include <variant> // Required for std::get_if
#include <iostream> // For logging

InputManager::InputManager() {}

void InputManager::mapActionToKey(const std::string& action, sf::Keyboard::Key key) {
    m_actionInputMap[action] = key;
}

void InputManager::mapActionToMouseButton(const std::string& action, sf::Mouse::Button button) {
    m_actionInputMap[action] = button;
}

void InputManager::mapActionToMouseWheel(const std::string& action, sf::Mouse::Wheel wheel) {
    // This map stores which wheel an action corresponds to.
    m_actionToWheelMap[action] = wheel;
    // We also add it to m_actionInputMap to indicate this action is associated with a mouse wheel input type.
    // This helps in isActionActive/Pressed/Released if we decide to give them meaning for wheels (e.g., any scroll occurred)
    m_actionInputMap[action] = wheel; 
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
    } else if (event.is<sf::Event::MouseWheelScrolled>()) {
        const auto* scrolled = event.getIf<sf::Event::MouseWheelScrolled>();
        if (scrolled) {
            // ADDED LOGGING HERE
            std::cout << "InputManager::handleEvent: MouseWheelScrolled - Wheel: " << static_cast<int>(scrolled->wheel) << ", Delta: " << scrolled->delta << ", X: " << scrolled->position.x << ", Y: " << scrolled->position.y << std::endl;
            // Accumulate delta for the specific wheel type during this frame
            m_currentFrameMouseWheelDelta[scrolled->wheel] += scrolled->delta;
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
        // For mouse wheel, "active" could mean it scrolled this frame.
        if (const sf::Mouse::Wheel* wheel = std::get_if<sf::Mouse::Wheel>(&input)) {
            auto deltaIt = m_currentFrameMouseWheelDelta.find(*wheel);
            return deltaIt != m_currentFrameMouseWheelDelta.end() && deltaIt->second != 0.0f;
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
        // For mouse wheel, "pressed" could also mean it scrolled this frame (first occurrence of scroll)
        // This might be tricky to define without more state, so often delta is preferred.
        // For now, let's mirror isActionActive for wheel for simplicity, or one could argue it should always be false
        // as wheel is more of a continuous delta than a binary press/release in the traditional sense.
        // Let's make it true if there was any scroll for an action mapped to a wheel.
        if (std::get_if<sf::Mouse::Wheel>(&input)) { // Check if the action is mapped to a wheel
            return getMouseWheelDelta(action) != 0.0f; 
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
        // "Released" doesn't directly map to mouse wheel scroll. 
        // Typically, you check the delta. If delta is 0, it's not scrolling.
        // We'll return false for now for wheel actions.
    }
    return false;
}

float InputManager::getMouseWheelDelta(const std::string& action) const {
    std::cout << "InputManager::getMouseWheelDelta: Called for action: " << action << std::endl; // ADDED LOG
    // Find which wheel this action is mapped to
    auto actionWheelIt = m_actionToWheelMap.find(action);
    if (actionWheelIt != m_actionToWheelMap.end()) {
        sf::Mouse::Wheel wheel = actionWheelIt->second;
        std::cout << "InputManager::getMouseWheelDelta: Action '" << action << "' maps to wheel: " << static_cast<int>(wheel) << std::endl; // ADDED LOG
        // Now get the delta for that wheel from the current frame's deltas
        auto deltaIt = m_currentFrameMouseWheelDelta.find(wheel);
        if (deltaIt != m_currentFrameMouseWheelDelta.end()) {
            std::cout << "InputManager::getMouseWheelDelta: Found delta for wheel " << static_cast<int>(wheel) << ": " << deltaIt->second << std::endl; // ADDED LOG
            return deltaIt->second;
        } else {
            std::cout << "InputManager::getMouseWheelDelta: No delta found for wheel " << static_cast<int>(wheel) << " for action '" << action << "'." << std::endl; // ADDED LOG
        }
    } else {
        std::cout << "InputManager::getMouseWheelDelta: Action '" << action << "' not found in m_actionToWheelMap." << std::endl; // ADDED LOG
    }
    return 0.0f;
}

void InputManager::update() {
    m_pressedKeys.clear();
    m_releasedKeys.clear();
    m_pressedMouseButtons.clear();
    m_releasedMouseButtons.clear();
    m_currentFrameMouseWheelDelta.clear(); // Clear accumulated wheel deltas for the next frame
}

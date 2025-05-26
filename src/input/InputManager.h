\
#ifndef INPUT_MANAGER_H
#define INPUT_MANAGER_H

#include <SFML/Window/Event.hpp>
#include <SFML/Window/Keyboard.hpp>
#include <SFML/Window/Mouse.hpp> // Added for mouse button/wheel
#include <string>
#include <map>
#include <set>
#include <variant> // Required for std::variant

class InputManager {
public:
    // Define an InputVariant that can hold different SFML input types
    using InputVariant = std::variant<sf::Keyboard::Key, sf::Mouse::Button, sf::Mouse::Wheel>;

    InputManager();

    void mapActionToKey(const std::string& action, sf::Keyboard::Key key);
    void mapActionToMouseButton(const std::string& action, sf::Mouse::Button button);
    void mapActionToMouseWheel(const std::string& action, sf::Mouse::Wheel wheel); // Maps action to a specific wheel

    void handleEvent(const sf::Event& event);

    bool isActionActive(const std::string& action) const;
    bool isActionPressed(const std::string& action) const;
    bool isActionReleased(const std::string& action) const;
    float getMouseWheelDelta(const std::string& action) const;

    void update();

private:
    std::map<std::string, InputVariant> m_actionInputMap; // Stores the SFML input type for an action

    std::set<sf::Keyboard::Key> m_activeKeys;
    std::set<sf::Keyboard::Key> m_pressedKeys;
    std::set<sf::Keyboard::Key> m_releasedKeys;

    std::set<sf::Mouse::Button> m_activeMouseButtons;
    std::set<sf::Mouse::Button> m_pressedMouseButtons;
    std::set<sf::Mouse::Button> m_releasedMouseButtons;

    // Stores the accumulated delta for each mouse wheel type for the current frame
    std::map<sf::Mouse::Wheel, float> m_currentFrameMouseWheelDelta;
    // Maps an action string directly to a specific mouse wheel (e.g., "zoom" -> sf::Mouse::Wheel::Vertical)
    std::map<std::string, sf::Mouse::Wheel> m_actionToWheelMap; 
};

#endif // INPUT_MANAGER_H

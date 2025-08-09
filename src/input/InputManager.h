\
#ifndef INPUT_MANAGER_H
#define INPUT_MANAGER_H

#include <SFML/Window/Event.hpp>
#include <SFML/Window/Keyboard.hpp>
#include <SFML/Window/Mouse.hpp> // Added for mouse button
#include <string>
#include <map>
#include <set>
#include <variant> // Required for std::variant

class InputManager {
public:
    // Define an InputVariant that can hold different SFML input types
    using InputVariant = std::variant<sf::Keyboard::Key, sf::Mouse::Button>;

    InputManager();

    void mapActionToKey(const std::string& action, sf::Keyboard::Key key);
    void mapActionToMouseButton(const std::string& action, sf::Mouse::Button button);

    void handleEvent(const sf::Event& event);

    bool isActionActive(const std::string& action) const;
    bool isActionPressed(const std::string& action) const;
    bool isActionReleased(const std::string& action) const;

    void update();

private:
    std::map<std::string, InputVariant> m_actionInputMap; // Stores the SFML input type for an action

    std::set<sf::Keyboard::Key> m_activeKeys;
    std::set<sf::Keyboard::Key> m_pressedKeys;
    std::set<sf::Keyboard::Key> m_releasedKeys;

    std::set<sf::Mouse::Button> m_activeMouseButtons;
    std::set<sf::Mouse::Button> m_pressedMouseButtons;
    std::set<sf::Mouse::Button> m_releasedMouseButtons;
};

#endif // INPUT_MANAGER_H

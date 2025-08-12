// Copyright 2025 WildSpark Authors

#ifndef INPUT_INPUTMANAGER_H_
#define INPUT_INPUTMANAGER_H_

#include <map>
#include <set>
#include <string>
#include <variant>

#include <SFML/Window/Event.hpp>
#include <SFML/Window/Keyboard.hpp>
#include <SFML/Window/Mouse.hpp>

class InputManager {
 public:
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
  std::map<std::string, InputVariant> m_actionInputMap;

  std::set<sf::Keyboard::Key> m_activeKeys;
  std::set<sf::Keyboard::Key> m_pressedKeys;
  std::set<sf::Keyboard::Key> m_releasedKeys;

  std::set<sf::Mouse::Button> m_activeMouseButtons;
  std::set<sf::Mouse::Button> m_pressedMouseButtons;
  std::set<sf::Mouse::Button> m_releasedMouseButtons;
};

#endif  // INPUT_INPUTMANAGER_H_

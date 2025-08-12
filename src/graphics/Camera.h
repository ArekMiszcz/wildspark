// Copyright 2025 WildSpark Authors

#ifndef GRAPHICS_CAMERA_H_
#define GRAPHICS_CAMERA_H_

#include <utility>
#include <SFML/Graphics/RenderTarget.hpp>
#include <SFML/Graphics/View.hpp>
#include <SFML/System/Time.hpp>
#include <SFML/System/Vector2.hpp>

class Camera {
 public:
  explicit Camera(sf::RenderTarget& target, float moveSpeed = 300.0f);
  explicit Camera(float x, float y, float width, float height, float moveSpeed = 300.0f);

  void move(float offsetX, float offsetY);
  void setCenter(float x, float y);
  void setCenter(const sf::Vector2f& center);
  void zoom(float factor);
  void setSize(float width, float height);

  const sf::View& getView() const;
  void applyTo(sf::RenderTarget& target);

  void setMoveSpeed(float speed);
  float getMoveSpeed() const;

  void setMovingUp(bool active);
  void setMovingDown(bool active);
  void setMovingLeft(bool active);
  void setMovingRight(bool active);

  void update(sf::Time deltaTime);

 private:
  sf::View m_view;
  sf::RenderTarget* m_targetRef;
  float m_moveSpeed;
  bool m_movingUp;
  bool m_movingDown;
  bool m_movingLeft;
  bool m_movingRight;
};

#endif  // GRAPHICS_CAMERA_H_

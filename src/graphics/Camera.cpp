#include "Camera.h"
#include <cmath> // For std::sqrt
#include <iostream> // Added for std::cout

Camera::Camera(sf::RenderTarget& target, float moveSpeed)
    : m_targetRef(&target), 
      m_view(target.getDefaultView()), 
      m_moveSpeed(moveSpeed),
      m_movingUp(false), m_movingDown(false), m_movingLeft(false), m_movingRight(false) {
}

Camera::Camera(float x, float y, float width, float height, float moveSpeed)
    : m_targetRef(nullptr), 
      m_view(sf::Vector2f(x, y), sf::Vector2f(width, height)),
      m_moveSpeed(moveSpeed),
      m_movingUp(false), m_movingDown(false), m_movingLeft(false), m_movingRight(false) {
}

void Camera::move(float offsetX, float offsetY) { // This can still be used for direct, non-input based movement
    m_view.move(sf::Vector2f(offsetX, offsetY));
}

void Camera::setCenter(float x, float y) {
    m_view.setCenter(sf::Vector2f(x, y)); // SFML 3.x: Use sf::Vector2f
}

void Camera::setCenter(const sf::Vector2f& center) {
    m_view.setCenter(center);
}

void Camera::zoom(float factor) {
    std::cout << "Camera::zoom called with factor: " << factor << std::endl; // Log zoom factor
    sf::Vector2f oldSize = m_view.getSize(); // Log old size
    m_view.zoom(factor);
    sf::Vector2f newSize = m_view.getSize(); // Log new size
    std::cout << "Camera view size changed from (" << oldSize.x << ", " << oldSize.y 
              << ") to (" << newSize.x << ", " << newSize.y << ")" << std::endl;
}

void Camera::setSize(float width, float height) {
    m_view.setSize(sf::Vector2f(width, height)); // SFML 3.x: Use sf::Vector2f
}

const sf::View& Camera::getView() const {
    return m_view;
}

void Camera::applyTo(sf::RenderTarget& target) {
    target.setView(m_view);
}

void Camera::setMoveSpeed(float speed) {
    m_moveSpeed = speed;
}

float Camera::getMoveSpeed() const {
    return m_moveSpeed;
}

void Camera::setMovingUp(bool active) {
    m_movingUp = active;
}

void Camera::setMovingDown(bool active) {
    m_movingDown = active;
}

void Camera::setMovingLeft(bool active) {
    m_movingLeft = active;
}

void Camera::setMovingRight(bool active) {
    m_movingRight = active;
}

void Camera::update(sf::Time deltaTime) {
    sf::Vector2f movement(0.f, 0.f);

    if (m_movingUp)    movement.y -= 1.f;
    if (m_movingDown)  movement.y += 1.f;
    if (m_movingLeft)  movement.x -= 1.f;
    if (m_movingRight) movement.x += 1.f;

    if (movement.x != 0.f || movement.y != 0.f) {
        // Normalize diagonal movement to maintain consistent speed
        float length = std::sqrt(movement.x * movement.x + movement.y * movement.y);
        if (length != 0) { // Avoid division by zero
            movement /= length;
        }
        m_view.move(movement * m_moveSpeed * deltaTime.asSeconds());
    }
}

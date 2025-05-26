\
#ifndef CAMERA_H
#define CAMERA_H

#include <SFML/Graphics/View.hpp>
#include <SFML/Graphics/RenderTarget.hpp>
#include <SFML/System/Vector2.hpp>
#include <SFML/System/Time.hpp>

class Camera {
public:
    Camera(sf::RenderTarget& target, float moveSpeed = 300.0f); // Initialize with the window/target dimensions
    Camera(float x, float y, float width, float height, float moveSpeed = 300.0f);


    void move(float offsetX, float offsetY); // Kept for direct/programmatic movement if needed
    void setCenter(float x, float y);
    void setCenter(const sf::Vector2f& center);
    void zoom(float factor); // factor > 1 zooms out, factor < 1 zooms in
    void setSize(float width, float height);

    const sf::View& getView() const;
    void applyTo(sf::RenderTarget& target); // Applies the view to the target

    void setMoveSpeed(float speed);
    float getMoveSpeed() const;

    void setMovingUp(bool active);
    void setMovingDown(bool active);
    void setMovingLeft(bool active);
    void setMovingRight(bool active);

    // Optional: update method for smooth movement or following a target
    void update(sf::Time deltaTime);

private:
    sf::View m_view;
    sf::RenderTarget* m_targetRef; // Optional: for easy access to target size for certain operations
    float m_moveSpeed;
    bool m_movingUp;
    bool m_movingDown;
    bool m_movingLeft;
    bool m_movingRight;
};

#endif // CAMERA_H

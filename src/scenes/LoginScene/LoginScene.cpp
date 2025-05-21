#include <iostream>
#include "imgui-SFML.h"
#include "imgui.h"
#include "../SceneManager.h"
#include "LoginScene.h"

LoginScene::LoginScene(sf::RenderWindow& window) : windowRef(window) {
    setupUI();
    std::cout << "Login Scene initialized" << std::endl;
}

void LoginScene::onEnter(SceneManager& manager) {
    std::cout << "Entering Login Scene" << std::endl;
    setupUI();
}

void LoginScene::setupUI() {
    // titleText.setFont(font);
    // titleText.setString("Logowanie do Gry");
    // titleText.setCharacterSize(30);
    // titleText.setFillColor(sf::Color::White);

    // sf::FloatRect textRect = titleText.getLocalBounds();
    // titleText.setOrigin(textRect.left + textRect.width / 2.0f, textRect.top + textRect.height / 2.0f);
    // titleText.setPosition(windowRef.getSize().x / 2.0f, windowRef.getSize().y / 4.0f);

    // loginButtonShape.setSize(sf::Vector2f(200, 50));
    // loginButtonShape.setFillColor(sf::Color(100, 100, 100));
    // loginButtonShape.setOrigin(loginButtonShape.getSize().x / 2.0f, loginButtonShape.getSize().y / 2.0f);
    // loginButtonShape.setPosition(windowRef.getSize().x / 2.0f, windowRef.getSize().y / 2.0f);

    // loginButtonText.setFont(font);
    // loginButtonText.setString("Zaloguj");
    // loginButtonText.setCharacterSize(24);
    // loginButtonText.setFillColor(sf::Color::White);
    // textRect = loginButtonText.getLocalBounds();
    // loginButtonText.setOrigin(textRect.left + textRect.width / 2.0f, textRect.top + textRect.height / 2.0f);
    // loginButtonText.setPosition(loginButtonShape.getPosition());
}


void LoginScene::handleEvent(const sf::Event& event, SceneManager& manager) {
    if (event.is<sf::Event::MouseButtonPressed>()) {
        std::cout << "Login button clicked!" << std::endl;
    }

     if (event.is<sf::Event::Resized>()) {
        setupUI();
    }
}

void LoginScene::update(sf::Time deltaTime, SceneManager& manager) {
    // Logika aktualizacji specyficzna dla sceny logowania (np. animacje)
    // Na razie puste
}

void LoginScene::render(sf::RenderTarget& target) {
    ImGui::Begin("Hello, world!");
    ImGui::Button("Look at this pretty button");
    ImGui::End();
    // target.draw(titleText);
    // target.draw(loginButtonShape);
    // target.draw(loginButtonText);
}
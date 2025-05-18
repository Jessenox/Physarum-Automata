#include "SFML/Graphics.hpp"
#include <iostream>

class InfoButton {
	public:
		InfoButton (int width, int height, int posX, int posY);
		~InfoButton ();
		sf::RectangleShape button;
		sf::Vector2f btnPosition;
		void changeColor(sf::Color);
};

InfoButton ::InfoButton (int width, int height, int posX, int posY) {
	btnPosition = sf::Vector2f(posX, posY);
	button = sf::RectangleShape(sf::Vector2f(width, height));
	button.setPosition(btnPosition);
	button.setFillColor(sf::Color(66, 135, 245));
}

void InfoButton::changeColor(sf::Color newColor) {
	button.setFillColor(newColor);
}

InfoButton ::~InfoButton () {

}
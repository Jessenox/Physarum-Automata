#include "SFML/Graphics.hpp"
#include "Physarum.hpp"
#include <iostream>
#include <ctime>
#include <thread>
#include "files_management.hpp"
#include "LoadMap.hpp"
#include "DensityData.hpp"

#define X 500.f
#define Y 500.f

class App {
public:
	void example();
private:
	void func();
private:

};
void App::func() {
	sf::Vector2f convertPointOfView(sf::Vector2f point, const sf::View & sourceView, const sf::View & destinationView, bool mapViewports = false) {
		point = sourceView.getTransform().transformPoint(point);
		if (!mapViewports)
			return destinationView.getInverseTransform().transformPoint(point);
		const sf::FloatRect sourceViewport{ sourceView.getViewport() };
		const sf::FloatRect destinationViewport{ destinationView.getViewport() };
		point.x = ((point.x + 1.f) * sourceViewport.width / 2.f + sourceViewport.left - destinationViewport.left) * 2.f / destinationViewport.width - 1.f;
		point.y = 1.f - (((1.f - point.y) * sourceViewport.height / 2.f + sourceViewport.top - destinationViewport.top) * 2.f / destinationViewport.height);
		return destinationView.getInverseTransform().transformPoint(point);
	}
}
void App::example() {
	const sf::Vector2u windowSize{ 960u, 540u };
	const std::string windowTitle{ "Convert Point of View" };

	// VIEWS
	sf::View view1;
	view1.setSize(sf::Vector2f(windowSize)); // (0, 0) - (960, 540)
	view1.setCenter(sf::Vector2f(windowSize / 2u)); // (480, 270)
	sf::View view2(view1);
	view2.setSize(sf::Vector2f(windowSize / 2u)); // (0, 0) - (480, 270)
	view2.setCenter(sf::Vector2f(windowSize / 4u)); // (240, 135)
	view1.setViewport({ { 0.f, 0.f },{ 0.75f, 1.f } });
	view2.setViewport({ { 0.1f, 0.25f },{ 0.9f, 0.5f } });

	// VIEWPORT RECTANGLES (to make their region visible)
	sf::RectangleShape viewportRectangle1;
	sf::RectangleShape viewportRectangle2;
	viewportRectangle1.setFillColor(sf::Color(0, 64, 0));
	viewportRectangle2.setFillColor(sf::Color(64, 0, 0));
	viewportRectangle1.setSize(view1.getSize());
	viewportRectangle1.setOrigin(view1.getSize() / 2.f);
	viewportRectangle1.setPosition(view1.getCenter());
	viewportRectangle2.setSize(view2.getSize());
	viewportRectangle2.setOrigin(view2.getSize() / 2.f);
	viewportRectangle2.setPosition(view2.getCenter());

	// CIRCLES (to show the point in each view)
	sf::CircleShape circle1(5.f);
	circle1.setOrigin({ circle1.getRadius(), circle1.getRadius() });
	circle1.setPosition({ 480.f, 270.f });
	sf::CircleShape circle2(circle1);
	circle1.setFillColor(sf::Color::Green);
	circle2.setFillColor(sf::Color::Red);

	bool mapViewports{ false };
	sf::RenderWindow window(sf::VideoMode(windowSize.x, windowSize.y), windowTitle);
	while (window.isOpen())
	{
		sf::Event event;

		sf::Vector2f position{ window.mapPixelToCoords(sf::Mouse::getPosition(window), view1) };

		while (window.pollEvent(event))
		{
			if (event.type == sf::Event::Closed)
				window.close();
			else if ((event.type == sf::Event::KeyPressed) && (event.key.code == sf::Keyboard::M))
			{
				mapViewports = !mapViewports;
				window.setTitle(windowTitle + (mapViewports ? +" (mapping across viewports)" : ""));
			}
			else if (event.type == sf::Event::MouseButtonReleased) {
				std::cout << "Coords: (" << position.x << ", " << position.y << ")\n";
			}
		}
		circle1.setPosition(position);

		//circle2.setPosition(convertPointOfView(circle1.getPosition(), view1, view2, mapViewports)); // this is the important line!

		window.clear();
		window.setView(view1);
		window.draw(viewportRectangle1);
		window.draw(circle1);
		window.setView(view2);
		window.draw(viewportRectangle2, sf::BlendAdd);
		window.draw(circle2, sf::BlendAdd);
		window.display();
	}
}
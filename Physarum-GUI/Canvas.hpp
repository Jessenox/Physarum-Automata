#include <SFML/Graphics.hpp>

class Canvas {
public:
	Canvas(const int, const int);
	sf::Texture getCanvasTexture();
	void putPixel(const int x, const int y);
	void displayCanvas();
private:
public:
private:
	sf::RenderTexture canva;
};

Canvas::Canvas(const int width, const int height) {
	canva.create(width, height);
}

sf::Texture Canvas::getCanvasTexture() {
	return canva.getTexture();
}

void Canvas::putPixel(const int x, const int y) {
	sf::VertexArray myPixel(sf::TrianglesStrip, 4);
	myPixel[0].position = sf::Vector2f(x, y);
	myPixel[1].position = sf::Vector2f(x + 1, y);
	myPixel[2].position = sf::Vector2f(x, y + 1);
	myPixel[3].position = sf::Vector2f(x + 1, y + 1);

	for (int i = 0; i < 4; i++)
		myPixel[i].color = sf::Color(255, 255, 255);

	canva.draw(myPixel);
}

void Canvas::displayCanvas() {
	canva.clear(sf::Color(0,255,0));
	canva.display();
}
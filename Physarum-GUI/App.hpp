#include "imgui.h"
#include "imgui-SFML.h"
#include "Canvas.hpp"

#include <stdio.h>
#include <SFML/Graphics.hpp>


class App {
public:
    App();
    void run();
private:
    void processEvents();
    void update(sf::Time);
    void render();
    sf::Vector2f convertPointOfView(sf::Vector2f, const sf::View&, const sf::View& , bool);
    void handleMouseEvents(sf::Mouse::Button, bool);
public:
    bool running = true;

private:
    sf::RenderWindow myWindow;
    sf::Sprite canvasSprite;
    sf::RenderTexture rt;

    sf::View view1;
    

    sf::View view2;
    

    bool onLeftClick = false;
};

App::App() : myWindow(sf::VideoMode(800, 800), "Physarum Test") {
    myWindow.clear();
    rt.create(500,500);

    view1.reset(sf::FloatRect(0.f, 0.f, 500.f, 500.f));
    view1.setSize(50,50);
    
    view1.setViewport(sf::FloatRect(0.f, 0.f, 1.f, 1.f));
    rt.setView(view1);

    view2.setCenter(sf::Vector2f(350.f, 300.f));
    view2.setSize(sf::Vector2f(200.f, 200.f));
}

void App::run() {
    sf::Clock clock;
    while (myWindow.isOpen()) {
        // handle events
        sf::Time deltaTime = clock.restart();
        processEvents();
        update(deltaTime);
        render();
    }
}

void App::processEvents() {
    sf::Event event;
    while (myWindow.pollEvent(event)) {
        switch (event.type) {
            case sf::Event::KeyPressed:

                break;
            case sf::Event::KeyReleased:

                break;
            case sf::Event::MouseButtonPressed:
                handleMouseEvents(event.mouseButton.button, true);
                break;
            case sf::Event::MouseButtonReleased:
                handleMouseEvents(event.mouseButton.button, false);
                break;
            case sf::Event::Closed:
                running = !running;
                myWindow.close();
                break;
        }
    }
}

void App::render() {
    myWindow.clear(sf::Color(255, 113, 67));
    
    rt.display();
    //canvas.displayCanvas();
    canvasSprite.setTexture(rt.getTexture());
    myWindow.setView(view1);
    myWindow.draw(canvasSprite);
    myWindow.display();
}

void App::update(sf::Time clk) {
    if (onLeftClick) {
        sf::Vector2f actualPosition = convertPointOfView(myWindow.mapPixelToCoords(sf::Mouse::getPosition(myWindow), view1), myWindow.getView(), view1, false);
        if (actualPosition.x < 500 && actualPosition.y < 500) {
            std::cout << "click\n";
            sf::VertexArray myPixel(sf::TrianglesStrip, 4);
            myPixel[0].position = sf::Vector2f(actualPosition.x , actualPosition.y);
            myPixel[1].position = sf::Vector2f(actualPosition.x  + 1, actualPosition.y);
            myPixel[2].position = sf::Vector2f(actualPosition.x , actualPosition.y + 1);
            myPixel[3].position = sf::Vector2f(actualPosition.x  + 1, actualPosition.y + 1);

            for (int i = 0; i < 4; i++)
                myPixel[i].color = sf::Color(255, 255, 255);

            rt.draw(myPixel);
        }
    }
   
}

void App::handleMouseEvents(sf::Mouse::Button button, bool isPressed) {
    if (button == sf::Mouse::Left) {
        onLeftClick = isPressed;
    }
    if (button == sf::Mouse::VerticalWheel) {

    }
}

sf::Vector2f App::convertPointOfView(sf::Vector2f point, const sf::View& sourceView, const sf::View& destinationView, bool mapViewports = false) {
    point = sourceView.getTransform().transformPoint(point);
    if (!mapViewports)
        return destinationView.getInverseTransform().transformPoint(point);
    const sf::FloatRect sourceViewport{ sourceView.getViewport() };
    const sf::FloatRect destinationViewport{ destinationView.getViewport() };
    point.x = ((point.x + 1.f) * sourceViewport.width / 2.f + sourceViewport.left - destinationViewport.left) * 2.f / destinationViewport.width - 1.f;
    point.y = 1.f - (((1.f - point.y) * sourceViewport.height / 2.f + sourceViewport.top - destinationViewport.top) * 2.f / destinationViewport.height);
    return destinationView.getInverseTransform().transformPoint(point);
}
#include "SFML/Graphics.hpp"
#include "Physarum.hpp"
#include <iostream>
#include <ctime>


#define X 500.f
#define Y 500.f

class App {
    public:
        App();
        void run();
    private:
        void processEvents();
        void update(sf::Time);
        void render();
        void handlePlayerInput(sf::Keyboard::Key, bool);
        void handleMouseEvents();
        void setCell(sf::Vector2i);
        void setPhysarumOnTexture();
    private:
        sf::RenderWindow myWindow;
        sf::RenderTexture baseTexture;
        sf::Sprite baseSprite;
        sf::CircleShape mPlayer;
        sf::Clock physarumClock;
        sf::Time physarumTimer;
        Physarum physarum{15};
        bool mNumOne = false, mNumTwo = false, mNumThree = false, 
             mNumFour = false, mNumFive = false, mNumSix = false, 
             mNumSeven = false, mNumEight = false, mNumNine = false,
             onLeftClick = false, mEnterKey = false;
        bool play = false;
        float scale = 15;
        short state = 0;
        int generation = 0;
};

App::App() : myWindow(sf::VideoMode(1000, 700), "Physarum Test"), mPlayer() {
    baseTexture.create(500,500);
    
    mPlayer.setRadius(40.f);
    mPlayer.setPosition(100.f, 100.f);
    mPlayer.setFillColor(sf::Color::Cyan);

}

void App::run() {
    sf::Clock clock;
    while (myWindow.isOpen()) {
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
        case sf::Event::KeyPressed :
            handlePlayerInput(event.key.code, true);
            break;
        case sf::Event::KeyReleased:
            handlePlayerInput(event.key.code, false);
            break;
        case sf::Event::MouseButtonReleased :
            handleMouseEvents();
            break;
        case sf::Event::Closed :
            myWindow.close();
            break;
        }
        
    }
}

void App::handlePlayerInput(sf::Keyboard::Key key, bool isPressed) {
    switch (key) {
    case sf::Keyboard::Num0:
        break;
    case sf::Keyboard::Num1:
        mNumOne = isPressed;
        break;
    case sf::Keyboard::Num2:
        mNumTwo = isPressed;
        break;
    case sf::Keyboard::Num3:
        mNumThree = isPressed;
        break;
    case sf::Keyboard::Num4:
        mNumFour = isPressed;
        break;
    case sf::Keyboard::Num5:
        mNumFive = isPressed;
        break;
    case sf::Keyboard::Num6:
        mNumSix = isPressed;
        break;
    case sf::Keyboard::Num7:
        mNumSeven = isPressed;
        break;
    case sf::Keyboard::Num8:
        mNumEight = isPressed;
        break;
    case sf::Keyboard::Num9:
        mNumNine = isPressed;
        break;
    case sf::Keyboard::Enter:
        mEnterKey = isPressed;
        break;
    default:
        break;
    }
}

void App::handleMouseEvents() {
    sf::Vector2i actualPosition = sf::Mouse::getPosition(myWindow);
    if (actualPosition.x < 500 && actualPosition.y < 500) {
        setCell(actualPosition);
    }
}

void App::setCell(sf::Vector2i pos) {
    sf::Vector2f coordinates((float)pos.x * scale / X, (float)pos.y * scale / Y);
    mPlayer.setPosition(coordinates);
    physarum.setCellState(coordinates.x, coordinates.y, state);
}

void App::setPhysarumOnTexture() {
    baseTexture.clear(sf::Color::Black);
    for (size_t i = 0; i < scale; i++) {
        for (size_t j = 0; j < scale; j++) {
            sf::VertexArray cell(sf::TrianglesStrip, 4);
            cell[0].position = sf::Vector2f(j * X / scale, i * Y / scale);
            cell[1].position = sf::Vector2f(j * X / scale + X / scale, i * Y / scale);
            cell[2].position = sf::Vector2f(j * X / scale, i * Y / scale + Y / scale);
            cell[3].position = sf::Vector2f(j * X / scale + X / scale, i * Y / scale + Y / scale);

            switch (physarum.cells[i][j]) {
                case 0 :
                    for (int k = 0; k < 4; k++)
                        cell[k].color = sf::Color(26, 26, 112);
                    break;
                case 1:
                    for (int k = 0; k < 4; k++)
                        cell[k].color = sf::Color(122, 105, 237);
                    break;
                case 2:
                    for (int k = 0; k < 4; k++)
                        cell[k].color = sf::Color(255, 0, 0);
                    break;                       
                case 3:                          
                    for (int k = 0; k < 4; k++)  
                        cell[k].color = sf::Color(0, 0, 0);
                    break;                       
                case 4:                          
                    for (int k = 0; k < 4; k++)  
                        cell[k].color = sf::Color(255, 224, 54);
                    break;                       
                case 5:                          
                    for (int k = 0; k < 4; k++)  
                        cell[k].color = sf::Color(0, 128, 0);
                    break;                       
                case 6:                          
                    for (int k = 0; k < 4; k++)  
                        cell[k].color = sf::Color(250, 232, 181);
                    break;                       
                case 7:                          
                    for (int k = 0; k < 4; k++)  
                        cell[k].color = sf::Color(46, 79, 79);
                    break;                       
                case 8:                          
                    for (int k = 0; k < 4; k++)  
                        cell[k].color = sf::Color(133, 186, 102);
                    break;
                default:
                    break;
            }

            baseTexture.draw(cell);
        }
    }
}

void App::render() {
    
    myWindow.clear();
    baseTexture.display();
    baseSprite.setTexture(baseTexture.getTexture());
    myWindow.draw(baseSprite);
    myWindow.display();
}

void App::update(sf::Time deltaTime) {
    if (mNumOne)
        state = 0;
    if (mNumTwo)
        state = 1;
    if (mNumThree)
        state = 2;
    if (mNumFour)
        state = 3;
    if (mNumFive)
        state = 4;
    if (mNumSix)
        state = 5;
    if (mNumSeven)
        state = 6;
    if (mNumEight)
        state = 7;
    if (mNumNine)
        state = 8;
    if (mEnterKey) {
        
            play = true;
        
    }
    sf::Vector2f movement(0.f, 0.f);
    mPlayer.move(movement * deltaTime.asSeconds());
    setPhysarumOnTexture();
    if (play && physarumClock.getElapsedTime().asMilliseconds() > 1000.f) {
        std::cout << generation << std::endl;
        physarum.evaluatePhysarum();
        physarumClock.restart();
        generation++;
    }
}

int main() {
    srand(time(NULL));

    App app;
    app.run();
   
    return 0;
}
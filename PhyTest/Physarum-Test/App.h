#include "SFML/Graphics.hpp"
#include "Physarum.hpp"
#include <iostream>
#include <ctime>
#include <thread>
// #include "files_management.hpp"
#include "LoadMap.hpp"
#include "DensityData.hpp"
#include "Matrix.hpp"

#define X 500.f
#define Y 500.f

class App {
public:
    App();
    void run();
    bool OpenFile();
private:
    void processEvents();
    void update(sf::Time);
    void render();
    void handlePlayerInput(sf::Keyboard::Key, bool);
    void handleMouseEvents(sf::Mouse::Button, bool);
    void setCell(sf::Vector2i);
    void setPhysarumOnTexture();
    void textSettings();
    void updateText();
    bool setGeneralFont(std::string);
    void physarumRoute();
    void initializeColors();
    void setMemoryOnTexture();


    void Reforce(int** tab, int n);

private:
    sf::RenderWindow myWindow;
    //sf::RenderTexture baseTexture;
    sf::Sprite baseSprite;
    // For memory analisys
    sf::RenderTexture memoryTexture;
    sf::Sprite memorySprite;


    sf::RectangleShape stateIndicator;

    sf::Clock physarumClock;
    sf::Time physarumTimer;

    sf::Text generationText;
    sf::Text currentStateText;

    sf::Font generalFont;

    std::vector<sf::Color> physarumStateColors;
    std::vector<sf::Color> memoryStateColors;

    Physarum physarum{ 100 };
    bool mNumOne = false, mNumTwo = false, mNumThree = false,
        mNumFour = false, mNumFive = false, mNumSix = false,
        mNumSeven = false, mNumEight = false, mNumNine = false,
        onLeftClick = false, mEnterKey = false, mSKey = false;
    bool play = false;
    float scale = 100;
    short state = 0;
    int generation = 0;
    float interval = 1.f;
    int screenshotTaked = 0;
    int debounce = 0;
    bool started = false;
    LoadMap loadmap;

    std::vector<DensityData> densityValues;
    
    // Optimizing field
    sf::VertexArray cellsMtx;

    std::array<sf::Color, 9> physarumColors{
        sf::Color(26, 26, 112),     // State 0
        sf::Color(122, 105, 237),   // State 1
        sf::Color(255, 0, 0),       // State 2
        sf::Color(0, 0, 0),         // State 3
        sf::Color(255, 224, 54),    // State 4
        sf::Color(0, 128, 0),       // State 5
        sf::Color(250, 232, 181),   // State 6
        sf::Color(46, 79, 79),      // State 7
        sf::Color(133, 186, 102)    // State 8
    };

    std::array<sf::Color, 9> memoryColors{
        sf::Color(250, 21, 5),
        sf::Color(237, 31, 17),
        sf::Color(207, 32, 21),
        sf::Color(184, 33, 24),
        sf::Color(163, 34, 26),
        sf::Color(135, 32, 26),
        sf::Color(112, 30, 25),
        sf::Color(92, 27, 23)
    };
};
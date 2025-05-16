#include "SFML/Graphics.hpp"
#include "Physarum.hpp"
#include <iostream>
#include <ctime>
#include <thread>
#include "files_management.hpp"
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
    sf::RenderTexture baseTexture;
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

    std::vector<sf::Color> stateColors;
    std::vector<sf::Color> memoryStateColors;

    Physarum physarum{ 200 };
    bool mNumOne = false, mNumTwo = false, mNumThree = false,
        mNumFour = false, mNumFive = false, mNumSix = false,
        mNumSeven = false, mNumEight = false, mNumNine = false,
        onLeftClick = false, mEnterKey = false, mSKey = false;
    bool play = false;
    float scale = 200;
    short state = 0;
    int generation = 0;
    float interval = 10.f;
    int screenshotTaked = 0;
    int debounce = 0;
    bool started = false;
    LoadMap loadmap;

    std::vector<DensityData> densityValues;
    
};
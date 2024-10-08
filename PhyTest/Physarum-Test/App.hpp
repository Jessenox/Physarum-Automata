#include "App.h"

App::App() : myWindow(sf::VideoMode(500, 700), "Physarum Test") {
    // Initialize color for physarum states
    initializeColors();
    baseTexture.create(500, 500);

    textSettings();
    stateIndicator.setSize(sf::Vector2f(30, 30));
    stateIndicator.setPosition(10.f, 570.f);
    stateIndicator.setFillColor(stateColors[state]);

    //loadmap.convertImageToMap("C:\\Users\\Angel\\Documents\\OpenGL\\Physarum-Automata\\PhyTest\\Physarum-Test\\MAPS\\espiral.png");
    //loadmap.setDataToArray(physarum.cells, scale, scale);
}

void App::run() {
    sf::Clock clock;
    while (myWindow.isOpen()) {
        sf::Time deltaTime = clock.restart();
        processEvents();
        update(deltaTime);
        render();
    }
    physarum.cleanCells();
}

void App::processEvents() {
    sf::Event event;
    while (myWindow.pollEvent(event)) {
        switch (event.type) {
        case sf::Event::KeyPressed:
            handlePlayerInput(event.key.code, true);
            break;
        case sf::Event::KeyReleased:
            handlePlayerInput(event.key.code, false);
            if (event.key.code == sf::Keyboard::S) {
                std::string name = "Screenshot_" + std::to_string(screenshotTaked) + ".png";
                baseTexture.getTexture().copyToImage().saveToFile("C:\\Users\\pikmi\\Pictures\\Screenshots\\PhysarumCaptures\\" + name);
                //baseTexture.getTexture().copyToImage().saveToFile("C:\\Users\\Angel\\Pictures\\Screenshots\\PhysarumCaptures\\" + name);
                std::cout << "Screenshot saved as: " << name << "\n";
                screenshotTaked++;
            }
            else if (event.key.code == sf::Keyboard::Delete) {



            }
            break;
        case sf::Event::MouseButtonPressed:
            handleMouseEvents(event.mouseButton.button, true);
            break;
        case sf::Event::MouseButtonReleased:
            handleMouseEvents(event.mouseButton.button, false);
            break;
        case sf::Event::Closed:
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

void App::handleMouseEvents(sf::Mouse::Button button, bool isPressed) {
    if (button == sf::Mouse::Left) {
        onLeftClick = isPressed;
    }
}

void App::setCell(sf::Vector2i pos) {
    sf::Vector2f coordinates((float)pos.x * scale / X, (float)pos.y * scale / Y);
    physarum.setCellState(coordinates.x, coordinates.y, state);
}

void App::textSettings() {
    if (setGeneralFont("arial.ttf")) {
        generationText.setFont(generalFont);
        generationText.setCharacterSize(24);
        generationText.setFillColor(sf::Color::White);
        generationText.setPosition(10, 520);

        currentStateText.setFont(generalFont);
        currentStateText.setCharacterSize(24);
        currentStateText.setFillColor(sf::Color::White);
        currentStateText.setPosition(50, 570);
    }
}

void App::updateText() {
    std::string updatedGenerationText = "Generation: " + std::to_string(generation);
    generationText.setString(updatedGenerationText);

    std::string updatedCurrentStateText = "Current State: " + std::to_string(state);
    currentStateText.setString(updatedCurrentStateText);
}

void App::initializeColors() {
    // Actual colors for Physarum
    stateColors.push_back(sf::Color(26, 26, 112)); // State 0
    stateColors.push_back(sf::Color(122, 105, 237)); // State 1
    stateColors.push_back(sf::Color(255, 0, 0)); // State 2
    stateColors.push_back(sf::Color(0, 0, 0)); // State 3
    stateColors.push_back(sf::Color(255, 224, 54)); // State 4
    stateColors.push_back(sf::Color(0, 128, 0)); // State 5
    stateColors.push_back(sf::Color(250, 232, 181)); // State 6
    stateColors.push_back(sf::Color(46, 79, 79)); // State 7
    stateColors.push_back(sf::Color(133, 186, 102)); // State 8

    memoryStateColors.push_back(sf::Color(250, 21, 5));
    memoryStateColors.push_back(sf::Color(237, 31, 17));
    memoryStateColors.push_back(sf::Color(207, 32, 21));
    memoryStateColors.push_back(sf::Color(184, 33, 24));
    memoryStateColors.push_back(sf::Color(163, 34, 26));
    memoryStateColors.push_back(sf::Color(135, 32, 26));
    memoryStateColors.push_back(sf::Color(112, 30, 25));
    memoryStateColors.push_back(sf::Color(92, 27, 23));
}

bool App::setGeneralFont(std::string fontName) {
    if (!generalFont.loadFromFile(fontName)) {
        std::cout << "Error when read font" << "\n";
        return false;
    }
    return true;
}

void App::setMemoryOnTexture() {
    memoryTexture.clear(sf::Color::Black);
    for (size_t i = 0; i < scale; i++) {
        for (size_t j = 0; j < scale; j++) {
            sf::VertexArray cell(sf::TrianglesStrip, 4);
            cell[0].position = sf::Vector2f(j * X / scale, i * Y / scale);
            cell[1].position = sf::Vector2f(j * X / scale + X / scale, i * Y / scale);
            cell[2].position = sf::Vector2f(j * X / scale, i * Y / scale + Y / scale);
            cell[3].position = sf::Vector2f(j * X / scale + X / scale, i * Y / scale + Y / scale);

            switch (physarum.cellsMemory[i][j]) {
            case 1:
                for (int k = 0; k < 4; k++)
                    cell[k].color = memoryStateColors[0];
                break;
            case 2:
                for (int k = 0; k < 4; k++)
                    cell[k].color = memoryStateColors[1];
                break;
            case 3:
                for (int k = 0; k < 4; k++)
                    cell[k].color = memoryStateColors[2];
                break;
            case 4:
                for (int k = 0; k < 4; k++)
                    cell[k].color = memoryStateColors[3];
                break;
            case 5:
                for (int k = 0; k < 4; k++)
                    cell[k].color = memoryStateColors[4];
                break;
            case 6:
                for (int k = 0; k < 4; k++)
                    cell[k].color = memoryStateColors[5];
                break;
            case 7:
                for (int k = 0; k < 4; k++)
                    cell[k].color = memoryStateColors[6];
                break;
            case 8:
                for (int k = 0; k < 4; k++)
                    cell[k].color = memoryStateColors[7];
                break;
            default:
                break;
            }

            memoryTexture.draw(cell);
        }
    }
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
            case 0:
                for (int k = 0; k < 4; k++)
                    cell[k].color = stateColors[0];
                break;
            case 1:
                for (int k = 0; k < 4; k++)
                    cell[k].color = stateColors[1];
                break;
            case 2:
                for (int k = 0; k < 4; k++)
                    cell[k].color = stateColors[2];
                break;
            case 3:
                for (int k = 0; k < 4; k++)
                    cell[k].color = stateColors[3];
                break;
            case 4:
                for (int k = 0; k < 4; k++)
                    cell[k].color = stateColors[4];
                break;
            case 5:
                for (int k = 0; k < 4; k++)
                    cell[k].color = stateColors[5];
                break;
            case 6:
                for (int k = 0; k < 4; k++)
                    cell[k].color = stateColors[6];
                break;
            case 7:
                for (int k = 0; k < 4; k++)
                    cell[k].color = stateColors[7];
                break;
            case 8:
                for (int k = 0; k < 4; k++)
                    cell[k].color = stateColors[8];
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

    myWindow.draw(generationText);
    myWindow.draw(currentStateText);

    myWindow.draw(stateIndicator);
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
    if (onLeftClick) {
        sf::Vector2i actualPosition = sf::Mouse::getPosition(myWindow);
        if (actualPosition.x < 500 && actualPosition.y < 500) {
            setCell(actualPosition);
        }
    }

    setPhysarumOnTexture();

    updateText();
    stateIndicator.setFillColor(stateColors[state]);

    if (play && physarumClock.getElapsedTime().asMilliseconds() > interval) {
        physarum.evaluatePhysarum();
        DensityData data(generation, physarum.statesDensity[0], physarum.statesDensity[1], physarum.statesDensity[2],
            physarum.statesDensity[3], physarum.statesDensity[4], physarum.statesDensity[5],
            physarum.statesDensity[6], physarum.statesDensity[7], physarum.statesDensity[8]);
        densityValues.push_back(data);
        physarumClock.restart();

        if (physarum.routed) {
            //Reforce(physarum.cells, scale);
            PerfectRouteAutomata objAut;
            objAut.getPrevRoute(physarum.cells, scale);
            saveDensityData("hola.txt", densityValues);
            play = false;
        }
        if (generation == 0) {
            saveInitialState(physarum.cells, scale);
            // Get initial point and one nutrient
            getInitialPoints();
        }
        generation++;

    }
}

void App::getInitialPoints() {
    for (size_t i = 0; i < scale; i++) {
        for (size_t j = 0; j < scale; j++) {
            if (physarum.cells[i][j] == 3) {
                _iniPoint = make_tuple('a', j, i); // Name, X, Y
            }
            if (physarum.cells[i][j] == 1) {
                _endPoint = make_tuple('b', j, i); // Name, X, Y
            }
        }
    }
}

void App::getPhysarumTempRoute() {
    for (size_t i = 0; i < scale; i++) {
        for (size_t j = 0; j < scale; j++) {
            if (physarum.cells[i][j] == 5 || physarum.cells[i][j] == 8) {
                // params (X, Y)
                std::tuple<int, int> coords(j, i);
                physarumCellsCoords.push_back(coords);
            }
        }
    }
}


bool App::OpenFile() {
    std::cout << "Hola\n";
    return false;
}

// TO DO
/*
    * First, get all physarum cells when is routed
    * Save all coordinates in array
    * for each cell, calculate Bresenham, stop if cell is repelent
    * finish when point reach nutrient
    * Save route and send 
*/
void App::Reforce(int **tab, int n) {
    bool xReached = false, yReached = false;
    int Xc = 0, Yc = 0;

    int dy = std::get<2>(_endPoint) - get<2>(_iniPoint);
    int dx = std::get<1>(_endPoint) - get<1>(_iniPoint);

    int inXci = 0;
    int inYci = 0;
    int inXcr = 0;
    int inYcr = 0;

    if (dy > 0)
        inYci = 1;
    else {
        dy = -dy;
        inYci = -1;
    }
    
    if (dx >= 0)
        inXci = 1;
    else {
        dx = -dx;
        inXci = -1;
    }

    if (dx >= dy) {
        inYcr = 0;
        inXcr = inXci;
    }
    else {
        inXcr = 0;
        inYcr = inYci;
        int temp = dx;
        dx = dy;
        dy = temp;
    }
    Xc = std::get<1>(_iniPoint);
    Yc = std::get<2>(_iniPoint);

    int avR = 0, av = 0, avI = 0;
    avR = (2 * dy);
    av = avR - dx;
    avI = (av - dx);

    do {
        physarum.setCellState(Xc, Yc,  9);
        std::cout << "x: " << Xc << "y: " << Yc << "\n";
        if (av >= 0) {
            Xc = Xc + inXci;
            Yc = Yc + inYci;
            av = av + avI;
        }
        else {
            Xc = Xc + inXcr;
            Yc = Yc + inYcr;
            av = av + avR;
        }
        if (Xc == std::get<1>(_endPoint)) xReached = true;
        if (Yc == std::get<2>(_endPoint)) yReached = true;
    } while (!xReached || !yReached);
}
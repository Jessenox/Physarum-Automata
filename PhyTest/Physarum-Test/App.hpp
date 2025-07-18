#include "App.h"

App::App() : myWindow(sf::VideoMode(500, 700), "Physarum Simulator") {
    // Initialize color for physarum states
    physarumStateColors.reserve(9);
    memoryStateColors.reserve(9);

    // Initialize colors which belong to each state of physarum cells and memory
    initializeColors();

    // Initialize text settings that are displaying in the window
    textSettings();

    // Set the state indicator color at corresponding position and color
    stateIndicator.setSize(sf::Vector2f(30, 30));
    stateIndicator.setPosition(10.f, 570.f);
    stateIndicator.setFillColor(physarumStateColors[state]);
    
    // Initialize a canvas where the cells and evaluation takes place
    cellsMtx.setPrimitiveType(sf::Quads);
    cellsMtx.resize(scale * scale * 4);

    // Its a 1D array, so we need a counter to separate each cell evaluation when its draws into the window
    int cs { 0 };

    // Filling the canvas with corresponding primitive
    for (size_t i = 0; i < scale; i++) {
        for (size_t j = 0; j < scale; j++) {
            cellsMtx[cs]    .position = sf::Vector2f(j * X / scale              , i * Y / scale);
            cellsMtx[cs + 1].position = sf::Vector2f(j * X / scale + X / scale  , i * Y / scale);
            cellsMtx[cs + 2].position = sf::Vector2f(j * X / scale + X / scale  , i * Y / scale + Y / scale);
            cellsMtx[cs + 3].position = sf::Vector2f(j * X / scale              , i * Y / scale + Y / scale);
            cs += 4;
        }
    }
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
        case sf::Event::KeyPressed:
            handlePlayerInput(event.key.code, true);
            break;
        case sf::Event::KeyReleased:
            handlePlayerInput(event.key.code, false);
            if (event.key.code == sf::Keyboard::S) {
                std::string name = "Screenshot_" + std::to_string(screenshotTaked) + ".png";
                //baseTexture.getTexture().copyToImage().saveToFile("C:\\Users\\pikmi\\Pictures\\Screenshots\\PhysarumCaptures\\" + name);
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
    // Verifyng that font exists in current path
    if ( generalFont.loadFromFile( selected_font ) ) {
        // Setting texts
        generationText.setFont(generalFont);
        generationText.setCharacterSize(24);
        generationText.setFillColor(sf::Color::White);
        generationText.setPosition(10, 520);

        currentStateText.setFont(generalFont);
        currentStateText.setCharacterSize(24);
        currentStateText.setFillColor(sf::Color::White);
        currentStateText.setPosition(50, 570);
    }
    else {
        std::cout << "No se pudo cargar correctamente la fuente: " << selected_font << "/n";
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
    for (const auto & color: physarumColors) {
        physarumStateColors.emplace_back(color);
    }

    for (const auto & color: memoryColors) {
        memoryStateColors.emplace_back(color);
    }
}

void App::setMemoryOnTexture() {
    memoryTexture.clear(sf::Color::Black);
    for (unsigned int i = 0; i < scale; i++) {
        for (unsigned int j = 0; j < scale; j++) {
            sf::VertexArray cell(sf::TrianglesStrip, 4);
            cell[0].position = sf::Vector2f(j * X / scale, i * Y / scale);
            cell[1].position = sf::Vector2f(j * X / scale + X / scale, i * Y / scale);
            cell[2].position = sf::Vector2f(j * X / scale, i * Y / scale + Y / scale);
            cell[3].position = sf::Vector2f(j * X / scale + X / scale, i * Y / scale + Y / scale);

            switch (physarum.mtxMemory.getAt(i, j)) {
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
    int vertexCounter{ 0 };
    for (unsigned int i = 0; i < scale; i++) {
        for (unsigned int j = 0; j < scale; j++) {
            // Get the actual value from cell matrix
            const unsigned int value = physarum.mtxPhysarum.getAt(i, j);

            // We only update colors at the matrix
            for (int k = 0; k < 4; k++)
                cellsMtx[vertexCounter + k].color = physarumStateColors[value];

            // Add 4 for increasing index
            vertexCounter += 4;
        }
    }
    //baseTexture.draw(cellsMtx);

}


void App::render() {
    myWindow.clear();

    myWindow.draw(generationText);
    myWindow.draw(currentStateText);

    myWindow.draw(stateIndicator);

    myWindow.draw(cellsMtx);

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
    if (generation == 0 || play && generation > 0)
        setPhysarumOnTexture();

    updateText();
    stateIndicator.setFillColor(physarumStateColors[state]);

    if (play && physarumClock.getElapsedTime().asMilliseconds() > interval) {

        physarum.evaluatePhysarum();
        physarumClock.restart();

        if (physarum.routed) {
            play = false;
        }
        if (generation == 0) {
            
        }
        generation++;

    }
}
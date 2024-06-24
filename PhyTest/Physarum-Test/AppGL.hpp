#include <stdio.h>
#include <glad/glad.h>
#include <GLFW/glfw3.h>
#include <SFML/Graphics.hpp>

class AppGL {
	public:
        AppGL();
        void run();
	private:
        void processEvents();
        void update();
        void render();
	public:

	private:
		sf::Window myWindow;
        bool running = true;
};

AppGL::AppGL() : myWindow(sf::VideoMode(800, 600), "Physarum Test") {
        myWindow.setVerticalSyncEnabled(true);
        // activate the window
        myWindow.setActive(true);
        // release resources...
}

void AppGL::run() {
    while (running) {
        // handle events
        processEvents();
        update();
        render();
    }
}

void AppGL::processEvents() {
    sf::Event event;
    while (myWindow.pollEvent(event)) {
        if (event.type == sf::Event::Closed) {
            // end the program
            running = false;
        }
        else if (event.type == sf::Event::Resized) {
            // adjust the viewport when the window is resized
            glViewport(0, 0, event.size.width, event.size.height);
        }
    }
}

void AppGL::render() {
    // clear the buffers
    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

    // draw...

    // end the current frame (internally swaps the front and back buffers)
    myWindow.display();
}

void AppGL::update() {

}
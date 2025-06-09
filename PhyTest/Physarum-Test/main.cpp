#include <stdio.h>
//#include "Physarum.hpp"
#include "App.hpp"

int main(int argc, char** argv) {
    srand(time(NULL));
    
    App app;
    app.run();

    return 0;
}
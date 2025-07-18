#include <stdio.h>
//#include "Physarum.hpp"
#include "App.hpp"
#include <thread>


int main(int argc, char** argv) {
    Physarum phy{200};
    //std::jthread phyThread(phy);
    phy.getRoute();
    // App app;
    // app.run();
   
    return 0;
}
#include <stdio.h>
#include "Physarum.hpp"
//#include "App.hpp"

int main(int argc, char** argv) {
    srand(time(NULL));
    Physarum phyApp(200);
    phyApp.setCellState(10, 20, 1);
    phyApp.setCellState(40, 100, 3);
    if (phyApp.getRoute()) {
        for (size_t i = 0; i < 200; i++)
        {
            for (size_t j = 0; j < 200; j++)
            {
                printf("%d", phyApp.mtxPhysarum.getAt(i, j));
            }
            printf("\n");
        }
    }
    else {
        printf("Error\n");
        return -1;
    }
    /*
    App app;
    app.run();
    */

    return 0;
}
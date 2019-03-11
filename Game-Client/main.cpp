#include <iostream>
#include "Client.h"

int main(int nargs, char **argv) {
    std::cout << "Computer Networks Chat Client Starting..." << std::endl;
    Application* app = new Client();
    std::cout << "Setting up" << std::endl;
    app->setup();
    app->run();
    delete app;
    return 0;
}

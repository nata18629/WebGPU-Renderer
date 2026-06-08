#include "Renderer.hpp"
#include <vector>
#include <string>
#include <iostream>

using namespace std;

int main (int argc, char** argv) {
    Renderer renderer;
    vector<string> models(argv+1, argc+argv);
    renderer.Run(models);
    return 0;
}
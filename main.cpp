#include "Renderer.hpp"

int main (int, char**) {
    Renderer app;
    if (!app.Initialize()){
        return 1;
    }
    while (app.IsRunning()) {
        app.MainLoop();
    }
    app.Terminate();
    return 0;
}
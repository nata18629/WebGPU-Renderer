#include "App.h"

int main (int, char**) {
    App app;
    if (!app.Initialize()){
        return 1;
    }
    while (app.IsRunning()) {
        app.MainLoop();
    }
    app.Terminate();
    return 0;
}
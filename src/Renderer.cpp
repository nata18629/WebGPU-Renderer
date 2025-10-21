#include "MainWindow.hpp"
#include "Renderer.hpp"
#include <iostream>

void Renderer::Run() {
    MainWindow window = MainWindow(&gpu);
    window.Initialize();
    gpu.SetWindow(&window);
    gpu.camera = window.camera;
    gpu.Initialize();
    while (window.IsRunning()) {
        gpu.MainLoop();
    }
    gpu.Terminate();
    printf("Terminate window\n");
    window.Terminate();
    printf("Done terminating\n");
}
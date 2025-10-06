#include "MainWindow.hpp"
#include "Renderer.hpp"
#include <iostream>

void Renderer::Run() {
    MainWindow window = MainWindow(&gpu);
    window.Initialize();
    gpu.SetWindow(&window);
    gpu.Initialize();
    while (window.IsRunning()) {
        gpu.MainLoop();
    }
    gpu.Terminate();
    window.Terminate();
}
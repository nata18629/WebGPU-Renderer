#include "MainWindow.hpp"
#include "Renderer.hpp"
#include <iostream>
#include <string>
#include <vector>


void Renderer::Run(std::vector<std::string> models) {
    MainWindow window = MainWindow(&gpu);
    window.Initialize();
    gpu.SetWindow(&window);
    gpu.camera = window.camera;
    gpu.Initialize(models);
    while (window.IsRunning()) {
        gpu.MainLoop();
    }
    gpu.Terminate();
    window.Terminate();
    printf("Done terminating\n");
}
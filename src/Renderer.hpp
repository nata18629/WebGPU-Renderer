#pragma once
#include "Gpu.hpp"

class MainWindow;

class Renderer{
public:
void Run(std::vector<std::string> models);

private:
Gpu gpu;
};
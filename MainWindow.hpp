#pragma once
#include <GLFW/glfw3.h>
#include "Helpers.hpp"

class Gpu;

class MainWindow{
public:
MainWindow(Gpu* gpu);
void Initialize();
bool IsRunning();
void Terminate();
GLFWwindow* GetWindow();

private:
Gpu* gpu;
GLFWwindow* window;
DragState drag;

void OnMouseMove(double x, double y);
void OnMouseButton(int button, int action, int mods);
void OnArrowsPressed(int key, int scancode, int action, int mods);
};
#pragma once
#include <GLFW/glfw3.h>

class Gpu;
class Camera;

class MainWindow{
public:
MainWindow(Gpu* gpu);
void Initialize();
bool IsRunning();
void Terminate();
GLFWwindow* GetWindow();
Camera* camera;

void OnMouseMove(double x, double y);
void OnMouseButton(int button, int action, int mods);
void OnArrowsPressed(int key, int scancode, int action, int mods);
private:
Gpu* gpu;
GLFWwindow* window;

};
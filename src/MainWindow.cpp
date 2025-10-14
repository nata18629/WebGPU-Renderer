#include "MainWindow.hpp"
#include "Camera.hpp"
#include "Gpu.hpp"
#include <iostream>
#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <glm/common.hpp>

#define LEFT 0
#define RIGHT 1
#define UP 2
#define DOWN 3

MainWindow::MainWindow(Gpu* gpu){
    this->gpu = gpu;
    this->camera = new Camera;
}
void MainWindow::Initialize(){
    if (!glfwInit()) {
        std::cerr << "Could not initialize GLFW!" << std::endl;
        return;
    }
    window = glfwCreateWindow(640, 480, "Learn WebGPU", nullptr, nullptr);
    if (!window) {
        std::cerr << "Could not open window!" << std::endl;
        glfwTerminate();
        return;
    }
    glfwSetWindowUserPointer(window, this);
}
bool MainWindow::IsRunning(){
    return !glfwWindowShouldClose(window);
} 
void MainWindow::Terminate(){
    glfwDestroyWindow(window);
    glfwTerminate();
}
GLFWwindow* MainWindow::GetWindow(){
    return window;
}
void MainWindow::OnMouseMove(double x, double y){
    if (camera->drag.active) {
        camera->OnMouseMove(x,y);
        gpu->UpdateViewMatrix();
    }
}
void MainWindow::OnMouseButton(int button, int action, int mods){
    if (button == GLFW_MOUSE_BUTTON_LEFT||GLFW_MOUSE_BUTTON_RIGHT){
        switch(action) {
        case GLFW_PRESS:
            camera->drag.mouseButton = GLFW_MOUSE_BUTTON_LEFT ? LEFT : RIGHT;
            camera->drag.active = true;
            double xpos, ypos;
            glfwGetCursorPos(window, &xpos, &ypos);
            camera->drag.startMouse = glm::vec2(-(float)xpos, (float)ypos);
            camera->drag.startCameraState = camera->cameraState;
            break;
        case GLFW_RELEASE:
            camera->drag.active = false;
            break;
        }
    }
}
void MainWindow::OnArrowsPressed(int key, int scancode, int action, int mods){
    camera->delta=glfwGetTime()-gpu->time;
    if(key==GLFW_KEY_LEFT || key==GLFW_KEY_A){
        camera->OnArrowsPressed(LEFT);
    }
    else if(key==GLFW_KEY_RIGHT || key==GLFW_KEY_D){
        camera->OnArrowsPressed(RIGHT);
    }
    if(key==GLFW_KEY_UP || key==GLFW_KEY_W){
        camera->OnArrowsPressed(UP);
    }
    else if(key==GLFW_KEY_DOWN || key==GLFW_KEY_S){
        camera->OnArrowsPressed(DOWN);
    }
    gpu->UpdateViewMatrix();
}
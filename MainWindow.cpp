#include "MainWindow.hpp"
#include "Gpu.hpp"
#include <iostream>
#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <glm/common.hpp>

constexpr float PI = 3.141592653589793f;


MainWindow::MainWindow(Gpu* gpu){
    this->gpu = gpu;
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
    glfwSetCursorPosCallback(window, [](GLFWwindow* window, double xpos, double ypos) {
        auto that = reinterpret_cast<MainWindow*>(glfwGetWindowUserPointer(window));
        if (that != nullptr) that->OnMouseMove(xpos, ypos);
    });
    glfwSetMouseButtonCallback(window, [](GLFWwindow* window, int button, int action, int mods) {
        auto that = reinterpret_cast<MainWindow*>(glfwGetWindowUserPointer(window));
        if (that != nullptr) that->OnMouseButton(button, action, mods);
    });
    glfwSetKeyCallback(window, [](GLFWwindow* window, int key, int scancode, int action, int mods){
        auto that = reinterpret_cast<MainWindow*>(glfwGetWindowUserPointer(window));
        if (that != nullptr) that->OnArrowsPressed(key, scancode, action, mods);
    });
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
    if (drag.active) {
        glm::vec2 currentMouse = glm::vec2(-(float)x, (float)y);
        glm::vec2 delta = (currentMouse - drag.startMouse) * drag.sensitivity;
        gpu->cameraState.angles = drag.startCameraState.angles + delta;
        gpu->cameraState.angles.y = glm::clamp(gpu->cameraState.angles.y, -PI / 2.0f + 1e-5f, PI / 2.0f - 1e-5f);
        gpu->UpdateViewMatrix();
    }
}
void MainWindow::OnMouseButton(int button, int action, int mods){
    if (button == GLFW_MOUSE_BUTTON_LEFT) {
        switch(action) {
        case GLFW_PRESS:
            drag.active = true;
            double xpos, ypos;
            glfwGetCursorPos(window, &xpos, &ypos);
            drag.startMouse = glm::vec2(-(float)xpos, (float)ypos);
            drag.startCameraState = gpu->cameraState;
            break;
        case GLFW_RELEASE:
            drag.active = false;
            break;
        }
    }
}
void MainWindow::OnArrowsPressed(int key, int scancode, int action, int mods){
    float delta=glfwGetTime()-gpu->time;
    delta*=5;
    float cx = cos(gpu->cameraState.angles.x);
    float sx = sin(gpu->cameraState.angles.x);
    float cy = cos(gpu->cameraState.angles.y);
    float sy = sin(gpu->cameraState.angles.y);
    glm::vec3 direction = glm::vec3(cx * cy, sy, sx * cy);
    direction = glm::normalize(direction);
    if(key==GLFW_KEY_LEFT || key==GLFW_KEY_A){
        gpu->cameraState.position -= glm::normalize(glm::cross(direction,glm::vec3(0,1,0)))*delta;
    }
    else if(key==GLFW_KEY_RIGHT || key==GLFW_KEY_D){
        gpu->cameraState.position += glm::normalize(glm::cross(direction,glm::vec3(0,1,0)))*delta;
    }
    if(key==GLFW_KEY_UP || key==GLFW_KEY_W){
        gpu->cameraState.position -= direction*delta;
    }
    else if(key==GLFW_KEY_DOWN || key==GLFW_KEY_S){
        gpu->cameraState.position += direction*delta;
    }
    gpu->UpdateViewMatrix();
}
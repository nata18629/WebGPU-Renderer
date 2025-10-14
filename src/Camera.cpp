#include "Camera.hpp"
#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <glm/common.hpp>

constexpr float PI = 3.141592653589793f;

#define LEFT 0
#define RIGHT 1
#define UP 2
#define DOWN 3

Camera::Camera()
{
    type = CameraType::DEFAULT;
    UpdateViewMatrix();
}

void Camera::SetCameraType(CameraType type)
{
    this->type = type;
}

void Camera::UpdateViewMatrix()
{
    float cx = cos(cameraState.angles.x);
    float sx = sin(cameraState.angles.x);
    float cy = cos(cameraState.angles.y);
    float sy = sin(cameraState.angles.y);
    glm::vec3 direction = glm::vec3(cx * cy, sy, sx * cy);
    glm::vec3 change = cameraState.position;
    view = glm::lookAt(change,change+direction, glm::vec3(0,1,0));
}

void Camera::OnMouseMove(double x, double y)
{
    Rotate(x, y);
    UpdateViewMatrix();
}

void Camera::OnArrowsPressed(int key)
{
    delta*=5;
    float cx = cos(cameraState.angles.x);
    float sx = sin(cameraState.angles.x);
    float cy = cos(cameraState.angles.y);
    float sy = sin(cameraState.angles.y);
    glm::vec3 direction = glm::vec3(cx * cy, sy, sx * cy);
    direction = glm::normalize(direction);
    switch(key){
    case(LEFT):
        cameraState.position -= glm::normalize(glm::cross(direction,glm::vec3(0,1,0)))*delta;
        break;
    case(RIGHT):
        cameraState.position += glm::normalize(glm::cross(direction,glm::vec3(0,1,0)))*delta;
        break;
    case(UP):
        cameraState.position -= direction*delta;
        break;
    case(DOWN):
        cameraState.position += direction*delta;
        break;
    }
    UpdateViewMatrix();
}

void Camera::Zoom()
{
}

void Camera::Move()
{
}

void Camera::Rotate(double x, double y)
{
    glm::vec2 currentMouse = glm::vec2(-(float)x, (float)y);
    glm::vec2 delta = (currentMouse - drag.startMouse) * drag.sensitivity;
    cameraState.angles = drag.startCameraState.angles+delta;
    cameraState.angles.y = glm::clamp(cameraState.angles.y, -PI / 2.0f + 1e-5f, PI / 2.0f - 1e-5f);
}
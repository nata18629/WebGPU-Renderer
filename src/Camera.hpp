#pragma once
#include "Helpers.hpp"
enum CameraType {DEFAULT}; //TODO: some other camera movement

struct DragState {
    bool active = false;
    glm::vec2 startMouse;
    CameraState startCameraState;
    bool mouseButton;
    float sensitivity = 0.01f;
    float scrollSensitivity = 0.1f;
};

class Camera {
public:
float delta;
CameraState cameraState;
DragState drag;
glm::mat4x4 view = {
        1.0, 0.0, 0.0, 0.0,
        0.0, 1.0, 0.0, 0.0,
        0.0, 0.0, 1.0, 0.0,
        0.0, 0.0, 0.0, 1.0,
    };
Camera();
void UpdateViewMatrix();
void SetCameraType(CameraType type);
void OnMouseMove(double x, double y);
void OnArrowsPressed(int key);

private:
CameraType type;
void Zoom();
void Move();
void Rotate(double x, double y);
};
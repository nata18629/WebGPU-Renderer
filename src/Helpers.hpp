#pragma once
#include <glm/glm.hpp>

struct Uniforms {
    glm::mat4x4 view = {
        1.0, 0.0, 0.0, 0.0,
        0.0, 1.0, 0.0, 0.0,
        0.0, 0.0, 1.0, 0.0,
        0.0, 0.0, 0.0, 1.0,
    };
    glm::vec3 cameraPos;
    float time;
    //float pad[3];
};

struct ObjectTransforms {
    glm::mat4x4 Rot= {
        1.0, 0.0, 0.0, 0.0,
        0.0, 1.0, 0.0, 0.0,
        0.0, 0.0, 1.0, 0.0,
        0.0, 0.0, 0.0, 1.0,
    };
    glm::mat4x4 Scale= {
        1.0, 0.0, 0.0, 0.0,
        0.0, 1.0, 0.0, 0.0,
        0.0, 0.0, 1.0, 0.0,
        0.0, 0.0, 0.0, 1.0,
    };
    glm::mat4x4 Trans= {
        1.0, 0.0, 0.0, 0.0,
        0.0, 1.0, 0.0, 0.0,
        0.0, 0.0, 1.0, 0.0,
        0.0, 0.0, 0.0, 1.0,
    };
};

struct CameraState {
    glm::vec2 angles = {0.0f, 0.0f};
    float zoom;
    glm::vec3 position = glm::vec3(0.0f, 0.0f, 0.0f);  
};

struct DragState {
    bool active = false;
    glm::vec2 startMouse;
    CameraState startCameraState;

    float sensitivity = 0.01f;
    float scrollSensitivity = 0.1f;
};
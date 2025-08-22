#pragma once
#include <GLFW/glfw3.h>
#include <webgpu/webgpu.hpp>
#include <vector>
#include "ResourceManager.hpp"
#include "Helpers.hpp"

using namespace wgpu;

class App{
public:
    bool Initialize();
    void Terminate();
    void MainLoop();
    bool IsRunning();

private:
    Instance instance;
    Device device;
    GLFWwindow* window;
    Surface surface;
    SurfaceConfiguration config;
    Queue queue;
    RenderPipeline pipeline;
    TextureFormat surfaceFormat = TextureFormat::Undefined;
    Texture texture;
    TextureView texView;
    BindGroup bindGroup;
    BindGroupLayout bindGroupLayout;
    PipelineLayout pipelineLayout;
    Buffer vertexBuffer;
    Buffer uniformBuffer;
    uint32_t vertexCount;
    TextureView depthTextureView;
    Texture depthTexture;
    std::vector<VertexAttributes> vertexData;
    CameraState cameraState;
    Uniforms uniforms;
    DragState drag;
    float time=0;
    
    RequiredLimits GetRequiredLimits(Adapter adapter) const;
    void InitializeSurface(Adapter adapter);
    void InitializeBuffers();
    void InitializePipeline();
    std::pair<SurfaceTexture, TextureView> GetNextSurfaceViewData();
    void InitializeTexture();
    void InitializeBinding();
    void SmoothShading();
    void UpdateViewMatrix();
    void OnMouseMove(double x, double y);
    void OnMouseButton(int button, int action, int mods);
    void OnArrowsPressed(int key, int scancode, int action, int mods);
};
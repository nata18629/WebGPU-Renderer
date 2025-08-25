#pragma once
#include <GLFW/glfw3.h>
#include <webgpu/webgpu.hpp>
#include <vector>
#include "Mesh.hpp"
#include "Helpers.hpp"


using namespace wgpu;

class Renderer{
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
    PipelineLayout pipelineLayout;
    std::vector<Mesh> meshes;
    TextureView depthTextureView;
    Texture depthTexture;
    Buffer uniformBuffer;
    BindGroup bindGroup;
    
    BindGroupLayout bindGroupLayout;
    BindGroupLayout meshBindGroupLayout;
    std::vector<BindGroupLayout> bindGroupLayouts;
    CameraState cameraState;
    Uniforms uniforms;
    DragState drag;
    float time=0;
    
    RequiredLimits GetRequiredLimits(Adapter adapter) const;
    void InitializeSurface(Adapter adapter);
    void InitializeMeshes();
    void InitializeUniforms();
    void InitializeBinding();
    void InitializePipeline();
    std::pair<SurfaceTexture, TextureView> GetNextSurfaceViewData();
    void UpdateViewMatrix();
    void OnMouseMove(double x, double y);
    void OnMouseButton(int button, int action, int mods);
    void OnArrowsPressed(int key, int scancode, int action, int mods);
};
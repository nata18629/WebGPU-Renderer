#pragma once
#include <webgpu/webgpu.hpp>
#include <vector>
#include "Mesh.hpp"
#include "Helpers.hpp"
#include "Camera.hpp"
#include <string>
#include <vector>

using namespace wgpu;

class MainWindow;
class Scene;

class Gpu {
public:
bool Initialize(std::vector<std::string> models);
void Terminate();
void MainLoop();
void UpdateViewMatrix();
void SetWindow(MainWindow* window);

float time=0;
Camera* camera;

private:
Instance instance;
Device device;
MainWindow* window;
Surface surface;
SurfaceConfiguration config;
Queue queue;
TextureFormat surfaceFormat = TextureFormat::Undefined;
Scene* scene;
Sampler sampler;
Buffer uniformBuffer;
BindGroup bindGroup;
BindGroupLayout bindGroupLayout;
Uniforms uniforms;

RequiredLimits GetRequiredLimits(Adapter adapter) const;
void InitializeSurface(Adapter adapter);
void InitializeMeshes(std::vector<std::string> models);
void InitializeSampler();
void InitializeUniforms();
void InitializeBinding();
//void InitializePipeline();
void SetCallbacks();
std::pair<SurfaceTexture, TextureView> GetNextSurfaceViewData();
};
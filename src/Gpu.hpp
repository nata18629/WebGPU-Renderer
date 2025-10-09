#pragma once
#include <webgpu/webgpu.hpp>
#include <vector>
#include "Mesh.hpp"
#include "Helpers.hpp"

using namespace wgpu;

class MainWindow;

class Gpu {
public:
bool Initialize();
void Terminate();
void MainLoop();
void UpdateViewMatrix();
void SetWindow(MainWindow* window);

float time=0;
CameraState cameraState;

private:
Instance instance;
Device device;
MainWindow* window;
Surface surface;
SurfaceConfiguration config;
Queue queue;
RenderPipeline pipeline;
TextureFormat surfaceFormat = TextureFormat::Undefined;
PipelineLayout pipelineLayout;
std::vector<Mesh> meshes;
TextureView depthTextureView;
Texture depthTexture;
Sampler sampler;
Buffer uniformBuffer;
BindGroup bindGroup;

BindGroupLayout bindGroupLayout;
BindGroupLayout meshBindGroupLayout;
std::vector<BindGroupLayout> bindGroupLayouts;
Uniforms uniforms;


RequiredLimits GetRequiredLimits(Adapter adapter) const;
void InitializeSurface(Adapter adapter);
void InitializeMeshes();
void InitializeSampler();
void InitializeUniforms();
void InitializeBinding();
void InitializePipeline();
std::pair<SurfaceTexture, TextureView> GetNextSurfaceViewData();
};
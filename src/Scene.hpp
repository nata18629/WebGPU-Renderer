#pragma once
#define WEBGPU_CPP_IMPLEMENTATION
#include <vector>
#include <filesystem>
class Mesh;

namespace fs = std::filesystem;

class Scene {
public:
std::vector<Mesh> meshes;
wgpu::TextureFormat depthTextureFormat=wgpu::TextureFormat::Undefined;
wgpu::TextureView depthTextureView;
wgpu::Texture depthTexture;

Scene(wgpu::Device device, wgpu::BindGroupLayout bindGroupLayout, wgpu::TextureFormat surfaceFormat);
void LoadFromFile(const fs::path& path);
void Terminate();

private:
wgpu::Queue queue;
wgpu::Device device;
wgpu::BindGroupLayout bindGroupLayout;
wgpu::TextureFormat surfaceFormat=wgpu::TextureFormat::Undefined;

void InitializeDepthTexture();
};
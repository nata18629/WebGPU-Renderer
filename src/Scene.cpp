#include <webgpu/webgpu.hpp>
#include "Scene.hpp"
#include "Mesh.hpp"
#include "ResourceManager.hpp"

auto MODELS_DIR = fs::path{"assets/models"};

using namespace wgpu;

Scene::Scene(Device device, BindGroupLayout bindGroupLayout, TextureFormat surfaceFormat)
{
    this->device = device;
    this->bindGroupLayout = bindGroupLayout;
    this->surfaceFormat = surfaceFormat;
    InitializeDepthTexture();
}

void Scene::LoadFromFile(const fs::path &path)
{
    if (path.extension() == ".obj"){
        ResourceManager::loadGeometryObj(MODELS_DIR/path, meshes);
    }
    else if (path.extension() == ".gltf" || path.extension() == ".glb"){
        ResourceManager::loadGeometryGltf(MODELS_DIR/path, meshes);
    }
    
    for (auto &mesh: meshes) {
        mesh.SetGpu(device, bindGroupLayout, surfaceFormat, depthTextureFormat);
    }
}

void Scene::Terminate()
{
    for (auto &mesh : meshes){
        mesh.Terminate();
    }
    depthTextureView.release();
    depthTexture.destroy();
    depthTexture.release();
}

void Scene::InitializeDepthTexture()
{
    depthTextureFormat = TextureFormat::Depth24Plus;
    TextureDescriptor depthTextureDesc;
    depthTextureDesc.dimension = TextureDimension::_2D;
    depthTextureDesc.format = depthTextureFormat;
    depthTextureDesc.mipLevelCount = 1;
    depthTextureDesc.sampleCount = 1;
    depthTextureDesc.size = {640, 480, 1};
    depthTextureDesc.usage = TextureUsage::RenderAttachment;
    depthTextureDesc.viewFormatCount = 1;
    depthTextureDesc.viewFormats = (WGPUTextureFormat*)&depthTextureFormat;
    depthTexture = device.createTexture(depthTextureDesc);
    TextureViewDescriptor depthTextureViewDesc;
    depthTextureViewDesc.aspect = TextureAspect::DepthOnly;
    depthTextureViewDesc.baseArrayLayer = 0;
    depthTextureViewDesc.arrayLayerCount = 1;
    depthTextureViewDesc.baseMipLevel = 0;
    depthTextureViewDesc.mipLevelCount = 1;
    depthTextureViewDesc.dimension = TextureViewDimension::_2D;
    depthTextureViewDesc.format = depthTextureFormat;
    depthTextureView = depthTexture.createView(depthTextureViewDesc);
}

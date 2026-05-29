#include <iostream>
#define WEBGPU_CPP_IMPLEMENTATION
#include <cassert>
#include <filesystem>
#include <GLFW/glfw3.h>
#include <glfw3webgpu.h>
#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <glm/common.hpp>
#include "ResourceManager.hpp"
#include "Gpu.hpp"
#include "MainWindow.hpp"
#include "Scene.hpp"


using namespace wgpu;
namespace fs = std::filesystem;


auto onDeviceError = [](WGPUErrorType type, char const* message, void* /* pUserData */) {
        std::cout << "Uncaptured device error: type " << type;
        if (message) std::cout << " (" << message << ")";
        std::cout << std::endl;
};

bool Gpu::Initialize() {
    // instance
    std::cout <<"start init"<<std::endl;
    InstanceDescriptor desc = {};
    desc.nextInChain = nullptr;
    instance = createInstance(desc);
    if (!instance) {
        std::cerr << "Could not initialize WebGPU!" << std::endl;
        return false;
    }
    // adapter
    RequestAdapterOptions options = {};
    Adapter adapter = instance.requestAdapter(options);
    // device
    DeviceDescriptor devDesc = {};
    RequiredLimits requiredLimits = GetRequiredLimits(adapter);
    devDesc.requiredLimits = &requiredLimits;
    devDesc.deviceLostCallbackInfo.callback = [](const WGPUDevice* /* device */, WGPUDeviceLostReason reason, char const* message, void* /* pUserData */) {    std::cout << "Device lost: reason " << reason;
    if (message) std::cout << " (" << message << ")";
    std::cout << std::endl;
    };
    device = adapter.requestDevice(devDesc);
    wgpuDeviceSetUncapturedErrorCallback(device, onDeviceError, nullptr /* pUserData */);
    
    InitializeSurface(adapter);
    queue = device.getQueue();
    InitializeUniforms();
    InitializeSampler();
    InitializeBinding();
    InitializeMeshes();
    UpdateViewMatrix();
    SetCallbacks();
    adapter.release();
    return true;
}
void Gpu::Terminate(){
    scene->Terminate();
    delete scene;
    uniformBuffer.release();
    bindGroup.release();
    bindGroupLayout.release();
    instance.release();
    surface.unconfigure();
    surface.release();
    device.release();
    queue.release();
}
void Gpu::MainLoop(){
    glfwPollEvents();
    time = static_cast<float>(glfwGetTime());
    queue.writeBuffer(uniformBuffer, offsetof(Uniforms, time), &time, sizeof(float));
    auto [ surfaceTexture, targetView ] = GetNextSurfaceViewData();
    if (!targetView) return;
    RenderPassDescriptor renderPassDesc = {};
    renderPassDesc.nextInChain = nullptr;
    // describe render pass
    // color attachment
    RenderPassColorAttachment renderPassColorAttachment = {};
    renderPassColorAttachment.view = targetView;
    renderPassColorAttachment.resolveTarget = nullptr;
    renderPassColorAttachment.loadOp = WGPULoadOp_Clear;
    renderPassColorAttachment.storeOp = WGPUStoreOp_Store;
    renderPassColorAttachment.clearValue = Color{ 0.6, 0.4, 1.0, 1.0 };
    renderPassColorAttachment.depthSlice = WGPU_DEPTH_SLICE_UNDEFINED;
    renderPassDesc.colorAttachmentCount = 1;
    renderPassDesc.colorAttachments = &renderPassColorAttachment;
    // depth stencil
    RenderPassDepthStencilAttachment depthStencilAttachment;
    depthStencilAttachment.view = scene->depthTextureView;
    depthStencilAttachment.depthClearValue = 1.0f;
    depthStencilAttachment.depthLoadOp = LoadOp::Clear;
    depthStencilAttachment.depthStoreOp = StoreOp::Store;
    depthStencilAttachment.depthReadOnly = false;
    depthStencilAttachment.stencilClearValue = 0;
    depthStencilAttachment.stencilLoadOp = LoadOp::Undefined;
    depthStencilAttachment.stencilStoreOp = StoreOp::Undefined;
    depthStencilAttachment.stencilReadOnly = true;
    renderPassDesc.depthStencilAttachment = &depthStencilAttachment;
    renderPassDesc.timestampWrites = nullptr;
    // encoder
    CommandEncoderDescriptor encoderDesc = {};
    encoderDesc.nextInChain = nullptr;
    encoderDesc.label = "My command encoder";
    CommandEncoder encoder = device.createCommandEncoder(encoderDesc);
    // render pass
    RenderPassEncoder renderPass = encoder.beginRenderPass(renderPassDesc);
    for (auto &mesh : scene->meshes){
        renderPass.setPipeline(mesh.pipeline);
        queue.writeBuffer(mesh.transformsBuffer, 0, &mesh.globalTransforms, sizeof(ObjectTransforms));
        renderPass.setBindGroup(0, bindGroup, 0, nullptr);
        renderPass.setBindGroup(1, mesh.bindGroup, 0, nullptr);
        renderPass.setVertexBuffer(0, mesh.vertexBuffer, 0, mesh.vertexData.size()*sizeof(VertexAttributes));
        if(!mesh.indexData.empty()){
            renderPass.setIndexBuffer(mesh.indexBuffer, IndexFormat::Uint32, 0, mesh.indexBuffer.getSize());
            renderPass.drawIndexed(mesh.indexCount, 1, 0, 0, 0);
        }
        else{
            renderPass.draw(mesh.vertexCount, 1, 0, 0);
        }
    }
    renderPass.end();
    CommandBufferDescriptor cmdBufferDescriptor = {};
    cmdBufferDescriptor.nextInChain = nullptr;
    cmdBufferDescriptor.label = "Command buffer";
    CommandBuffer command = encoder.finish(cmdBufferDescriptor);
    queue.submit(1, &command);

    renderPass.release();
    surface.present();
    targetView.release();
    wgpuTextureRelease(surfaceTexture.texture);
    command.release();
    encoder.release();
}
RequiredLimits Gpu::GetRequiredLimits(Adapter adapter) const {
    SupportedLimits supportedLimits;
    adapter.getLimits(&supportedLimits);
    RequiredLimits requiredLimits = Default;
    requiredLimits.limits.maxVertexAttributes = 1;
    requiredLimits.limits.maxVertexBuffers = 1;
    requiredLimits.limits.maxBufferSize = 6 * 3 * sizeof(float);
    requiredLimits.limits.maxVertexBufferArrayStride = 3 * sizeof(float);
    requiredLimits.limits.minUniformBufferOffsetAlignment = supportedLimits.limits.minUniformBufferOffsetAlignment;
    requiredLimits.limits.minStorageBufferOffsetAlignment = supportedLimits.limits.minStorageBufferOffsetAlignment;
    
    return requiredLimits;
}
std::pair<SurfaceTexture, TextureView> Gpu::GetNextSurfaceViewData() {
    // next texture
    SurfaceTexture surfaceTexture;
    surface.getCurrentTexture(&surfaceTexture);
    if (surfaceTexture.status != WGPUSurfaceGetCurrentTextureStatus_Success) {
        return { surfaceTexture, nullptr };
    }
    // surface texture view
    TextureViewDescriptor viewDescriptor;
    viewDescriptor.nextInChain = nullptr;
    viewDescriptor.label = "Surface texture view";
    viewDescriptor.format = wgpuTextureGetFormat(surfaceTexture.texture);
    viewDescriptor.dimension = WGPUTextureViewDimension_2D;
    viewDescriptor.baseMipLevel = 0;
    viewDescriptor.mipLevelCount = 1;
    viewDescriptor.baseArrayLayer = 0;
    viewDescriptor.arrayLayerCount = 1;
    viewDescriptor.aspect = WGPUTextureAspect_All;
    TextureView targetView = wgpuTextureCreateView(surfaceTexture.texture, &viewDescriptor);
    return { surfaceTexture, targetView };
}
void Gpu::InitializeSurface(Adapter adapter){
    surface = glfwGetWGPUSurface(instance, window->GetWindow());
    SurfaceConfiguration config = {};
    config.nextInChain = nullptr;
    config.width = 640;
    config.height = 480;
    SurfaceCapabilities capabilities;
    surface.getCapabilities(adapter, &capabilities);
    surfaceFormat = capabilities.formats[0];    config.format = surfaceFormat;
    config.viewFormatCount = 0;
    config.viewFormats = nullptr;
    config.usage = WGPUTextureUsage_RenderAttachment;
    config.device = device;
    config.presentMode = WGPUPresentMode_Fifo;
    config.alphaMode = WGPUCompositeAlphaMode_Auto;
    surface.configure(config);
}
void Gpu::InitializeMeshes() {
    scene = new Scene(device, bindGroupLayout, surfaceFormat);
    scene->LoadFromFile("krzeslo.obj");
    scene->LoadFromFile("monkey.obj");
    Mesh& mesh = scene->meshes.at(0);
    Mesh& mesh2 = scene->meshes.at(1);
    mesh2.SetParent(&(scene->meshes.at(0)));
    mesh.SetTransforms({1,1,1},{-1,-5,2});
    mesh2.SetTransforms({1.5,1.5,1.5},{1.0,2.0,2.0});
}
void Gpu::InitializeUniforms() {
    BufferDescriptor bufferDesc;
    bufferDesc.label = "uniform data";
    bufferDesc.size = sizeof(Uniforms);
    bufferDesc.usage = BufferUsage::CopyDst | BufferUsage::Uniform;
    bufferDesc.mappedAtCreation = false;
    uniformBuffer = device.createBuffer(bufferDesc);
    
    float t = static_cast<float>(glfwGetTime());
    queue.writeBuffer(uniformBuffer, offsetof(Uniforms, time), &t, sizeof(float));
}
void Gpu::InitializeSampler() {
    SamplerDescriptor samplerDesc;
    samplerDesc.addressModeU = AddressMode::ClampToEdge;
    samplerDesc.addressModeV = AddressMode::ClampToEdge;
    samplerDesc.addressModeW = AddressMode::ClampToEdge;
    samplerDesc.magFilter = FilterMode::Linear;
    samplerDesc.minFilter = FilterMode::Linear;
    samplerDesc.mipmapFilter = MipmapFilterMode::Linear;
    samplerDesc.lodMinClamp = 0.0f;
    samplerDesc.lodMaxClamp = 1.0f;
    samplerDesc.compare = CompareFunction::Undefined;
    samplerDesc.maxAnisotropy = 1;
    sampler = device.createSampler(samplerDesc);
}
void Gpu::InitializeBinding() {
    // The uniform time binding
    std::vector<BindGroupLayoutEntry> uniformBindingLayout(2, Default);
    uniformBindingLayout[0].binding = 0;
    uniformBindingLayout[0].visibility = ShaderStage::Vertex;
    uniformBindingLayout[0].buffer.type = BufferBindingType::Uniform;
    uniformBindingLayout[0].buffer.minBindingSize = sizeof(Uniforms);
    // sampler binding
    uniformBindingLayout[1].binding = 1;
    uniformBindingLayout[1].visibility = ShaderStage::Fragment;
    uniformBindingLayout[1].sampler.type = SamplerBindingType::Filtering;

    BindGroupLayoutDescriptor bindGroupLayoutDesc{};
    bindGroupLayoutDesc.entryCount = uniformBindingLayout.size();
    bindGroupLayoutDesc.entries = uniformBindingLayout.data();
    bindGroupLayout = device.createBindGroupLayout(bindGroupLayoutDesc);

    std::vector<BindGroupEntry> bindings(2);
    bindings[0].binding = 0;
    bindings[0].buffer = uniformBuffer;
    bindings[0].offset = 0;
    bindings[0].size = sizeof(Uniforms);
    
    bindings[1].binding = 1;
    bindings[1].sampler = sampler;

    BindGroupDescriptor bindGroupDesc;
    bindGroupDesc.layout = bindGroupLayout;
    bindGroupDesc.entryCount = bindings.size();
    bindGroupDesc.entries = bindings.data();
    bindGroup = device.createBindGroup(bindGroupDesc);
}
void Gpu::SetCallbacks(){
    GLFWwindow* window = this->window->GetWindow();
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
void Gpu::UpdateViewMatrix(){
    uniforms.view = camera->view;
    queue.writeBuffer(
        uniformBuffer,
        offsetof(Uniforms, view),
        &uniforms.view,
        sizeof(Uniforms::view)
    );
    glm::vec3 cameraPos = camera->cameraState.position;
    queue.writeBuffer(uniformBuffer, offsetof(Uniforms, cameraPos), &cameraPos, sizeof(Uniforms::cameraPos));
}

void Gpu::SetWindow(MainWindow* window) {
    this->window = window;
}

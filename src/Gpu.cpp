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
    InitializeBinding();
    InitializeMeshes();
    UpdateViewMatrix();
    InitializePipeline();
    adapter.release();

    return true;
}
void Gpu::Terminate(){
    for (auto &mesh : meshes){
        mesh.Terminate();
    }
    depthTextureView.release();
    depthTexture.destroy();
    depthTexture.release();
    uniformBuffer.release();
    bindGroup.release();
    bindGroupLayout.release();
    meshBindGroupLayout.release();
    pipeline.release();
    pipelineLayout.release();
    instance.release();
    surface.unconfigure();
    surface.release();
    device.release();
    queue.release();
}
void Gpu::MainLoop(){
    glfwPollEvents();
    time = static_cast<float>(glfwGetTime()); // glfwGetTime returns a double
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
    depthStencilAttachment.view = depthTextureView;
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
    renderPass.setPipeline(pipeline);
    for (auto &mesh : meshes){
        queue.writeBuffer(mesh.transformsBuffer, 0, &mesh.transforms, sizeof(ObjectTransforms));
        //std::vector<BindGroup> bindGroups = {bindGroup, mesh.bindGroup};
        renderPass.setBindGroup(0, bindGroup, 0, nullptr);
        renderPass.setBindGroup(1, mesh.bindGroup, 0, nullptr);
        renderPass.setVertexBuffer(0, mesh.vertexBuffer, 0, mesh.vertexData.size()*sizeof(VertexAttributes));
        renderPass.draw(mesh.vertexCount, 1, 0, 0);
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
    config = {};
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
    Mesh mesh(device, queue, meshBindGroupLayout, "assets/textures/asteroid.obj");
    //Mesh mesh2(device, queue, meshBindGroupLayout, "krzeslo.obj");
    //Mesh mesh3(device, queue, meshBindGroupLayout, "obszar_prism.obj");
    mesh.SetTransforms(glm::vec3(2.0f,2.0f,2.0f),glm::vec3(0.0f,3.0f,1.0f),glm::vec3(1.0f,1.0f,1.0f));
    //mesh3.SetTransforms(glm::vec3(2.0f,2.0f,2.0f),glm::vec3(0.0f,6.0f,1.0f),glm::vec3(1.0f,1.0f,1.0f));
    //meshes = {mesh, mesh2, mesh3};
    meshes = {mesh};
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
void Gpu::InitializeBinding() {
    // The uniform time binding
    BindGroupLayoutEntry uniformBindingLayout(Default);
    uniformBindingLayout.binding = 0;
    uniformBindingLayout.visibility = ShaderStage::Vertex;
    uniformBindingLayout.buffer.type = BufferBindingType::Uniform;
    uniformBindingLayout.buffer.minBindingSize = sizeof(Uniforms);

    BindGroupLayoutDescriptor bindGroupLayoutDesc{};
    bindGroupLayoutDesc.entryCount = 1;
    bindGroupLayoutDesc.entries = &uniformBindingLayout;
    bindGroupLayout = device.createBindGroupLayout(bindGroupLayoutDesc);

    BindGroupEntry binding;
    binding.binding = 0;
    binding.buffer = uniformBuffer;
    binding.offset = 0;
    binding.size = sizeof(Uniforms);

    BindGroupDescriptor bindGroupDesc;
    bindGroupDesc.layout = bindGroupLayout;
    bindGroupDesc.entryCount = 1;
    bindGroupDesc.entries = &binding;
    bindGroup = device.createBindGroup(bindGroupDesc);

    std::vector<BindGroupLayoutEntry> bindingLayoutEntries(2, Default);
    //BindGroupLayoutEntry textureBindingLayout(Default);
    bindingLayoutEntries[0].binding = 0;
    bindingLayoutEntries[0].visibility = ShaderStage::Fragment;
    bindingLayoutEntries[0].texture.sampleType = TextureSampleType::Float;
    bindingLayoutEntries[0].texture.viewDimension = TextureViewDimension::_2D;
    bindingLayoutEntries[0].texture.multisampled = 0;
    
    bindingLayoutEntries[1].binding = 1;
    bindingLayoutEntries[1].visibility = ShaderStage::Vertex;
    bindingLayoutEntries[1].buffer.type = BufferBindingType::Uniform;
    bindingLayoutEntries[1].buffer.minBindingSize = sizeof(ObjectTransforms);

    // Create a bind group layout
    bindGroupLayoutDesc.entryCount = bindingLayoutEntries.size();
    bindGroupLayoutDesc.entries = bindingLayoutEntries.data();
    meshBindGroupLayout = device.createBindGroupLayout(bindGroupLayoutDesc);
    
    bindGroupLayouts = {bindGroupLayout, meshBindGroupLayout};
}
void Gpu::InitializePipeline(){
    // create shader module
    ShaderModule shaderModule = ResourceManager::loadShaderModule("src/shaders.wgsl", device);
    std::cout<<fs::current_path();
    if (shaderModule == nullptr) {
        std::cerr << "Could not load shader!" << std::endl;
        exit(1);
    }
    // vertex buffer layout
    VertexBufferLayout vertexBufferLayout;
    std::vector<VertexAttribute> vertexAttrib(4);
    
    vertexAttrib[0].shaderLocation = 0;
    vertexAttrib[0].offset = offsetof(VertexAttributes, position);
    vertexAttrib[0].format = VertexFormat::Float32x3;
    vertexAttrib[1].shaderLocation = 1;
    vertexAttrib[1].offset = offsetof(VertexAttributes, normal);
    vertexAttrib[1].format = VertexFormat::Float32x3;
    vertexAttrib[2].shaderLocation = 2;
    vertexAttrib[2].offset = offsetof(VertexAttributes, color);
    vertexAttrib[2].format = VertexFormat::Float32x3;
    vertexAttrib[3].shaderLocation = 3;
    vertexAttrib[3].offset = offsetof(VertexAttributes, texCoords);
    vertexAttrib[3].format = VertexFormat::Float32x2;

    vertexBufferLayout.attributeCount = vertexAttrib.size();
    vertexBufferLayout.attributes = vertexAttrib.data();
    vertexBufferLayout.arrayStride = sizeof(VertexAttributes);
    vertexBufferLayout.stepMode = VertexStepMode::Vertex;

    // pipeline
    RenderPipelineDescriptor pipelineDesc;
    pipelineDesc.label = "Pipeline";
    pipelineDesc.vertex.bufferCount = 1;
    pipelineDesc.vertex.buffers = &vertexBufferLayout;
    // vertex shader
    pipelineDesc.vertex.module = shaderModule;
    pipelineDesc.vertex.entryPoint = "vs_main";
    pipelineDesc.vertex.constantCount = 0;
    pipelineDesc.vertex.constants = nullptr;
    pipelineDesc.primitive.topology = PrimitiveTopology::TriangleList;
    pipelineDesc.primitive.stripIndexFormat = IndexFormat::Undefined;
    pipelineDesc.primitive.frontFace = FrontFace::CCW;
    pipelineDesc.primitive.cullMode = CullMode::None;
    // fragment shader
    FragmentState fragmentState;
    fragmentState.module = shaderModule;
    fragmentState.entryPoint = "fs_main";
    fragmentState.constantCount = 0;
    fragmentState.constants = nullptr;
    // blending
    BlendState blendState;
    blendState.color.srcFactor = BlendFactor::SrcAlpha;
    blendState.color.dstFactor = BlendFactor::OneMinusSrcAlpha;
    blendState.color.operation = BlendOperation::Add;
    blendState.alpha.srcFactor = BlendFactor::Zero;
    blendState.alpha.dstFactor = BlendFactor::One;
    blendState.alpha.operation = BlendOperation::Add;
    ColorTargetState colorTarget;
    colorTarget.format = surfaceFormat;
    colorTarget.blend = &blendState;
    colorTarget.writeMask = ColorWriteMask::All;
    fragmentState.targetCount = 1;
    fragmentState.targets = &colorTarget;
    pipelineDesc.fragment = &fragmentState;
    DepthStencilState depthStencilState = Default;
    depthStencilState.depthCompare = CompareFunction::Less;
    depthStencilState.depthWriteEnabled = true;
    TextureFormat depthTextureFormat = TextureFormat::Depth24Plus;
    depthStencilState.format = depthTextureFormat;
    depthStencilState.stencilReadMask = 0;
    depthStencilState.stencilWriteMask = 0;
    pipelineDesc.depthStencil = &depthStencilState;
    
    // multisampling
    pipelineDesc.multisample.count = 1;
    pipelineDesc.multisample.mask = ~0u;
    pipelineDesc.multisample.alphaToCoverageEnabled = false;

    PipelineLayoutDescriptor layoutDesc{};
    layoutDesc.bindGroupLayoutCount = bindGroupLayouts.size();
    layoutDesc.bindGroupLayouts = (WGPUBindGroupLayout*)bindGroupLayouts.data();
    pipelineLayout = device.createPipelineLayout(layoutDesc);
    pipelineDesc.layout = pipelineLayout;

    pipeline = device.createRenderPipeline(pipelineDesc);
    shaderModule.release();

    // Create the depth texture
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

void Gpu::UpdateViewMatrix(){
    float cx = cos(cameraState.angles.x);
    float sx = sin(cameraState.angles.x);
    float cy = cos(cameraState.angles.y);
    float sy = sin(cameraState.angles.y);
    glm::vec3 direction = glm::vec3(cx * cy, sy, sx * cy);
    glm::vec3 change = cameraState.position;
    uniforms.view = glm::lookAt(change,change+direction, glm::vec3(0,1,0));

    queue.writeBuffer(
        uniformBuffer,
        offsetof(Uniforms, view),
        &uniforms.view,
        sizeof(Uniforms::view)
    );
    glm::vec3 cameraPos = cameraState.position+direction;
    queue.writeBuffer(uniformBuffer, offsetof(Uniforms, cameraPos), &cameraPos, sizeof(Uniforms::cameraPos));
}

void Gpu::SetWindow(MainWindow* window) {
    this->window = window;
}

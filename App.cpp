#include <iostream>
#define WEBGPU_CPP_IMPLEMENTATION
#include <cassert>
#include <GLFW/glfw3.h>
#include <glfw3webgpu.h>
#define STB_IMAGE_IMPLEMENTATION
#include <stb_image.h>
#include <filesystem>
#include <fstream>

//#include "ResourceManager.hpp"
#include "App.hpp"

using namespace wgpu;
namespace fs = std::filesystem;

auto RESOURCE_DIR = fs::path{"textures"};
auto MODELS_DIR = fs::path{"models"};

auto onDeviceError = [](WGPUErrorType type, char const* message, void* /* pUserData */) {
        std::cout << "Uncaptured device error: type " << type;
        if (message) std::cout << " (" << message << ")";
        std::cout << std::endl;
};
Texture LoadTexture(const fs::path& path, Device device, TextureView* pTextureView){
    // create texture
    
    int width, height, channels;
    unsigned char *data = stbi_load(path.string().c_str(), &width, &height, &channels, 4);
    if (data==nullptr) return nullptr;
    TextureDescriptor textureDesc;
    textureDesc.dimension = TextureDimension::_2D;
    textureDesc.format = TextureFormat::RGBA8Unorm; // by convention for bmp, png and jpg file. Be careful with other formats.
    textureDesc.mipLevelCount = 1;
    textureDesc.sampleCount = 1;
    textureDesc.size = { (unsigned int)width, (unsigned int)height, 1 };
    textureDesc.usage = TextureUsage::TextureBinding | TextureUsage::CopyDst;
    textureDesc.viewFormatCount = 0;
    textureDesc.viewFormats = nullptr;
    Texture texture = device.createTexture(textureDesc);
    // load texture to gpu
    ImageCopyTexture destination;
    destination.texture = texture;
    destination.mipLevel = 0;
    destination.origin = { 0, 0, 0 }; // equivalent of the offset argument of Queue::writeBuffer
    destination.aspect = TextureAspect::All; // only relevant for depth/Stencil textures
    TextureDataLayout source;
    source.offset = 0;
    source.bytesPerRow = 4 *textureDesc.size.width;
    source.rowsPerImage = textureDesc.size.height;
    Queue queue = device.getQueue();
    queue.writeTexture(destination, data, 4 * textureDesc.size.width * textureDesc.size.height, source, textureDesc.size);
    queue.release();

    TextureViewDescriptor textureViewDesc;
    textureViewDesc.aspect = TextureAspect::All;
    textureViewDesc.baseArrayLayer = 0;
    textureViewDesc.arrayLayerCount = 1;
    textureViewDesc.baseMipLevel = 0;
    textureViewDesc.mipLevelCount = textureDesc.mipLevelCount;
    textureViewDesc.dimension = TextureViewDimension::_2D;
    textureViewDesc.format = textureDesc.format;
    *pTextureView = texture.createView(textureViewDesc);

    stbi_image_free(data);
    return texture;
}

bool App::Initialize() {
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
    devDesc.deviceLostCallback = [](WGPUDeviceLostReason reason, char const* message, void* /* pUserData */) {
    std::cout << "Device lost: reason " << reason;
    if (message) std::cout << " (" << message << ")";
    std::cout << std::endl;
    };
    device = adapter.requestDevice(devDesc);
    wgpuDeviceSetUncapturedErrorCallback(device, onDeviceError, nullptr /* pUserData */);
    
    if (!glfwInit()) {
    std::cerr << "Could not initialize GLFW!" << std::endl;
        return false;
    }
    window = glfwCreateWindow(640, 480, "Learn WebGPU", nullptr, nullptr);
    if (!window) {
    std::cerr << "Could not open window!" << std::endl;
    glfwTerminate();
    return 1;
    }
    // surface
    surface = glfwGetWGPUSurface(instance, window);
    config = {};
    config.nextInChain = nullptr;
    config.width = 640;
    config.height = 480;
    surfaceFormat = surface.getPreferredFormat(adapter);
    config.format = surfaceFormat;
    config.viewFormatCount = 0;
    config.viewFormats = nullptr;
    config.usage = WGPUTextureUsage_RenderAttachment;
    config.device = device;
    config.presentMode = WGPUPresentMode_Fifo;
    config.alphaMode = WGPUCompositeAlphaMode_Auto;
    surface.configure(config);
    // queue
    queue = device.getQueue();
    InitializeTexture();
    InitializeBuffers();
    InitializeBinding();
    InitializePipeline();
    adapter.release();

    return true;
}
void App::Terminate(){
    depthTextureView.release();
    depthTexture.destroy();
    depthTexture.release();
    uniformBuffer.release();
    vertexBuffer.release();
    bindGroup.release();
    bindGroupLayout.release();
    pipeline.release();
    pipelineLayout.release();
    instance.release();
    glfwDestroyWindow(window);
    glfwTerminate();
    surface.unconfigure();
    surface.release();
    device.release();
    queue.release();
    texture.destroy();
    texture.release();
    texView.release();
}
void App::MainLoop(){
    glfwPollEvents();
    float t = static_cast<float>(glfwGetTime()); // glfwGetTime returns a double
    queue.writeBuffer(uniformBuffer, 0, &t, sizeof(float));
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
    renderPass.setBindGroup(0, bindGroup, 0, nullptr);
    renderPass.setVertexBuffer(0, vertexBuffer, 0, vertexData.size()*sizeof(VertexAttributes));
    renderPass.draw(vertexCount, 1, 0, 0);
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
    //instance.processEvents();
}
bool App::IsRunning(){
    return !glfwWindowShouldClose(window);
} 
RequiredLimits App::GetRequiredLimits(Adapter adapter) const {
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
std::pair<SurfaceTexture, TextureView> App::GetNextSurfaceViewData() {
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
void App::InitializePipeline(){
    // create shader module
    ShaderModule shaderModule = ResourceManager::loadShaderModule("shaders.wgsl", device);
    if (shaderModule == nullptr) {
        std::cerr << "Could not load shader!" << std::endl;
        exit(1);
    }
    // vertex buffer layout
    VertexBufferLayout vertexBufferLayout;
    std::vector<VertexAttribute> vertexAttrib(3);
    
    vertexAttrib[0].shaderLocation = 0;
    vertexAttrib[0].offset = offsetof(VertexAttributes, position);
    vertexAttrib[0].format = VertexFormat::Float32x3;
    vertexAttrib[1].shaderLocation = 1;
    vertexAttrib[1].offset = offsetof(VertexAttributes, normal);
    vertexAttrib[1].format = VertexFormat::Float32x3;
    vertexAttrib[2].shaderLocation = 2;
    vertexAttrib[2].offset = offsetof(VertexAttributes, color);
    vertexAttrib[2].format = VertexFormat::Float32x3;

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
    depthStencilState.depthCompare = CompareFunction::Always;
    depthStencilState.depthWriteEnabled = false;
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
    layoutDesc.bindGroupLayoutCount = 1;
    layoutDesc.bindGroupLayouts = (WGPUBindGroupLayout*)&bindGroupLayout;
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
void App::InitializeBuffers(){

    ResourceManager::loadGeometryObj(MODELS_DIR/"monkey.obj", vertexData);

    vertexCount = static_cast<int>(vertexData.size());
    std::cout << vertexCount;

    BufferDescriptor bufferDesc;
    bufferDesc.label = "vertex data";
    bufferDesc.usage = BufferUsage::CopyDst | BufferUsage::Vertex;
    bufferDesc.size = vertexData.size() * sizeof(VertexAttributes);
    bufferDesc.mappedAtCreation = false;
    vertexBuffer = device.createBuffer(bufferDesc);
    queue.writeBuffer(vertexBuffer, 0, vertexData.data(), bufferDesc.size);

    bufferDesc.label = "time data";
    bufferDesc.size = 4 * sizeof(float);
    bufferDesc.usage = BufferUsage::CopyDst | BufferUsage::Uniform;
    bufferDesc.mappedAtCreation = false;
    uniformBuffer = device.createBuffer(bufferDesc);
    float currentTime = 1.0f;
    queue.writeBuffer(uniformBuffer, 0, &currentTime, sizeof(float));

}
void App::InitializeTexture(){
    texView = nullptr;
    texture = LoadTexture(RESOURCE_DIR/"obszar.jpg", device, &texView);
}
void App::InitializeBinding(){
    std::vector<BindGroupLayoutEntry> bindingLayoutEntries(2, Default);
    // The texture binding
    BindGroupLayoutEntry& textureBindingLayout = bindingLayoutEntries[0];
    textureBindingLayout.binding = 0;
    textureBindingLayout.visibility = ShaderStage::Fragment;
    textureBindingLayout.texture.sampleType = TextureSampleType::Float;
    textureBindingLayout.texture.viewDimension = TextureViewDimension::_2D;
    textureBindingLayout.texture.multisampled = 0;
    // The uniform time binding
    BindGroupLayoutEntry& uniformBindingLayout = bindingLayoutEntries[1];
    uniformBindingLayout.binding = 1;
    uniformBindingLayout.visibility = ShaderStage::Vertex;
    uniformBindingLayout.buffer.type = BufferBindingType::Uniform;
    uniformBindingLayout.buffer.minBindingSize = 4 * sizeof(float);
    // Create a bind group layout
    BindGroupLayoutDescriptor bindGroupLayoutDesc{};
    bindGroupLayoutDesc.entryCount = 2;
    bindGroupLayoutDesc.entries = bindingLayoutEntries.data();
    bindGroupLayout = device.createBindGroupLayout(bindGroupLayoutDesc);
    // Create a binding
    std::vector<BindGroupEntry> bindings(2);

    bindings[0].binding = 0;
    bindings[0].textureView = texView;

    bindings[1].binding = 1;
    bindings[1].buffer = uniformBuffer;
    bindings[1].offset = 0;
    bindings[1].size = 4*sizeof(float);

    BindGroupDescriptor bindGroupDesc;
    bindGroupDesc.layout = bindGroupLayout;
    bindGroupDesc.entryCount = 2;
    bindGroupDesc.entries = bindings.data();
    bindGroup = device.createBindGroup(bindGroupDesc);
}
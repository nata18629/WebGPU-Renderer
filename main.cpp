#include <iostream>
#define WEBGPU_CPP_IMPLEMENTATION
#include <webgpu/webgpu.hpp>
#include <cassert>
#include <GLFW/glfw3.h>
#include <glfw3webgpu.h>
#define STB_IMAGE_IMPLEMENTATION
#include <stb_image.h>
#include <filesystem>

using namespace wgpu;
namespace fs = std::filesystem;

auto RESOURCE_DIR = fs::path{"textures"};

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
    uint32_t vertexCount;
    std::vector<float> vertexData;
    
    RequiredLimits GetRequiredLimits(Adapter adapter) const;
    void InitializeBuffers();
    void InitializePipeline();
    std::pair<SurfaceTexture, TextureView> GetNextSurfaceViewData();
    void InitializeTexture();
    void InitializeBinding();
    
};

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
    std::cout <<"before init tex"<<std::endl;
    InitializeTexture();
    InitializeBinding();
    InitializeBuffers();
    InitializePipeline();
    adapter.release();

    return true;
}
void App::Terminate(){
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
    auto [ surfaceTexture, targetView ] = GetNextSurfaceViewData();
    if (!targetView) return;
    RenderPassDescriptor renderPassDesc = {};
    renderPassDesc.nextInChain = nullptr;
    // describe render pass
    RenderPassColorAttachment renderPassColorAttachment = {};
    renderPassColorAttachment.view = targetView;
    renderPassColorAttachment.resolveTarget = nullptr;
    renderPassColorAttachment.loadOp = WGPULoadOp_Clear;
    renderPassColorAttachment.storeOp = WGPUStoreOp_Store;
    renderPassColorAttachment.clearValue = Color{ 0.6, 0.4, 1.0, 1.0 };
    renderPassColorAttachment.depthSlice = WGPU_DEPTH_SLICE_UNDEFINED;
    renderPassDesc.colorAttachmentCount = 1;
    renderPassDesc.colorAttachments = &renderPassColorAttachment;
    renderPassDesc.depthStencilAttachment = nullptr;
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
    renderPass.setVertexBuffer(0, vertexBuffer, 0, vertexBuffer.getSize());
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
    requiredLimits.limits.maxBufferSize = 6 * 2 * sizeof(float);
    requiredLimits.limits.maxVertexBufferArrayStride = 2 * sizeof(float);
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
    const char* shaderSource = R"(
    struct VertexInput {
        @location(0) position: vec2f,
        @location(1) coord: vec2f,
    };

    struct VertexOutput {
        @builtin(position) position: vec4f,
        @location(0) uv: vec2f,
    };

    @vertex
    fn vs_main(@location(0) vertex_pos: vec2f) -> VertexOutput {
        //let coords = (vertex_pos+1.0)*100.0;
        let pos = vec4f(vertex_pos, 0.0, 1.0);
        let uv = vertex_pos.xy*0.5+0.5;
        return VertexOutput(pos, uv);
    }
    @group(0) @binding(0) var imageTexture: texture_2d<f32>;
    @fragment
    fn fs_main(in: VertexOutput) -> @location(0) vec4f {
    let texelCoords = vec2i(in.uv * vec2f(textureDimensions(imageTexture)));
    let color = textureLoad(imageTexture, texelCoords, 0).rgb;
    return vec4f(color,1.0);
    }
    )";
    // create shader module
    ShaderModuleDescriptor shaderDesc;
    ShaderModuleWGSLDescriptor shaderCodeDesc;
    shaderCodeDesc.chain.next = nullptr;
    shaderCodeDesc.chain.sType = SType::ShaderModuleWGSLDescriptor;
    shaderCodeDesc.code = shaderSource;
    shaderDesc.nextInChain = &shaderCodeDesc.chain;
    ShaderModule shaderModule = device.createShaderModule(shaderDesc);

    // vertex buffer layout
    VertexBufferLayout vertexBufferLayout;
    VertexAttribute positionAttrib;
    positionAttrib.shaderLocation = 0;
    positionAttrib.offset = 0;
    positionAttrib.format = VertexFormat::Float32x2;
    vertexBufferLayout.attributeCount = 1;
    vertexBufferLayout.attributes = &positionAttrib;
    vertexBufferLayout.arrayStride = 2 * sizeof(float);
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
    pipelineDesc.depthStencil = nullptr;
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
}
void App::InitializeBuffers(){
    vertexData = {
        -0.8, +0.7,
        -0.4, +0.7,
        -0.4, -0.5,

        -0.8, +0.7,
        -0.8, -0.5,
        -0.4, -0.5
    };

    vertexCount = static_cast<uint32_t>(vertexData.size() / 2);

    BufferDescriptor bufferDesc;
    bufferDesc.label = "vertex data";
    bufferDesc.usage = BufferUsage::CopyDst | BufferUsage::Vertex;
    bufferDesc.size = vertexData.size() * sizeof(float);
    bufferDesc.mappedAtCreation = false;
    vertexBuffer = device.createBuffer(bufferDesc);
    queue.writeBuffer(vertexBuffer, 0, vertexData.data(), bufferDesc.size);
}
void App::InitializeTexture(){
    texView = nullptr;
    texture = LoadTexture(RESOURCE_DIR/"obszar.jpg", device, &texView);
}
void App::InitializeBinding(){
    BindGroupLayoutEntry textureBindingLayout = Default;
    // The texture binding
    textureBindingLayout.binding = 0;
    textureBindingLayout.visibility = ShaderStage::Fragment;
    textureBindingLayout.texture.sampleType = TextureSampleType::UnfilterableFloat;
    textureBindingLayout.texture.viewDimension = TextureViewDimension::_2D;
    textureBindingLayout.texture.multisampled = 0;

    // Create a bind group layout
    BindGroupLayoutDescriptor bindGroupLayoutDesc{};
    bindGroupLayoutDesc.entryCount = 1;
    bindGroupLayoutDesc.entries = &textureBindingLayout;
    bindGroupLayout = device.createBindGroupLayout(bindGroupLayoutDesc);
    // Create a binding
    BindGroupEntry binding;

    binding.binding = 0;
    binding.textureView = texView;

    BindGroupDescriptor bindGroupDesc;
    bindGroupDesc.layout = bindGroupLayout;
    bindGroupDesc.entryCount = 1;
    bindGroupDesc.entries = &binding;
    bindGroup = device.createBindGroup(bindGroupDesc);
}


int main (int, char**) {
    App app;
    if (!app.Initialize()){
        return 1;
    }
    while (app.IsRunning()) {
        app.MainLoop();
    }
    app.Terminate();
    return 0;
}
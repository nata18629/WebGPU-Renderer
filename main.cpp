#include <iostream>
#define WEBGPU_CPP_IMPLEMENTATION
#include <webgpu/webgpu.hpp>
#include <cassert>
#include <GLFW/glfw3.h>
#include <glfw3webgpu.h>

using namespace wgpu;

class App{
public:
    bool Initialize();
    void Terminate();
    void MainLoop();
    bool IsRunning();

private:
    WGPUInstance instance;
    WGPUDevice device;
    GLFWwindow* window;
    WGPUSurface surface;
    WGPUSurfaceConfiguration config;
    WGPUQueue queue;
    RenderPipeline pipeline;
    TextureFormat surfaceFormat = TextureFormat::Undefined;

    void InitializePipeline();
    WGPUDevice requestDeviceSync(WGPUAdapter adapter, WGPUDeviceDescriptor const* descriptor);
    WGPUAdapter requestAdapterSync(WGPUInstance instance, WGPURequestAdapterOptions const * options);
    std::pair<WGPUSurfaceTexture, WGPUTextureView> GetNextSurfaceViewData();
    void DrawTriangle();
};

bool App::Initialize() {
    // instance
    WGPUInstanceDescriptor desc = {};
    desc.nextInChain = nullptr;
    instance = wgpuCreateInstance(&desc);
    if (!instance) {
        std::cerr << "Could not initialize WebGPU!" << std::endl;
        return false;
    }
    // adapter
    WGPURequestAdapterOptions options = {};
    bool requestEnded = false;
    WGPUAdapter adapter = requestAdapterSync(instance, &options);
    // WGPUAdapterProperties properties = {};
    // properties.nextInChain = nullptr;
    // wgpuAdapterGetProperties(adapter, &properties);
    // device
    WGPUDeviceDescriptor devDesc = {};
    device = requestDeviceSync(adapter, &devDesc);
    
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
    WGPUTextureFormat format = wgpuSurfaceGetPreferredFormat(surface, adapter);
    config.format = format;
    config.viewFormatCount = 0;
    config.viewFormats = nullptr;
    config.usage = WGPUTextureUsage_RenderAttachment;
    config.device = device;
    config.presentMode = WGPUPresentMode_Fifo;
    config.alphaMode = WGPUCompositeAlphaMode_Auto;
    wgpuSurfaceConfigure(surface, &config);
    // queue
    queue = wgpuDeviceGetQueue(device);

    wgpuAdapterRelease(adapter);
    return true;
}
void App::Terminate(){
    pipeline.release();
    wgpuInstanceRelease(instance);
    glfwDestroyWindow(window);
    glfwTerminate();
    wgpuSurfaceUnconfigure(surface);
    wgpuSurfaceRelease(surface);
    wgpuDeviceRelease(device);
    wgpuQueueRelease(queue);
}
void App::MainLoop(){
    glfwPollEvents();
    auto [ surfaceTexture, targetView ] = GetNextSurfaceViewData();
    if (!targetView) return;
    WGPURenderPassDescriptor renderPassDesc = {};
    renderPassDesc.nextInChain = nullptr;
    // describe render pass
    WGPURenderPassColorAttachment renderPassColorAttachment = {};
    renderPassColorAttachment.view = targetView;
    renderPassColorAttachment.resolveTarget = nullptr;
    renderPassColorAttachment.loadOp = WGPULoadOp_Clear;
    renderPassColorAttachment.storeOp = WGPUStoreOp_Store;
    renderPassColorAttachment.clearValue = WGPUColor{ 0.6, 0.4, 1.0, 1.0 };
    renderPassColorAttachment.depthSlice = WGPU_DEPTH_SLICE_UNDEFINED;
    renderPassDesc.colorAttachmentCount = 1;
    renderPassDesc.colorAttachments = &renderPassColorAttachment;
    renderPassDesc.depthStencilAttachment = nullptr;
    renderPassDesc.timestampWrites = nullptr;
    // encoder
    WGPUCommandEncoderDescriptor encoderDesc = {};
    encoderDesc.nextInChain = nullptr;
    encoderDesc.label = "My command encoder";
    WGPUCommandEncoder encoder = wgpuDeviceCreateCommandEncoder(device, &encoderDesc);
    // render pass
    WGPURenderPassEncoder renderPass = wgpuCommandEncoderBeginRenderPass(encoder, &renderPassDesc);
    wgpuRenderPassEncoderEnd(renderPass);
    WGPUCommandBufferDescriptor cmdBufferDescriptor = {};
    cmdBufferDescriptor.nextInChain = nullptr;
    cmdBufferDescriptor.label = "Command buffer";
    WGPUCommandBuffer command = wgpuCommandEncoderFinish(encoder, &cmdBufferDescriptor);
    // std::cout << "Submitting command..." << std::endl;
    wgpuQueueSubmit(queue, 1, &command);
    // std::cout << "Command submitted." << std::endl;

    wgpuRenderPassEncoderRelease(renderPass);
    wgpuSurfacePresent(surface);
    wgpuTextureViewRelease(targetView);
    wgpuTextureRelease(surfaceTexture.texture);
    wgpuCommandBufferRelease(command);
    wgpuCommandEncoderRelease(encoder);
    wgpuInstanceProcessEvents(instance);
}
bool App::IsRunning(){
    return !glfwWindowShouldClose(window);
}
WGPUDevice App::requestDeviceSync(WGPUAdapter adapter, WGPUDeviceDescriptor const* descriptor){
    struct UserData {
        WGPUDevice device = nullptr;
        bool requestEnded = false;
    };
    UserData userData;
    auto onDeviceRequestEnded = [](WGPURequestDeviceStatus status, WGPUDevice device, char const* message, void* pUserData) {
        UserData& userData = *reinterpret_cast<UserData*>(pUserData);
        if (status == WGPURequestDeviceStatus_Success) {
            userData.device = device;
        } else {
            std::cout << "Could not get WebGPU device: " << message << std::endl;
        }
        userData.requestEnded = true;
    };

    wgpuAdapterRequestDevice(adapter, descriptor, onDeviceRequestEnded, (void*)&userData);

    assert(userData.requestEnded);
    return userData.device;
} 
WGPUAdapter App::requestAdapterSync(WGPUInstance instance, WGPURequestAdapterOptions const * options) {

    struct UserData {
        WGPUAdapter adapter = nullptr;
        bool requestEnded = false;
    };
    UserData userData;
    auto onAdapterRequestEnded = [](WGPURequestAdapterStatus status, WGPUAdapter adapter, char const * message, void * pUserData) {
        UserData& userData = *reinterpret_cast<UserData*>(pUserData);
        if (status == WGPURequestAdapterStatus_Success) {
            userData.adapter = adapter;
        } else {
            std::cout << "Could not get WebGPU adapter: " << message << std::endl;
        }
        userData.requestEnded = true;
    };

    // Call to the WebGPU request adapter procedure
    wgpuInstanceRequestAdapter(
        instance /* equivalent of navigator.gpu */,
        options,
        onAdapterRequestEnded,
        (void*)&userData
    );

    // We wait until userData.requestEnded gets true
    assert(userData.requestEnded);

    return userData.adapter;
}
std::pair<WGPUSurfaceTexture, WGPUTextureView> App::GetNextSurfaceViewData() {
    // nextTexture
    WGPUSurfaceTexture surfaceTexture;
    wgpuSurfaceGetCurrentTexture(surface, &surfaceTexture);
    if (surfaceTexture.status != WGPUSurfaceGetCurrentTextureStatus_Success) {
        return { surfaceTexture, nullptr };
    }
    // surfaceTextureView
    WGPUTextureViewDescriptor viewDescriptor;
    viewDescriptor.nextInChain = nullptr;
    viewDescriptor.label = "Surface texture view";
    viewDescriptor.format = wgpuTextureGetFormat(surfaceTexture.texture);
    viewDescriptor.dimension = WGPUTextureViewDimension_2D;
    viewDescriptor.baseMipLevel = 0;
    viewDescriptor.mipLevelCount = 1;
    viewDescriptor.baseArrayLayer = 0;
    viewDescriptor.arrayLayerCount = 1;
    viewDescriptor.aspect = WGPUTextureAspect_All;
    WGPUTextureView targetView = wgpuTextureCreateView(surfaceTexture.texture, &viewDescriptor);
    return { surfaceTexture, targetView };
}
void App::InitializePipeline(){
    // RenderPipelineDescriptor pipelineDesc;
    // pipeline = createRenderPipeline(pipelineDesc);
}
void App::DrawTriangle(){

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
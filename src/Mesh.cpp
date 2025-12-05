#include <stb_image.h>
#include <GLFW/glfw3.h>
#include <glm/gtc/matrix_transform.hpp>
#include <webgpu/webgpu.hpp>
#include <filesystem>
#include <iostream>
#include "ResourceManager.hpp"
#include "Mesh.hpp"

namespace fs = std::filesystem;

auto RESOURCE_DIR = fs::path{"assets/textures"};
uint8_t TEXTURE[4] = {200,200,200,255};
uint8_t NORMAL_MAP[4] = {128,128,255,255};

Texture Mesh::LoadTexture(const std::filesystem::path& path, TextureView* pTextureView, void *data){
    // create texture   
    int width, height, channels;
    if(data==nullptr){
        data = stbi_load(path.string().c_str(), &width, &height, &channels, 4); 
        if (data==nullptr) return nullptr;
    }
    else{
        width=1;
        height=1;
    }
    
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
    
    if(path!=""){
        stbi_image_free(data);
    }
    return texture;
}

void Mesh::SetTransforms(glm::vec3 scale, glm::vec3 translate, glm::vec3 rotate) {
    glm::mat4x4 S=glm::scale(glm::mat4x4(1.0), scale);
    glm::mat4x4 T=glm::translate(glm::mat4x4(1.0), translate);
    localTransforms.Scale=S;
    localTransforms.Trans=T;
    localTransforms.Rot=localTransforms.Trans*localTransforms.Scale;
    UpdateTransforms();
}

void Mesh::UpdateTransforms() {
    if(parent!=nullptr){
        globalTransforms.Scale=parent->globalTransforms.Scale*localTransforms.Scale;
        globalTransforms.Trans=parent->globalTransforms.Trans*localTransforms.Trans;
    }
    else{
        globalTransforms.Scale=localTransforms.Scale;
        globalTransforms.Trans=localTransforms.Trans;
    }
    globalTransforms.Rot = globalTransforms.Trans*globalTransforms.Scale;
    for(auto child:children){
        child->UpdateTransforms();
    }
}

Mesh* Mesh::GetParent() {
    return parent;
}

std::vector<Mesh*> Mesh::GetChildren() {
    return children;
}

void Mesh::AddChild(Mesh* child) {
    children.push_back(child);
}

void Mesh::SetParent(Mesh* parent) {
    this->parent = parent;
    parent->AddChild(this);
}

void Mesh::SetGpu(Device device, BindGroupLayout bindGroupLayout, TextureFormat surfaceFormat, TextureFormat depthTextureFormat)
{
    this->device = device;
    this->queue = device.getQueue();
    this->bindGroupLayout = bindGroupLayout;
    this->surfaceFormat = surfaceFormat;
    this->depthTextureFormat = depthTextureFormat;
    InitializeTexture(texturePath);
    InitializeNormalMap(normalMapPath);
    InitializeBuffers();
    InitializeBinding();
    InitializePipeline();
}

void Mesh::InitializeNormalMap(const std::filesystem::path& path)
{
    if(path==""){
        normalTexture = LoadTexture(path, &normalTexView, NORMAL_MAP);
    }
    else{
        normalTexture = LoadTexture(RESOURCE_DIR/path, &normalTexView);
    }
}


void Mesh::InitializeTexture(const std::filesystem::path& path) {
    if(path==""){
        texture = LoadTexture(path, &texView, TEXTURE);
    }
    else{
        texture = LoadTexture(RESOURCE_DIR/path, &texView);
    }    
}

void Mesh::InitializeBuffers() {
    vertexCount = static_cast<int>(vertexData.size());

    BufferDescriptor bufferDesc;
    bufferDesc.label = "vertex data";
    bufferDesc.usage = BufferUsage::CopyDst | BufferUsage::Vertex;
    bufferDesc.size = vertexData.size() * sizeof(VertexAttributes);
    bufferDesc.mappedAtCreation = false;
    vertexBuffer = device.createBuffer(bufferDesc);
    queue.writeBuffer(vertexBuffer, 0, vertexData.data(), bufferDesc.size);

    bufferDesc.label = "object transforms data";
    bufferDesc.size = sizeof(ObjectTransforms);
    bufferDesc.usage = BufferUsage::CopyDst | BufferUsage::Uniform;
    bufferDesc.mappedAtCreation = false;
    transformsBuffer = device.createBuffer(bufferDesc);
    queue.writeBuffer(transformsBuffer, 0, &globalTransforms, bufferDesc.size);

    indexCount = static_cast<int>(indexData.size());
    if(indexCount>0){
        bufferDesc.label = "index data";
        bufferDesc.usage = BufferUsage::CopyDst | BufferUsage::Index;
        bufferDesc.size = indexData.size() * sizeof(uint32_t);
        bufferDesc.mappedAtCreation = false;
        indexBuffer = device.createBuffer(bufferDesc);
        queue.writeBuffer(indexBuffer, 0, indexData.data(), bufferDesc.size);
    }
}

void Mesh::InitializeBinding() {
    std::vector<BindGroupLayoutEntry> bindingLayoutEntries(3, Default);
    bindingLayoutEntries[0].binding = 0;
    bindingLayoutEntries[0].visibility = ShaderStage::Fragment;
    bindingLayoutEntries[0].texture.sampleType = TextureSampleType::Float;
    bindingLayoutEntries[0].texture.viewDimension = TextureViewDimension::_2D;
    bindingLayoutEntries[0].texture.multisampled = 0;
    
    bindingLayoutEntries[1].binding = 1;
    bindingLayoutEntries[1].visibility = ShaderStage::Vertex;
    bindingLayoutEntries[1].buffer.type = BufferBindingType::Uniform;
    bindingLayoutEntries[1].buffer.minBindingSize = sizeof(ObjectTransforms);

    bindingLayoutEntries[2].binding = 2;
    bindingLayoutEntries[2].visibility = ShaderStage::Fragment;
    bindingLayoutEntries[2].texture.sampleType = TextureSampleType::Float;
    bindingLayoutEntries[2].texture.viewDimension = TextureViewDimension::_2D;
    bindingLayoutEntries[2].texture.multisampled = 0;
    
    std::vector<BindGroupEntry> bindings(3);
    bindings[0].binding = 0;
    bindings[0].textureView = texView;
    bindings[1].binding = 1;
    bindings[1].buffer = transformsBuffer;
    bindings[1].offset = 0;
    bindings[1].size = sizeof(ObjectTransforms);
    bindings[2].binding = 2;
    bindings[2].textureView = normalTexView;

    BindGroupLayoutDescriptor bindGroupLayoutDesc{};
    bindGroupLayoutDesc.entryCount = bindingLayoutEntries.size();
    bindGroupLayoutDesc.entries = bindingLayoutEntries.data();
    meshBindGroupLayout = device.createBindGroupLayout(bindGroupLayoutDesc);

    BindGroupDescriptor bindGroupDesc;
    bindGroupDesc.label = "mesh bind group";
    bindGroupDesc.layout = meshBindGroupLayout;
    bindGroupDesc.entryCount = bindings.size();
    bindGroupDesc.entries = bindings.data();
    bindGroup = device.createBindGroup(bindGroupDesc);

    bindGroupLayouts={bindGroupLayout, meshBindGroupLayout};
}

void Mesh::InitializePipeline()
{
    ShaderModule shaderModule = ResourceManager::loadShaderModule("src/shaders.wgsl", device);
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
    
}

void Mesh::Terminate() {
    vertexBuffer.release();
    if(indexCount>0){
        indexBuffer.release();
    }
    transformsBuffer.release();
    meshBindGroupLayout.release();
    pipeline.release();
    pipelineLayout.release();
    bindGroup.release();
    if(texture!=nullptr){
        texture.destroy();
        texture.release();
        texView.release();
    }    
    if(normalTexture!=nullptr){
        normalTexture.destroy();
        normalTexture.release();
        normalTexView.release();
    }
}
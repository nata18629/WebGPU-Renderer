#define STB_IMAGE_IMPLEMENTATION
#include <stb_image.h>
#include <GLFW/glfw3.h>
#include <glm/gtc/matrix_transform.hpp>
#include <webgpu/webgpu.hpp>
#include <filesystem>
#include "Mesh.hpp"

namespace fs = std::filesystem;

auto RESOURCE_DIR = fs::path{"textures"};
auto MODELS_DIR = fs::path{"models"};

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

Mesh::Mesh(Device device, Queue queue, BindGroupLayout bindGroupLayout, const std::filesystem::path& path) {
    this->device = device;
    this->queue = queue;
    InitializeTexture();
    InitializeBuffers(path);
    InitializeBinding(bindGroupLayout);
}

void Mesh::SetTransforms(glm::vec3 scale, glm::vec3 translate, glm::vec3 rotate) {
    glm::mat4x4 S = glm::scale(glm::mat4x4(1.0), scale);
    glm::mat4x4 T = glm::translate(glm::mat4x4(1.0), translate);
    transforms.Rot = T*S;
}

void Mesh::InitializeTexture() {
    texView = nullptr;
    texture = LoadTexture(RESOURCE_DIR/"asteroid.png", device, &texView);
}

void Mesh::InitializeBuffers(const std::filesystem::path& path) {
    ResourceManager::loadGeometryObj(MODELS_DIR/path, vertexData);
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
    queue.writeBuffer(transformsBuffer, 0, &transforms, bufferDesc.size);
}

void Mesh::InitializeBinding(BindGroupLayout bindGroupLayout) {
    // Create a binding
    std::vector<BindGroupEntry> bindings(2);

    bindings[0].binding = 0;
    bindings[0].textureView = texView;

    bindings[1].binding = 1;
    bindings[1].buffer = transformsBuffer;
    bindings[1].offset = 0;
    bindings[1].size = sizeof(ObjectTransforms);

    BindGroupDescriptor bindGroupDesc;
    bindGroupDesc.layout = bindGroupLayout;
    bindGroupDesc.entryCount = bindings.size();
    bindGroupDesc.entries = bindings.data();
    bindGroup = device.createBindGroup(bindGroupDesc);
}

void Mesh::Terminate() {
    vertexBuffer.release();
    transformsBuffer.release();
    bindGroup.release();
    texture.destroy();
    texture.release();
    texView.release();
}
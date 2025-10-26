#pragma once
#include <vector>
#include <glm/glm.hpp>
#include <array>
#include <filesystem>
#include "Helpers.hpp"

using namespace wgpu;

struct VertexAttributes {
    std::array<float,3> position = {0.0f,0.0f,0.0f};
    std::array<float,3> normal={0.0f,0.0f,0.0f};
    std::array<float,3> color={0.0f,0.0f,0.0f};
    std::array<float,2> texCoords={0.0f,0.0f};
};

class Mesh{
public:
    BindGroup bindGroup;
    Texture texture=nullptr, normalTexture=nullptr;
    TextureView texView=nullptr, normalTexView=nullptr;
    Buffer vertexBuffer, indexBuffer, transformsBuffer;
    uint32_t vertexCount, indexCount;
    std::vector<VertexAttributes> vertexData;
    std::vector<uint32_t> indexData;
    ObjectTransforms localTransforms, globalTransforms;
    RenderPipeline pipeline;
    PipelineLayout pipelineLayout;
    TextureFormat surfaceFormat=TextureFormat::Undefined;
    std::filesystem::path texturePath = "";
    std::filesystem::path normalMapPath = "";

    //Mesh();
    //Mesh(Device device, Queue queue, BindGroupLayout bindGroupLayout, const std::filesystem::path& path, Mesh* parent=nullptr);
    void SetTransforms(glm::vec3 scale=glm::vec3(1.0f,1.0f,1.0f), glm::vec3 translate=glm::vec3(1.0f,1.0f,1.0f), glm::vec3 rotate=glm::vec3(0.0f,0.0f,0.0f));
    void UpdateTransforms();
    void InitializeNormalMap(const std::filesystem::path& path);
    void InitializeTexture(const std::filesystem::path& path);
    Texture LoadTexture(const std::filesystem::path& path, TextureView* pTextureView, void *data = nullptr);
    Mesh* GetParent();
    std::vector<Mesh*> GetChildren();
    void AddChild(Mesh* child);
    void SetParent(Mesh* parent);
    void SetGpu(Device device, BindGroupLayout bindGroupLayout, TextureFormat surfaceFormat, TextureFormat depthTextureFormat);
    void Terminate();

    private:
    Queue queue;
    Device device;
    BindGroupLayout bindGroupLayout;
    BindGroupLayout meshBindGroupLayout;
    TextureView depthTextureView;
    Texture depthTexture;
    TextureFormat depthTextureFormat=TextureFormat::Undefined;
    std::vector<BindGroupLayout> bindGroupLayouts;
    Mesh* parent=nullptr;
    std::vector<Mesh*> children;
    void InitializeBuffers();
    void InitializeBinding();
    void InitializePipeline();
};
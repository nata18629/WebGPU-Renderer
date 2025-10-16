#include <vector>
#include <glm/glm.hpp>
#include "Helpers.hpp"
#include "ResourceManager.hpp"

using namespace wgpu;

class Mesh{
public:
    BindGroup bindGroup;
    Texture texture, normalTexture;
    TextureView texView, normalTexView;
    Buffer vertexBuffer;
    uint32_t vertexCount;
    std::vector<VertexAttributes> vertexData;
    ObjectTransforms localTransforms, globalTransforms;
    Buffer transformsBuffer;

    Mesh(Device device, Queue queue, BindGroupLayout bindGroupLayout, const std::filesystem::path& path, Mesh* parent=nullptr);
    void SetTransforms(glm::vec3 scale=glm::vec3(1.0f,1.0f,1.0f), glm::vec3 translate=glm::vec3(1.0f,1.0f,1.0f), glm::vec3 rotate=glm::vec3(0.0f,0.0f,0.0f));
    void UpdateTransforms();
    Mesh* GetParent();
    std::vector<Mesh*> GetChildren();
    void AddChild(Mesh* child);
    void Terminate();
private:
    Queue queue;
    Device device;
    Mesh* parent;
    std::vector<Mesh*> children;
    void InitializeTexture();
    void InitializeBuffers(const std::filesystem::path& path);
    void InitializeBinding(BindGroupLayout bindGroupLayout);
};
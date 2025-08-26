#include <vector>
#include <glm/glm.hpp>
#include "Helpers.hpp"
#include "ResourceManager.hpp"

using namespace wgpu;

class Mesh{
public:
    BindGroup bindGroup;
    Texture texture;
    TextureView texView;
    Buffer vertexBuffer;
    uint32_t vertexCount;
    std::vector<VertexAttributes> vertexData;
    ObjectTransforms transforms;
    Buffer transformsBuffer;

    Mesh(Device device, Queue queue, BindGroupLayout bindGroupLayout, const std::filesystem::path& path);
    void SetTransforms(glm::vec3 scale, glm::vec3 translate, glm::vec3 rotate);
    void Terminate();
private:
    Queue queue;
    Device device;
    void InitializeTexture();
    void InitializeBuffers(const std::filesystem::path& path);
    void InitializeBinding(BindGroupLayout bindGroupLayout);
};
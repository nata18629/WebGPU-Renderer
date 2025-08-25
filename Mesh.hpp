#include <vector>
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

    Mesh(Device device, Queue queue, BindGroupLayout bindGroupLayout, const std::filesystem::path& path);
    void Terminate();
private:
    void InitializeTexture(Device device);
    void InitializeBuffers(Device device, Queue queue, const std::filesystem::path& path);
    void InitializeBinding(Device device, BindGroupLayout bindGroupLayout);
};
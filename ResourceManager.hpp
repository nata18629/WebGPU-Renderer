#include <webgpu/webgpu.hpp>
#include <vector>
#include <filesystem>

namespace fs = std::filesystem;

class ResourceManager {
    public:
    static wgpu::ShaderModule loadShaderModule(const fs::path& path, wgpu::Device device);

    private:

};
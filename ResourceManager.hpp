#pragma once
#include <webgpu/webgpu.hpp>
#include <filesystem>

namespace fs = std::filesystem;

// struct VertexAttributes {
//     std::vector<float> position;
//     std::vector<float> normal;
//     std::vector<float> color;
// };


class ResourceManager {
    public:
    static wgpu::ShaderModule loadShaderModule(const fs::path& path, wgpu::Device device);
    //static bool loadGeometryObj(const fs::path& path, std::vector<VertexAttributes>& vertexData);

    private:

};
#pragma once
#include <webgpu/webgpu.hpp>
#include <filesystem>
#include <array>

namespace fs = std::filesystem;

struct VertexAttributes {
    std::array<float,3> position;
    std::array<float,3> normal;
    std::array<float,3> color;
};


class ResourceManager {
    public:
    static wgpu::ShaderModule loadShaderModule(const fs::path& path, wgpu::Device device);
    static bool loadGeometryObj(const fs::path& path, std::vector<VertexAttributes>& vertexData);

    private:

};
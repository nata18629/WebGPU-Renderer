#pragma once
#include <webgpu/webgpu.hpp>
#include <vector>
#include <filesystem>
#include "Mesh.hpp"

namespace fs = std::filesystem;

class ResourceManager {
    public:
    static wgpu::ShaderModule loadShaderModule(const fs::path& path, wgpu::Device device);
    static bool loadGeometryObj(const fs::path& path, std::vector<Mesh>& meshes);
    static bool loadGeometryGltf(const fs::path& path, std::vector<Mesh>& meshes);

    private:

};
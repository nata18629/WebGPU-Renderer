#define TINYOBJLOADER_IMPLEMENTATION
#include "tiny_obj_loader.h"
#include <fstream>
#include <sstream>
#include <vector>
#include <string>
#include "ResourceManager.hpp"

using namespace wgpu;

ShaderModule ResourceManager::loadShaderModule(const std::filesystem::path& path, Device device) {
    std::ifstream file(path);
    if (!file.is_open()) {
        return nullptr;
    }
    file.seekg(0, std::ios::end);
    size_t size = file.tellg();
    std::string shaderSource(size, ' ');
    file.seekg(0);
    file.read(shaderSource.data(), size);

    ShaderModuleWGSLDescriptor shaderCodeDesc{};
    shaderCodeDesc.chain.next = nullptr;
    shaderCodeDesc.chain.sType = SType::ShaderModuleWGSLDescriptor;
    shaderCodeDesc.code = shaderSource.c_str();

    ShaderModuleDescriptor shaderDesc{};
    shaderDesc.nextInChain = &shaderCodeDesc.chain;
    return device.createShaderModule(shaderDesc);
}

bool ResourceManager::loadGeometryObj(const fs::path& path, std::vector<VertexAttributes>& vertexData){
    tinyobj::attrib_t attrib;
    std::vector<tinyobj::shape_t> shapes;
    std::vector<tinyobj::material_t> materials;

    std::string warn;
    std::string err;

    bool ret = tinyobj::LoadObj(&attrib, &shapes, &materials, &warn, &err, path.string().c_str());

    if (!warn.empty()) {
        std::cout << warn << std::endl;
    }

    if (!err.empty()) {
        std::cerr << err << std::endl;
    }

    if (!ret) {
        return false;
    }

    const auto& shape = shapes[0];
    vertexData.resize(shape.mesh.indices.size());
    for (size_t i = 0; i < shape.mesh.indices.size(); ++i) {
        const tinyobj::index_t& idx = shape.mesh.indices[i];

        vertexData[i].position = {
            attrib.vertices[3 * idx.vertex_index + 0],
            attrib.vertices[3 * idx.vertex_index + 1],
            attrib.vertices[3 * idx.vertex_index + 2]
        };

        vertexData[i].normal = {
            attrib.normals[3 * idx.normal_index + 0],
            attrib.normals[3 * idx.normal_index + 1],
            attrib.normals[3 * idx.normal_index + 2]
        };

        vertexData[i].color = {
            attrib.colors[3 * idx.vertex_index + 0],
            attrib.colors[3 * idx.vertex_index + 1],
            attrib.colors[3 * idx.vertex_index + 2]
        };

        vertexData[i].texCoords = {
            attrib.texcoords[2 * idx.texcoord_index + 0],
            1-attrib.texcoords[2 * idx.texcoord_index + 1]
        };
    }

    return true;
}
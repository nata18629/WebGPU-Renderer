#define TINYGLTF_IMPLEMENTATION
#include "tiny_gltf.h"
#define STB_IMAGE_IMPLEMENTATION
#include "stb_image.h"
#define STB_IMAGE_WRITE_IMPLEMENTATION
#include "stb_image_write.h"
#define TINYOBJLOADER_IMPLEMENTATION
#include "tiny_obj_loader.h"
#include <fstream>
#include <sstream>
#include <vector>
#include <string>
#include <iostream>
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

bool ResourceManager::loadGeometryObj(const fs::path& path, std::vector<Mesh>& meshes){
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
    Mesh mesh;
    const auto& shape = shapes[0];
    mesh.vertexData.resize(shape.mesh.indices.size());
    for (size_t i = 0; i < shape.mesh.indices.size(); ++i) {
        const tinyobj::index_t& idx = shape.mesh.indices[i];

        mesh.vertexData[i].position = {
            attrib.vertices[3 * idx.vertex_index + 0],
            attrib.vertices[3 * idx.vertex_index + 1],
            attrib.vertices[3 * idx.vertex_index + 2]
        };
        //printf("x:%f y:%f z:%f\n ", attrib.vertices[3 * idx.vertex_index + 0],attrib.vertices[3 * idx.vertex_index + 1],attrib.vertices[3 * idx.vertex_index + 2]);
        mesh.vertexData[i].normal = {
            attrib.normals[3 * idx.normal_index + 0],
            attrib.normals[3 * idx.normal_index + 1],
            attrib.normals[3 * idx.normal_index + 2]
        };

        mesh.vertexData[i].color = {
            attrib.colors[3 * idx.vertex_index + 0],
            attrib.colors[3 * idx.vertex_index + 1],
            attrib.colors[3 * idx.vertex_index + 2]
        };

        mesh.vertexData[i].texCoords = {
            attrib.texcoords[2 * idx.texcoord_index + 0],
            1-attrib.texcoords[2 * idx.texcoord_index + 1]
        };
    }
    meshes.push_back(mesh);
    return true;
}
// lol does not work yet
bool ResourceManager::loadGeometryGltf(const fs::path& path, std::vector<Mesh>& meshes) {
    //using namespace tinygltf;
    tinygltf::Model model;
    tinygltf::TinyGLTF loader;
    std::string err;
    std::string warn;
    bool ret;
    if (path.extension() == ".glb") {
        ret = loader.LoadBinaryFromFile(&model, &err, &warn, path.string());
    }
    else {
        ret = loader.LoadASCIIFromFile(&model, &err, &warn, path.string());
    }
    if (!warn.empty()) {
        printf("Warn: %s\n", warn.c_str());
    }
    if (!err.empty()) {
        printf("Err: %s\n", err.c_str());
    }

    for (auto mesh : model.meshes) {
        for (auto primitive : mesh.primitives) { // TODO: if a mesh has multiple primitives they should be children of a mesh
            Mesh sceneMesh;
            tinygltf::Accessor& accessor = model.accessors[primitive.attributes["POSITION"]];
            tinygltf::BufferView& bufferView = model.bufferViews[accessor.bufferView];
            tinygltf::Buffer& buffer = model.buffers[bufferView.buffer];
            float* positions = reinterpret_cast<float*>(&buffer.data[bufferView.byteOffset + accessor.byteOffset + bufferView.byteStride]);
            sceneMesh.vertexData.resize(accessor.count);
            for (size_t i = 0; i < accessor.count; i++) {
                sceneMesh.vertexData[i].position = {positions[i*3+0],positions[i*3+1],positions[i*3+2]};
            } // TODO: load indices
            meshes.push_back(sceneMesh);
            continue;
            if (primitive.attributes["NORMAL"]>=0){
                accessor = model.accessors[primitive.attributes["NORMAL"]];
                bufferView = model.bufferViews[accessor.bufferView];
                buffer = model.buffers[bufferView.buffer];
                const float* positions = reinterpret_cast<const float*>(&buffer.data[bufferView.byteOffset + accessor.byteOffset]);
                for (size_t i = 0; i < accessor.count; ++i) {
                    sceneMesh.vertexData[i].normal = {positions[i*3+0],positions[i*3+1],positions[i*3+2]};
                }
            }
            if (primitive.attributes["TEXCOORD_0"]>=0){ // it's probably done differently
                accessor = model.accessors[primitive.attributes["TEXCOORD_0"]];
                bufferView = model.bufferViews[accessor.bufferView];
                buffer = model.buffers[bufferView.buffer];
                const float* positions = reinterpret_cast<const float*>(&buffer.data[bufferView.byteOffset + accessor.byteOffset]);
                for (size_t i = 0; i < accessor.count; ++i) {
                    sceneMesh.vertexData[i].texCoords = {positions[i*3+0],positions[i*3+1]};
                }
            }
            if (primitive.attributes["COLOR_0"]>=0){
                accessor = model.accessors[primitive.attributes["COLOR_0"]];
                bufferView = model.bufferViews[accessor.bufferView];
                buffer = model.buffers[bufferView.buffer];
                const float* positions = reinterpret_cast<const float*>(&buffer.data[bufferView.byteOffset + accessor.byteOffset]);
                for (size_t i = 0; i < accessor.count; ++i) {
                    sceneMesh.vertexData[i].color = {positions[i*3+0],positions[i*3+1]};
                }
            }
            if(primitive.material<0){ continue;}
            tinygltf::Material material = model.materials[primitive.material];
            if(material.pbrMetallicRoughness.baseColorTexture.index<0){ continue;}
            tinygltf::Texture texture = model.textures[material.pbrMetallicRoughness.baseColorTexture.index];
            if(texture.source<0){ continue;}
            tinygltf::Image image = model.images[texture.source];
            sceneMesh.InitializeTexture(image.uri);
            meshes.push_back(sceneMesh);
        }
    }
    //printf("Mesh num: %I64u", meshes.size());
    return ret;
}

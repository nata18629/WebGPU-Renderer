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
    tinyobj::ObjReaderConfig reader_config;
    reader_config.mtl_search_path = fs::path{"assets/models"}.string().c_str();
    tinyobj::ObjReader reader;
    
    if (!reader.ParseFromFile(path.string().c_str(), reader_config)) {
        if (!reader.Error().empty()) {
            std::cerr << "TinyObjReader: " << reader.Error();
        }
        return false;
    }
    if (!reader.Warning().empty()) {
        std::cout << "TinyObjReader: " << reader.Warning();
    }
    auto& attrib = reader.GetAttrib();
    auto& shapes = reader.GetShapes();
    auto& materials = reader.GetMaterials();
    Mesh mesh;
    for (size_t s = 0; s < shapes.size(); s++) {
        mesh.vertexData.resize(shapes[s].mesh.indices.size());
        //std::cout<<"num: "<<shapes[s].mesh.indices.size()<<std::endl;
        // Loop over faces(polygon)
        size_t index_offset = 0;
        for (size_t f = 0; f < shapes[s].mesh.num_face_vertices.size(); f++) {
            size_t fv = size_t(shapes[s].mesh.num_face_vertices[f]);
            
            // Loop over vertices in the face.
            for (size_t i = 0; i < fv; i++) {
            // access to vertex
                tinyobj::index_t idx = shapes[s].mesh.indices[index_offset + i];
                mesh.vertexData[i+index_offset].position = {attrib.vertices[3*size_t(idx.vertex_index)+0],
                attrib.vertices[3*size_t(idx.vertex_index)+1],
                attrib.vertices[3*size_t(idx.vertex_index)+2]};
                
                // Check if `normal_index` is zero or positive. negative = no normal data
                if (idx.normal_index >= 0) {
                    mesh.vertexData[i+index_offset].normal = {attrib.normals[3*size_t(idx.normal_index)+0],
                    attrib.normals[3*size_t(idx.normal_index)+1],
                    attrib.normals[3*size_t(idx.normal_index)+2]};
                }

                // Check if `texcoord_index` is zero or positive. negative = no texcoord data
                if (idx.texcoord_index >= 0) {
                    mesh.vertexData[i+index_offset].texCoords = {attrib.texcoords[2*size_t(idx.texcoord_index)+0],
                    1-attrib.texcoords[2*size_t(idx.texcoord_index)+1]};
                }

                mesh.vertexData[i+index_offset].color = {attrib.colors[3*size_t(idx.vertex_index)+0],
                attrib.colors[3*size_t(idx.vertex_index)+1],
                attrib.colors[3*size_t(idx.vertex_index)+2]};
            }
            index_offset += fv;
            // per-face material
            //std::cout<<shapes[s].mesh.material_ids[f];
        }
        if(shapes[s].mesh.material_ids[0]>=0){
            mesh.texturePath = (materials[shapes[s].mesh.material_ids[0]].diffuse_texname);
            mesh.normalMapPath = (materials[shapes[s].mesh.material_ids[0]].bump_texname);
            std::cout << mesh.texturePath << std::endl;
        }
        meshes.push_back(mesh);
        
    }    
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
            std::cout<<"Next model.....\n";

            tinygltf::Accessor& accessor = model.accessors[primitive.attributes["POSITION"]];
            tinygltf::BufferView& bufferView = model.bufferViews[accessor.bufferView];
            tinygltf::Buffer& buffer = model.buffers[bufferView.buffer];
            float* positions = reinterpret_cast<float*>(&buffer.data[bufferView.byteOffset + accessor.byteOffset + bufferView.byteStride]);
            sceneMesh.vertexData.resize(accessor.count);
            for (size_t i = 0; i < accessor.count; i++) {
                sceneMesh.vertexData[i].position = {positions[i*3+0],positions[i*3+1],positions[i*3+2]};
            }

            accessor = model.accessors[primitive.indices];
            bufferView = model.bufferViews[accessor.bufferView];
            buffer = model.buffers[bufferView.buffer];
            unsigned short* indices = reinterpret_cast<unsigned short*>(&buffer.data[bufferView.byteOffset + accessor.byteOffset + bufferView.byteStride]);
            for (size_t i=0; i<accessor.count; i++){
                sceneMesh.indexData.push_back(indices[i]);
            }
            auto idx = primitive.attributes.find("NORMAL");
            if (idx != primitive.attributes.end()){
                accessor = model.accessors[primitive.attributes["NORMAL"]];
                std::cout<<"Count nor: "<<accessor.count<<std::endl;
                bufferView = model.bufferViews[accessor.bufferView];
                buffer = model.buffers[bufferView.buffer];
                const float* positions = reinterpret_cast<const float*>(&buffer.data[bufferView.byteOffset + accessor.byteOffset]);
                for (size_t i = 0; i < accessor.count; ++i) {
                    sceneMesh.vertexData[i].normal = {positions[i*3+0],positions[i*3+1],positions[i*3+2]};
                }
            }
            idx = primitive.attributes.find("TEXCOORD_0");
            if (idx != primitive.attributes.end()){
                accessor = model.accessors[primitive.attributes["TEXCOORD_0"]];
                std::cout<<"Count tex: "<<accessor.count<<std::endl;
                bufferView = model.bufferViews[accessor.bufferView];
                buffer = model.buffers[bufferView.buffer];
                const float* positions = reinterpret_cast<const float*>(&buffer.data[bufferView.byteOffset + accessor.byteOffset]);
                for (size_t i = 0; i < accessor.count; ++i) {
                    sceneMesh.vertexData[i].texCoords = {positions[i*3+0],positions[i*3+1]};
                }
            }
            meshes.push_back(sceneMesh);
            //continue;
            idx = primitive.attributes.find("COLOR_0");
            if (idx != primitive.attributes.end()){
                accessor = model.accessors[primitive.attributes["COLOR_0"]];
                std::cout<<"Count color: "<<accessor.count<<std::endl;
                bufferView = model.bufferViews[accessor.bufferView];
                buffer = model.buffers[bufferView.buffer];
                size_t colorStride = bufferView.byteStride > 0 ? bufferView.byteStride : 4;
                // std::cout<<accessor.componentType<<std::endl;
                // std::cout<<accessor.type<<std::endl;
                std::cout<<colorStride<<std::endl;
                const float* positions = reinterpret_cast<const float*>(&buffer.data[bufferView.byteOffset + accessor.byteOffset]);
                for (size_t i = 0; i < accessor.count; ++i) {
                    sceneMesh.vertexData[i].color = {positions[i*3+0],positions[i*3+1],positions[i*3+2]};
                    //std::cout<<positions[i*3+0]<<", "<<positions[i*3+1]<<", "<<positions[i*3+2]<<", "<<positions[i*3+3]<<std::endl;
                }
            }
            if(primitive.material<0){ continue;}
            tinygltf::Material material = model.materials[primitive.material];
            if(material.pbrMetallicRoughness.baseColorTexture.index<0){ continue;}
            tinygltf::Texture texture = model.textures[material.pbrMetallicRoughness.baseColorTexture.index];
            if(texture.source<0){ continue;}
            tinygltf::Image image = model.images[texture.source];
            std::cout << image.uri<<std::endl;
            //sceneMesh.InitializeTexture(image.uri);
            //meshes.push_back(sceneMesh);
        }
    }
    //printf("Mesh num: %I64u", meshes.size());
    return ret;
}

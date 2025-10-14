cmake_minimum_required(VERSION 3.0...3.25)

add_executable(App main.cpp Renderer.cpp Renderer.hpp
    ResourceManager.cpp ResourceManager.hpp
    Helpers.hpp Mesh.cpp Mesh.hpp Camera.hpp Camera.cpp MainWindow.hpp MainWindow.cpp Gpu.hpp Gpu.cpp
)
find_package(glm CONFIG REQUIRED)
target_include_directories(App PRIVATE 3rdparty/)
target_link_libraries(App PUBLIC glfw webgpu glfw3webgpu glm::glm)
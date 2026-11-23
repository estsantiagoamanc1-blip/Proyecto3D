#pragma once
#include <GL/glew.h>
#include <glm/glm.hpp>
#include <string>
#include <vector>
#include "Shader.h"

namespace tinygltf { class Model; }

struct Primitive {
    GLuint vao = 0, vbo = 0, ebo = 0;
    GLsizei count = 0;
    glm::mat4 local{1.0f};          // transformacion del nodo glTF
    glm::vec3 color{0.8f, 0.8f, 0.8f}; // baseColorFactor del material
};

// Carga archivos .gltf / .glb con tinygltf y los dibuja con OpenGL
class Model {
public:
    ~Model() { destroy(); }
    bool load(const std::string& path, std::string& message);
    void draw(const Shader& shader, const glm::mat4& modelMatrix, const glm::vec3* overrideColor = nullptr) const;
    void destroy();
    size_t primitiveCount() const { return prims.size(); }
    // Caja envolvente del modelo (coordenadas del modelo, con las transformaciones de nodo aplicadas)
    glm::vec3 boundsMin() const { return bMin; }
    glm::vec3 boundsMax() const { return bMax; }
private:
    std::vector<Primitive> prims;
    glm::vec3 bMin{ 1e30f, 1e30f, 1e30f };
    glm::vec3 bMax{ -1e30f, -1e30f, -1e30f };
    void processNode(const tinygltf::Model& m, int nodeIndex, const glm::mat4& parent);
    void createPrimitive(const tinygltf::Model& m, int meshIndex, const glm::mat4& transform);
};

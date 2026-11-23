#define TINYGLTF_IMPLEMENTATION
#define STB_IMAGE_IMPLEMENTATION
#define STB_IMAGE_WRITE_IMPLEMENTATION
#include <tiny_gltf.h>

#include "Model.h"
#include <glm/gtc/type_ptr.hpp>
#include <glm/gtc/quaternion.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtc/matrix_inverse.hpp>

// Devuelve puntero al primer elemento del accessor y el stride en bytes
static const unsigned char* accessorData(const tinygltf::Model& m, const tinygltf::Accessor& a, size_t& stride) {
    const tinygltf::BufferView& bv = m.bufferViews[a.bufferView];
    const tinygltf::Buffer& buf = m.buffers[bv.buffer];
    int s = a.ByteStride(bv);
    stride = (s > 0) ? (size_t)s : (size_t)tinygltf::GetComponentSizeInBytes(a.componentType) * tinygltf::GetNumComponentsInType(a.type);
    return buf.data.data() + bv.byteOffset + a.byteOffset;
}

bool Model::load(const std::string& path, std::string& message) {
    destroy();
    tinygltf::Model gltf;
    tinygltf::TinyGLTF loader;
    std::string err, warn;
    bool ok = false;
    bool isGlb = path.size() > 4 && path.substr(path.size() - 4) == ".glb";
    ok = isGlb ? loader.LoadBinaryFromFile(&gltf, &err, &warn, path)
               : loader.LoadASCIIFromFile(&gltf, &err, &warn, path);
    if (!ok) { message = "Error cargando glTF: " + err; return false; }

    int sceneIdx = gltf.defaultScene >= 0 ? gltf.defaultScene : 0;
    if (gltf.scenes.empty()) { message = "El archivo no tiene escenas."; return false; }
    for (int n : gltf.scenes[sceneIdx].nodes) processNode(gltf, n, glm::mat4(1.0f));

    message = "Cargado: " + path + " (" + std::to_string(prims.size()) + " primitivas)";
    return !prims.empty();
}

void Model::processNode(const tinygltf::Model& m, int nodeIndex, const glm::mat4& parent) {
    const tinygltf::Node& node = m.nodes[nodeIndex];
    glm::mat4 local(1.0f);
    if (node.matrix.size() == 16) {
        for (int i = 0; i < 16; ++i) glm::value_ptr(local)[i] = (float)node.matrix[i];
    } else {
        glm::vec3 t(0.0f), s(1.0f);
        glm::quat q(1, 0, 0, 0);
        if (node.translation.size() == 3) t = glm::vec3((float)node.translation[0], (float)node.translation[1], (float)node.translation[2]);
        if (node.rotation.size() == 4)    q = glm::quat((float)node.rotation[3], (float)node.rotation[0], (float)node.rotation[1], (float)node.rotation[2]);
        if (node.scale.size() == 3)       s = glm::vec3((float)node.scale[0], (float)node.scale[1], (float)node.scale[2]);
        local = glm::translate(glm::mat4(1.0f), t) * glm::mat4_cast(q) * glm::scale(glm::mat4(1.0f), s);
    }
    glm::mat4 world = parent * local;
    if (node.mesh >= 0) createPrimitive(m, node.mesh, world);
    for (int c : node.children) processNode(m, c, world);
}

void Model::createPrimitive(const tinygltf::Model& m, int meshIndex, const glm::mat4& transform) {
    const tinygltf::Mesh& mesh = m.meshes[meshIndex];
    for (const tinygltf::Primitive& p : mesh.primitives) {
        if (p.mode != TINYGLTF_MODE_TRIANGLES && p.mode != -1) continue;
        auto posIt = p.attributes.find("POSITION");
        if (posIt == p.attributes.end()) continue;

        const tinygltf::Accessor& posAcc = m.accessors[posIt->second];
        size_t posStride; const unsigned char* posData = accessorData(m, posAcc, posStride);

        const unsigned char* nrmData = nullptr; size_t nrmStride = 0;
        auto nIt = p.attributes.find("NORMAL");
        if (nIt != p.attributes.end()) nrmData = accessorData(m, m.accessors[nIt->second], nrmStride);

        // Vertices intercalados: pos(3) + normal(3)
        std::vector<float> vertices;
        vertices.reserve(posAcc.count * 6);
        for (size_t i = 0; i < posAcc.count; ++i) {
            const float* pp = reinterpret_cast<const float*>(posData + i * posStride);
            vertices.push_back(pp[0]); vertices.push_back(pp[1]); vertices.push_back(pp[2]);
            glm::vec3 wp = glm::vec3(transform * glm::vec4(pp[0], pp[1], pp[2], 1.0f));
            bMin = glm::min(bMin, wp);
            bMax = glm::max(bMax, wp);
            if (nrmData) {
                const float* nn = reinterpret_cast<const float*>(nrmData + i * nrmStride);
                vertices.push_back(nn[0]); vertices.push_back(nn[1]); vertices.push_back(nn[2]);
            } else { vertices.push_back(0.0f); vertices.push_back(1.0f); vertices.push_back(0.0f); }
        }

        // Indices (se convierten a uint32)
        std::vector<unsigned int> indices;
        if (p.indices >= 0) {
            const tinygltf::Accessor& ia = m.accessors[p.indices];
            size_t st; const unsigned char* d = accessorData(m, ia, st);
            indices.reserve(ia.count);
            for (size_t i = 0; i < ia.count; ++i) {
                const unsigned char* e = d + i * st;
                switch (ia.componentType) {
                    case TINYGLTF_COMPONENT_TYPE_UNSIGNED_BYTE:  indices.push_back(*e); break;
                    case TINYGLTF_COMPONENT_TYPE_UNSIGNED_SHORT: indices.push_back(*reinterpret_cast<const unsigned short*>(e)); break;
                    case TINYGLTF_COMPONENT_TYPE_UNSIGNED_INT:   indices.push_back(*reinterpret_cast<const unsigned int*>(e)); break;
                }
            }
        } else {
            for (size_t i = 0; i < posAcc.count; ++i) indices.push_back((unsigned int)i);
        }

        Primitive prim;
        prim.local = transform;
        prim.count = (GLsizei)indices.size();
        if (p.material >= 0) {
            const auto& f = m.materials[p.material].pbrMetallicRoughness.baseColorFactor;
            if (f.size() >= 3) prim.color = glm::vec3((float)f[0], (float)f[1], (float)f[2]);
        }

        glGenVertexArrays(1, &prim.vao);
        glGenBuffers(1, &prim.vbo);
        glGenBuffers(1, &prim.ebo);
        glBindVertexArray(prim.vao);
        glBindBuffer(GL_ARRAY_BUFFER, prim.vbo);
        glBufferData(GL_ARRAY_BUFFER, vertices.size() * sizeof(float), vertices.data(), GL_STATIC_DRAW);
        glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, prim.ebo);
        glBufferData(GL_ELEMENT_ARRAY_BUFFER, indices.size() * sizeof(unsigned int), indices.data(), GL_STATIC_DRAW);
        glEnableVertexAttribArray(0);
        glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 6 * sizeof(float), (void*)0);
        glEnableVertexAttribArray(1);
        glVertexAttribPointer(1, 3, GL_FLOAT, GL_FALSE, 6 * sizeof(float), (void*)(3 * sizeof(float)));
        glBindVertexArray(0);
        prims.push_back(prim);
    }
}

void Model::draw(const Shader& shader, const glm::mat4& modelMatrix, const glm::vec3* overrideColor) const {
    for (const Primitive& p : prims) {
        glm::mat4 M = modelMatrix * p.local;
        shader.setMat4("uModel", M);
        shader.setMat3("uNormalMat", glm::inverseTranspose(glm::mat3(M)));
        shader.setVec3("uObjColor", overrideColor ? *overrideColor : p.color);
        glBindVertexArray(p.vao);
        glDrawElements(GL_TRIANGLES, p.count, GL_UNSIGNED_INT, nullptr);
    }
    glBindVertexArray(0);
}

void Model::destroy() {
    for (Primitive& p : prims) {
        glDeleteBuffers(1, &p.vbo); glDeleteBuffers(1, &p.ebo); glDeleteVertexArrays(1, &p.vao);
    }
    prims.clear();
    bMin = glm::vec3(1e30f);
    bMax = glm::vec3(-1e30f);
}

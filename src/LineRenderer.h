#pragma once
#include "Shader.h"
#include "Shaders.h"
#include <vector>

// Dibuja polilineas y puntos (curva de Bezier, puntos de control, luz)
class LineRenderer {
public:
    LineRenderer() : shader(LINE_VS, LINE_FS) {
        glGenVertexArrays(1, &vao);
        glGenBuffers(1, &vbo);
        glBindVertexArray(vao);
        glBindBuffer(GL_ARRAY_BUFFER, vbo);
        glEnableVertexAttribArray(0);
        glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, sizeof(glm::vec3), (void*)0);
        glBindVertexArray(0);
    }
    ~LineRenderer() { glDeleteBuffers(1, &vbo); glDeleteVertexArrays(1, &vao); }

    void draw(const std::vector<glm::vec3>& pts, const glm::mat4& mvp, const glm::vec3& color,
              GLenum mode, float pointSize = 1.0f) {
        if (pts.empty()) return;
        shader.use();
        shader.setMat4("uMVP", mvp);
        shader.setVec3("uColor", color);
        shader.setFloat("uPointSize", pointSize);
        glBindVertexArray(vao);
        glBindBuffer(GL_ARRAY_BUFFER, vbo);
        glBufferData(GL_ARRAY_BUFFER, pts.size() * sizeof(glm::vec3), pts.data(), GL_DYNAMIC_DRAW);
        glDrawArrays(mode, 0, (GLsizei)pts.size());
        glBindVertexArray(0);
    }
private:
    Shader shader;
    GLuint vao = 0, vbo = 0;
};

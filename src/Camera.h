#pragma once
#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <cmath>

// Camara con matrices explicitas de TRASLACION y ROTACION.
// View = Rotacion^-1 * Traslacion^-1  (inversa de la transformacion de la camara en el mundo)
struct Camera {
    glm::vec3 position{0.0f, 1.5f, 6.0f};
    float yaw = 0.0f;     // radianes, alrededor de Y
    float pitch = -0.2f;  // radianes, alrededor de X
    float fov = 45.0f;    // grados

    // Matriz de traslacion de la camara: mueve el mundo por -position
    glm::mat4 translationMatrix() const {
        return glm::translate(glm::mat4(1.0f), -position);
    }
    // Matriz de rotacion de la camara (inversa de Ry(yaw)*Rx(pitch))
    glm::mat4 rotationMatrix() const {
        glm::mat4 rx = glm::rotate(glm::mat4(1.0f), -pitch, glm::vec3(1, 0, 0));
        glm::mat4 ry = glm::rotate(glm::mat4(1.0f), -yaw, glm::vec3(0, 1, 0));
        return rx * ry;
    }
    glm::mat4 view() const { return rotationMatrix() * translationMatrix(); }
    glm::mat4 projection(float aspect) const {
        return glm::perspective(glm::radians(fov), aspect, 0.1f, 200.0f);
    }

    glm::vec3 forward() const {
        return glm::vec3(-std::sin(yaw) * std::cos(pitch), std::sin(pitch), -std::cos(yaw) * std::cos(pitch));
    }
    glm::vec3 right() const { return glm::vec3(std::cos(yaw), 0.0f, -std::sin(yaw)); }
    glm::vec3 up() const { return glm::vec3(0.0f, 1.0f, 0.0f); }
};

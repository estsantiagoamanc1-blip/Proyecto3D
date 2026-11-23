#pragma once
#include <glm/glm.hpp>
#include <vector>

// Curva de Bezier cubica: B(t) = (1-t)^3 P0 + 3(1-t)^2 t P1 + 3(1-t) t^2 P2 + t^3 P3
struct BezierCurve {
    glm::vec3 p[4] = {
        { -4.0f, 0.0f,  0.0f },
        { -2.0f, 4.0f,  3.0f },
        {  2.0f, -2.0f, -3.0f },
        {  4.0f, 1.0f,  0.0f }
    };

    glm::vec3 evaluate(float t) const {
        float u = 1.0f - t;
        return u * u * u * p[0] + 3.0f * u * u * t * p[1] + 3.0f * u * t * t * p[2] + t * t * t * p[3];
    }
    glm::vec3 tangent(float t) const {
        float u = 1.0f - t;
        return 3.0f * u * u * (p[1] - p[0]) + 6.0f * u * t * (p[2] - p[1]) + 3.0f * t * t * (p[3] - p[2]);
    }
    std::vector<glm::vec3> sample(int n = 100) const {
        std::vector<glm::vec3> pts;
        pts.reserve(n + 1);
        for (int i = 0; i <= n; ++i) pts.push_back(evaluate(float(i) / float(n)));
        return pts;
    }
};

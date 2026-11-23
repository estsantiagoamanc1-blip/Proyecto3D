// Proyecto3D - Visor OpenGL: glTF/GLB + Phong + camara + Bezier + Dear ImGui (teclado)
#ifndef NOMINMAX
#define NOMINMAX
#endif
#include <GL/glew.h>
#include <GLFW/glfw3.h>

#include <imgui.h>
#include <imgui_impl_glfw.h>
#include <imgui_impl_opengl3.h>

#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>

#include <algorithm>
#include <cmath>
#include <iostream>
#include <string>
#include <vector>

#include "Bezier.h"
#include "Camera.h"
#include "LineRenderer.h"
#include "Model.h"
#include "Shader.h"
#include "Shaders.h"

// =====================================================================
//  CAMBIA AQUI LA RUTA DE TU MODELO (.glb o .gltf, relativa a la carpeta del proyecto)
// =====================================================================
static const char* MODEL_PATH = "assets/modelo.glb";

// ---------------------------------------------------------------- Estado
struct ObjectState {
    glm::vec3 position{0.0f};
    glm::vec3 rotationDeg{0.0f};
    float scale = 1.0f;
    glm::vec3 color{0.9f, 0.4f, 0.2f};
    bool useCustomColor = false;     // false = usa el color del material del modelo
    bool autoRotate = true;
    float autoRotateSpeed = 30.0f;

    // Rotacion de objeto: Rz * Ry * Rx
    glm::mat4 rotationMatrix() const {
        glm::mat4 rx = glm::rotate(glm::mat4(1.0f), glm::radians(rotationDeg.x), glm::vec3(1, 0, 0));
        glm::mat4 ry = glm::rotate(glm::mat4(1.0f), glm::radians(rotationDeg.y), glm::vec3(0, 1, 0));
        glm::mat4 rz = glm::rotate(glm::mat4(1.0f), glm::radians(rotationDeg.z), glm::vec3(0, 0, 1));
        return rz * ry * rx;
    }
};

struct LightState {
    glm::vec3 position{3.0f, 5.0f, 4.0f};
    glm::vec3 color{1.0f, 1.0f, 1.0f};
    float ka = 0.20f, kd = 0.80f, ks = 0.50f, shininess = 32.0f;
};

struct BezierState {
    BezierCurve curve;
    bool follow = false;
    bool animate = true;
    float t = 0.0f;
    float speed = 0.2f;
    int direction = 1;
};

// Camara inicial: mira al objeto (que siempre se normaliza a ~2 unidades de tamano)
static Camera defaultCamera() {
    Camera c;
    c.position = glm::vec3(0.0f, 0.0f, 4.0f);
    c.yaw = 0.0f;
    c.pitch = 0.0f;
    return c;
}

// Matriz que centra el modelo en el origen y lo escala para que mida ~2 unidades
static glm::mat4 fitMatrix(const Model& m) {
    glm::vec3 mn = m.boundsMin(), mx = m.boundsMax();
    if (mn.x > mx.x) return glm::mat4(1.0f);
    glm::vec3 center = (mn + mx) * 0.5f;
    glm::vec3 size = mx - mn;
    float maxDim = std::max(size.x, std::max(size.y, size.z));
    float k = maxDim > 0.0f ? 2.0f / maxDim : 1.0f;
    return glm::scale(glm::mat4(1.0f), glm::vec3(k)) * glm::translate(glm::mat4(1.0f), -center);
}

// Arrastrar y soltar un .glb/.gltf sobre la ventana para cargarlo
static std::string g_droppedPath;
static void dropCallback(GLFWwindow*, int count, const char** paths) {
    if (count > 0) g_droppedPath = paths[0];
}

// ---------------------------------------------------------------- Teclado (Dear ImGui)
static void processKeyboard(float dt, Camera& cam, ObjectState& obj, BezierState& bez,
                            bool& showPanel, bool& showCurve) {
    ImGuiIO& io = ImGui::GetIO();
    if (io.WantTextInput) return;

    const float moveSpeed = 3.0f, lookSpeed = 1.5f, rotSpeed = 90.0f;

    // Traslacion de la camara
    if (ImGui::IsKeyDown(ImGuiKey_W)) cam.position += cam.forward() * moveSpeed * dt;
    if (ImGui::IsKeyDown(ImGuiKey_S)) cam.position -= cam.forward() * moveSpeed * dt;
    if (ImGui::IsKeyDown(ImGuiKey_A)) cam.position -= cam.right()   * moveSpeed * dt;
    if (ImGui::IsKeyDown(ImGuiKey_D)) cam.position += cam.right()   * moveSpeed * dt;
    if (ImGui::IsKeyDown(ImGuiKey_E)) cam.position += cam.up()      * moveSpeed * dt;
    if (ImGui::IsKeyDown(ImGuiKey_Q)) cam.position -= cam.up()      * moveSpeed * dt;

    // Rotacion de la camara
    if (ImGui::IsKeyDown(ImGuiKey_J)) cam.yaw   += lookSpeed * dt;
    if (ImGui::IsKeyDown(ImGuiKey_L)) cam.yaw   -= lookSpeed * dt;
    if (ImGui::IsKeyDown(ImGuiKey_I)) cam.pitch += lookSpeed * dt;
    if (ImGui::IsKeyDown(ImGuiKey_K)) cam.pitch -= lookSpeed * dt;
    cam.pitch = glm::clamp(cam.pitch, -1.5f, 1.5f);

    // Rotacion del objeto
    if (ImGui::IsKeyDown(ImGuiKey_LeftArrow))  obj.rotationDeg.y -= rotSpeed * dt;
    if (ImGui::IsKeyDown(ImGuiKey_RightArrow)) obj.rotationDeg.y += rotSpeed * dt;
    if (ImGui::IsKeyDown(ImGuiKey_UpArrow))    obj.rotationDeg.x -= rotSpeed * dt;
    if (ImGui::IsKeyDown(ImGuiKey_DownArrow))  obj.rotationDeg.x += rotSpeed * dt;
    if (ImGui::IsKeyDown(ImGuiKey_Z))          obj.rotationDeg.z -= rotSpeed * dt;
    if (ImGui::IsKeyDown(ImGuiKey_X))          obj.rotationDeg.z += rotSpeed * dt;

    // Interruptores
    if (ImGui::IsKeyPressed(ImGuiKey_Space, false)) obj.autoRotate = !obj.autoRotate;
    if (ImGui::IsKeyPressed(ImGuiKey_F1, false))    showPanel = !showPanel;   // panel de controles
    if (ImGui::IsKeyPressed(ImGuiKey_F2, false))    showCurve = !showCurve;   // curva de Bezier
    if (ImGui::IsKeyPressed(ImGuiKey_B, false))     bez.follow = !bez.follow; // objeto sigue la curva
    if (ImGui::IsKeyPressed(ImGuiKey_R, false)) {
        cam = defaultCamera();
        obj.rotationDeg = glm::vec3(0.0f);
        bez.t = 0.0f;
    }
}

// ---------------------------------------------------------------- Panel opcional (F1)
static void drawUI(Camera& cam, ObjectState& obj, LightState& light, BezierState& bez, bool& showCurve) {
    ImGui::SetNextWindowPos(ImVec2(10, 10), ImGuiCond_FirstUseEver);
    ImGui::SetNextWindowSize(ImVec2(340, 520), ImGuiCond_FirstUseEver);
    ImGui::Begin("Controles (F1 para ocultar)");

    if (ImGui::CollapsingHeader("Camara", ImGuiTreeNodeFlags_DefaultOpen)) {
        ImGui::DragFloat3("Posicion", &cam.position.x, 0.05f);
        ImGui::SliderAngle("Yaw", &cam.yaw, -180.0f, 180.0f);
        ImGui::SliderAngle("Pitch", &cam.pitch, -85.0f, 85.0f);
        ImGui::SliderFloat("FOV", &cam.fov, 20.0f, 90.0f);
    }
    if (ImGui::CollapsingHeader("Objeto", ImGuiTreeNodeFlags_DefaultOpen)) {
        ImGui::SliderFloat3("Rotacion", &obj.rotationDeg.x, -180.0f, 180.0f);
        ImGui::SliderFloat("Escala", &obj.scale, 0.1f, 3.0f);
        ImGui::Checkbox("Rotar automaticamente (Espacio)", &obj.autoRotate);
        ImGui::Checkbox("Color propio", &obj.useCustomColor);
        ImGui::ColorEdit3("Color", &obj.color.x);
    }
    if (ImGui::CollapsingHeader("Iluminacion Phong")) {
        ImGui::DragFloat3("Pos. luz", &light.position.x, 0.05f);
        ImGui::SliderFloat("Ka", &light.ka, 0.0f, 1.0f);
        ImGui::SliderFloat("Kd", &light.kd, 0.0f, 1.0f);
        ImGui::SliderFloat("Ks", &light.ks, 0.0f, 1.0f);
        ImGui::SliderFloat("Brillo", &light.shininess, 1.0f, 256.0f);
    }
    if (ImGui::CollapsingHeader("Curva de Bezier")) {
        ImGui::Checkbox("Mostrar curva (F2)", &showCurve);
        ImGui::Checkbox("Objeto sigue la curva (B)", &bez.follow);
        ImGui::SliderFloat("t", &bez.t, 0.0f, 1.0f);
        ImGui::SliderFloat("Velocidad", &bez.speed, 0.0f, 1.0f);
        for (int i = 0; i < 4; ++i) {
            std::string label = "P" + std::to_string(i);
            ImGui::DragFloat3(label.c_str(), &bez.curve.p[i].x, 0.05f);
        }
    }
    ImGui::Text("%.1f FPS", ImGui::GetIO().Framerate);
    ImGui::End();
}

// ---------------------------------------------------------------- main
int main(int argc, char** argv) {
    if (!glfwInit()) { std::cerr << "No se pudo iniciar GLFW\n"; return -1; }
    glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 3);
    glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 3);
    glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE);

    GLFWwindow* window = glfwCreateWindow(1280, 720, "Proyecto3D", nullptr, nullptr);
    if (!window) { std::cerr << "No se pudo crear la ventana\n"; glfwTerminate(); return -1; }
    glfwMakeContextCurrent(window);
    glfwSwapInterval(1);
    glfwSetDropCallback(window, dropCallback);

    glewExperimental = GL_TRUE;
    if (glewInit() != GLEW_OK) { std::cerr << "No se pudo iniciar GLEW\n"; return -1; }

    IMGUI_CHECKVERSION();
    ImGui::CreateContext();
    ImGui::StyleColorsDark();
    ImGui_ImplGlfw_InitForOpenGL(window, true);
    ImGui_ImplOpenGL3_Init("#version 330");

    glEnable(GL_DEPTH_TEST);
    glEnable(GL_PROGRAM_POINT_SIZE);

    {
        Shader phong(PHONG_VS, PHONG_FS);
        LineRenderer lines;
        Model model;
        Camera camera = defaultCamera();
        ObjectState object;
        LightState light;
        BezierState bezier;
        bool showPanel = false;   // panel oculto por defecto (F1 lo muestra)
        bool showCurve = false;   // curva oculta por defecto (F2 la muestra)

        // Ruta del modelo: argumento de linea de comandos > MODEL_PATH > cubo de prueba
        std::string path = (argc > 1) ? argv[1] : MODEL_PATH;
        std::string status;
        if (!model.load(path, status)) {
            std::cerr << status << "\n";
            std::cerr << "Intentando cargar assets/cube.gltf...\n";
            if (!model.load("assets/cube.gltf", status)) std::cerr << status << "\n";
        }
        std::cout << status << "\n";

        double last = glfwGetTime();
        while (!glfwWindowShouldClose(window)) {
            glfwPollEvents();
            double now = glfwGetTime();
            float dt = float(now - last);
            last = now;

            if (!g_droppedPath.empty()) {
                if (model.load(g_droppedPath, status)) { object.rotationDeg = glm::vec3(0.0f); }
                std::cout << status << "\n";
                g_droppedPath.clear();
            }

            ImGui_ImplOpenGL3_NewFrame();
            ImGui_ImplGlfw_NewFrame();
            ImGui::NewFrame();

            processKeyboard(dt, camera, object, bezier, showPanel, showCurve);
            if (showPanel) drawUI(camera, object, light, bezier, showCurve);

            // Actualizacion
            if (object.autoRotate) object.rotationDeg.y += object.autoRotateSpeed * dt;
            if (bezier.animate && bezier.follow) {
                bezier.t += bezier.direction * bezier.speed * dt;
                if (bezier.t >= 1.0f) { bezier.t = 1.0f; bezier.direction = -1; }
                if (bezier.t <= 0.0f) { bezier.t = 0.0f; bezier.direction = 1; }
            }
            glm::vec3 objPos = bezier.follow ? bezier.curve.evaluate(bezier.t) : object.position;

            // Render
            int w, h;
            glfwGetFramebufferSize(window, &w, &h);
            glViewport(0, 0, w, h);
            glClearColor(0.08f, 0.09f, 0.11f, 1.0f);
            glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

            glm::mat4 view = camera.view();
            glm::mat4 proj = camera.projection(h > 0 ? float(w) / float(h) : 1.0f);

            // Modelo = T(pos) * Rotacion * Escala * Ajuste(centrar y normalizar tamano)
            glm::mat4 modelMatrix = glm::translate(glm::mat4(1.0f), objPos)
                                  * object.rotationMatrix()
                                  * glm::scale(glm::mat4(1.0f), glm::vec3(object.scale))
                                  * fitMatrix(model);

            phong.use();
            phong.setMat4("uView", view);
            phong.setMat4("uProj", proj);
            phong.setVec3("uLightPos", light.position);
            phong.setVec3("uLightColor", light.color);
            phong.setVec3("uViewPos", camera.position);
            phong.setFloat("uKa", light.ka);
            phong.setFloat("uKd", light.kd);
            phong.setFloat("uKs", light.ks);
            phong.setFloat("uShininess", light.shininess);
            model.draw(phong, modelMatrix, object.useCustomColor ? &object.color : nullptr);

            if (showCurve) {
                glm::mat4 vp = proj * view;
                lines.draw(bezier.curve.sample(100), vp, glm::vec3(0.2f, 0.9f, 0.4f), GL_LINE_STRIP);
                std::vector<glm::vec3> ctrl(bezier.curve.p, bezier.curve.p + 4);
                lines.draw(ctrl, vp, glm::vec3(0.5f, 0.5f, 0.5f), GL_LINE_STRIP);
                lines.draw(ctrl, vp, glm::vec3(1.0f, 0.8f, 0.1f), GL_POINTS, 9.0f);
            }

            ImGui::Render();
            ImGui_ImplOpenGL3_RenderDrawData(ImGui::GetDrawData());
            glfwSwapBuffers(window);
        }
    }

    ImGui_ImplOpenGL3_Shutdown();
    ImGui_ImplGlfw_Shutdown();
    ImGui::DestroyContext();
    glfwDestroyWindow(window);
    glfwTerminate();
    return 0;
}

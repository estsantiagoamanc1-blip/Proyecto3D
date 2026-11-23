# Proyecto3D – Visor 3D con glTF, Phong, Bézier y Dear ImGui

## 1. Datos del proyecto

| Campo | Valor |
|---|---|
| **Nombre del proyecto** | Proyecto3D – Visor 3D interactivo |
| **Estudiante** | _(escribe aquí tu nombre completo)_ |
| **Código** | _(escribe aquí tu código estudiantil)_ |
| **Curso / Docente** | _(opcional)_ |
| **IDE** | Visual Studio 2019 (toolset v142), C++17, x64 |

## 2. Descripción

Aplicación en C++ y OpenGL 3.3 que muestra un modelo glTF iluminado con el modelo de Phong. La cámara y el objeto se manipulan con teclado (a través de Dear ImGui) y con paneles de control. El objeto puede recorrer una curva de Bézier cúbica editable.

## 3. Requisitos implementados

| Requisito | Dónde está | Descripción |
|---|---|---|
| Matrices de **traslación de la cámara** | `src/Camera.h` → `translationMatrix()`, `rotationMatrix()`, `view()` | `View = R⁻¹ · T(−posición)` |
| **Rotación de objetos** | `src/main.cpp` → `ObjectState::rotationMatrix()` | `Rz · Ry · Rx` con ángulos de Euler; manual con flechas/Z/X, o automática |
| **Carga de glTF** | `src/Model.h`, `src/Model.cpp` | Usa **tinygltf**; soporta `.gltf` y `.glb`, jerarquía de nodos, normales, índices y color de material |
| **Iluminación Phong** | `src/Shaders.h` → `PHONG_VS` / `PHONG_FS` | Ambiental + difusa + especular, por fragmento |
| **Curvas de Bézier** | `src/Bezier.h` | Cúbica `B(t)=(1−t)³P0+3(1−t)²tP1+3(1−t)t²P2+t³P3`; el objeto la recorre |
| **Entradas de teclado con Dear ImGui** | `src/main.cpp` → `processKeyboard()` | Usa `ImGui::IsKeyDown` / `IsKeyPressed` |

## 4. Arquitectura del software

### 4.1 Estructura de carpetas

```
Proyecto3D/
├── Proyecto3D.sln
├── Proyecto3D.vcxproj
├── vcpkg.json              <- dependencias (modo manifiesto)
├── README.md
├── assets/
│   └── cube.gltf           <- modelo de prueba
└── src/
    ├── main.cpp            <- aplicación, bucle principal, entrada, UI
    ├── Camera.h            <- matrices de cámara
    ├── Model.h / Model.cpp <- carga y dibujo de glTF
    ├── Bezier.h            <- curva de Bézier
    ├── Shader.h            <- compilación y uniformes
    ├── Shaders.h           <- código GLSL (Phong y líneas)
    └── LineRenderer.h      <- dibujo de líneas y puntos
```

### 4.2 Capas y componentes

```
┌────────────────────────────────────────────────────┐
│ Capa de aplicación (main.cpp)                      │
│  bucle principal · estado de escena · entrada      │
└───────┬───────────────┬───────────────┬────────────┘
        │               │               │
┌───────▼──────┐ ┌──────▼───────┐ ┌─────▼────────────┐
│ Entrada / UI │ │ Escena       │ │ Renderizado      │
│ Dear ImGui   │ │ Camera       │ │ Shader (Phong)   │
│ + GLFW       │ │ Model (glTF) │ │ LineRenderer     │
│ teclado      │ │ BezierCurve  │ │ OpenGL 3.3 core  │
└──────────────┘ └──────────────┘ └──────────────────┘
```

| Componente | Responsabilidad |
|---|---|
| `main.cpp` | Crea ventana/contexto (GLFW + GLEW), inicia ImGui, ejecuta el bucle: entrada → actualización → render → UI |
| `Camera` | Posición, yaw/pitch, matrices de traslación, rotación, vista y proyección |
| `Model` | Lee glTF con tinygltf, crea VAO/VBO/EBO por primitiva y los dibuja |
| `BezierCurve` | Evalúa la curva, su tangente y la muestrea para dibujarla |
| `Shader` | Compila/enlaza GLSL y envía uniformes |
| `LineRenderer` | Dibuja la curva, puntos de control y marcador de luz |

### 4.3 Flujo de un fotograma

1. `glfwPollEvents()` y `ImGui::NewFrame()`
2. `processKeyboard()` modifica cámara, objeto y animación
3. Se actualizan rotación automática y parámetro `t` de Bézier
4. Se dibujan los paneles ImGui (parámetros editables)
5. Se renderiza el modelo con Phong y luego curva y puntos
6. `ImGui::Render()` y `glfwSwapBuffers()`

### 4.4 Tecnologías

- **OpenGL 3.3 Core** · **GLFW** (ventana) · **GLEW** (extensiones) · **GLM** (matemática)
- **tinygltf** (carga glTF) · **Dear ImGui** (interfaz y teclado)
- **vcpkg** (gestor de paquetes, modo manifiesto)

## 5. Cómo compilar y ejecutar (Visual Studio 2019)

1. Instalar [vcpkg](https://github.com/microsoft/vcpkg) y ejecutar una sola vez:
   ```
   git clone https://github.com/microsoft/vcpkg C:\vcpkg
   C:\vcpkg\bootstrap-vcpkg.bat
   C:\vcpkg\vcpkg integrate install
   ```
2. Abrir `Proyecto3D.sln` con Visual Studio 2019 (carga de trabajo *Desarrollo para el escritorio con C++*).
3. Seleccionar **x64** y **Debug** o **Release**.
4. Compilar (`Ctrl+Shift+B`). La primera vez vcpkg descargará y compilará las dependencias definidas en `vcpkg.json` (tarda unos minutos).
5. Ejecutar con `F5`. El directorio de trabajo es la carpeta del proyecto, por lo que `assets/cube.gltf` se carga automáticamente.

> Si el compilador no encuentra `imgui_impl_glfw.h`, cambia los `#include` en `main.cpp` a `<backends/imgui_impl_glfw.h>` y `<backends/imgui_impl_opengl3.h>` (depende de la versión de vcpkg).

## 6. Controles

| Tecla | Acción |
|---|---|
| `W` `A` `S` `D` | Mover la cámara (traslación) |
| `Q` / `E` | Bajar / subir la cámara |
| `J` `L` / `I` `K` | Girar la cámara (yaw / pitch) |
| Flechas | Rotar el objeto en X e Y |
| `Z` / `X` | Rotar el objeto en Z |
| `B` | Activar/desactivar que el objeto siga la curva de Bézier |
| `Espacio` | Pausar/reanudar la animación |
| `R` | Reiniciar cámara, rotación y `t` |

Desde la ventana **Controles** también se puede cargar otro `.gltf/.glb`, editar los 4 puntos de control de Bézier y ajustar Ka, Kd, Ks y el brillo de Phong.

## 7. Fundamento matemático

**Cámara:** `View = Rx(−pitch) · Ry(−yaw) · T(−pos)`

**Phong:** `I = Ka·Lc + Kd·max(N·L,0)·Lc + Ks·max(V·R,0)^n·Lc`, multiplicado por el color del objeto.

**Bézier cúbica:** `B(t)=(1−t)³P0+3(1−t)²tP1+3(1−t)t²P2+t³P3`, con `t ∈ [0,1]`.

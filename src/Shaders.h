#pragma once
// ---------- Phong (por fragmento) ----------
inline const char* PHONG_VS = R"(#version 330 core
layout(location=0) in vec3 aPos;
layout(location=1) in vec3 aNormal;
uniform mat4 uModel;
uniform mat4 uView;
uniform mat4 uProj;
uniform mat3 uNormalMat;
out vec3 vPos;
out vec3 vNormal;
void main(){
    vec4 world = uModel * vec4(aPos,1.0);
    vPos = world.xyz;
    vNormal = normalize(uNormalMat * aNormal);
    gl_Position = uProj * uView * world;
})";

inline const char* PHONG_FS = R"(#version 330 core
in vec3 vPos;
in vec3 vNormal;
uniform vec3 uLightPos;
uniform vec3 uLightColor;
uniform vec3 uViewPos;
uniform vec3 uObjColor;
uniform float uKa;
uniform float uKd;
uniform float uKs;
uniform float uShininess;
out vec4 FragColor;
void main(){
    vec3 N = normalize(vNormal);
    vec3 L = normalize(uLightPos - vPos);
    vec3 V = normalize(uViewPos - vPos);
    vec3 R = reflect(-L, N);
    vec3 ambient  = uKa * uLightColor;
    vec3 diffuse  = uKd * max(dot(N,L),0.0) * uLightColor;
    vec3 specular = uKs * pow(max(dot(V,R),0.0), uShininess) * uLightColor;
    FragColor = vec4((ambient + diffuse + specular) * uObjColor, 1.0);
})";

// ---------- Lineas / puntos ----------
inline const char* LINE_VS = R"(#version 330 core
layout(location=0) in vec3 aPos;
uniform mat4 uMVP;
uniform float uPointSize;
void main(){
    gl_Position = uMVP * vec4(aPos,1.0);
    gl_PointSize = uPointSize;
})";

inline const char* LINE_FS = R"(#version 330 core
uniform vec3 uColor;
out vec4 FragColor;
void main(){ FragColor = vec4(uColor,1.0); })";

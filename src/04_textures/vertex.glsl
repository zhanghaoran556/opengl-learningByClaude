#version 330 core

layout(location = 0) in vec3 aPos;
layout(location = 1) in vec2 aTexCoord;   // UV 坐标，范围约 [0,1]×[0,1]

out vec2 vTexCoord;   // 传给片段着色器，光栅化时自动插值

void main() {
    gl_Position = vec4(aPos, 1.0);
    vTexCoord   = aTexCoord;
}

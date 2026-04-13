#version 330 core

layout(location = 0) in vec3 aPos;
layout(location = 1) in vec2 aTexCoord;

out vec2 vTexCoord;

// MVP 三矩阵：Model（物体→世界）, View（世界→摄像机）, Projection（摄像机→裁剪）
uniform mat4 uModel;
uniform mat4 uView;
uniform mat4 uProjection;

void main() {
    // 从右往左读：先 Model 变换，再 View 变换，最后 Projection 变换
    gl_Position = uProjection * uView * uModel * vec4(aPos, 1.0);
    vTexCoord   = aTexCoord;
}

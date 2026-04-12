#version 330 core

// 从 VAO 属性0 读取位置，属性1 读取颜色
layout(location = 0) in vec3 aPos;
layout(location = 1) in vec3 aColor;

// 传给片段着色器的输出（光栅化时会在顶点之间自动插值）
out vec3 vertColor;

void main() {
    gl_Position = vec4(aPos, 1.0);
    vertColor = aColor;
}

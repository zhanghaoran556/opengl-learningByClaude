#version 330 core

layout(location = 0) in vec3 aPos;
layout(location = 1) in vec2 aTexCoord;

out vec2 vTexCoord;

// 模型矩阵：由 CPU 每帧传入，编码了平移/旋转/缩放
uniform mat4 uModel;

void main() {
    // 顶点坐标左乘模型矩阵，得到变换后的位置
    // 注意矩阵乘法顺序：矩阵在左，列向量在右
    gl_Position = uModel * vec4(aPos, 1.0);
    vTexCoord   = aTexCoord;
}

#version 330 core

layout(location = 0) in vec3 aPos;
layout(location = 1) in vec3 aNormal;

// 输出到片段着色器：世界坐标系下的位置和法线（用于光照计算）
out vec3 vFragPos;
out vec3 vNormal;

uniform mat4 uModel;
uniform mat4 uView;
uniform mat4 uProjection;

// 法线矩阵：模型矩阵左上 3×3 的逆转置
// 当模型有非均匀缩放时，直接用 mat3(uModel) 变换法线会失去垂直性，需要法线矩阵修正
uniform mat3 uNormalMatrix;

void main() {
    // 把顶点变换到世界空间，供片段着色器做光照计算
    vFragPos = vec3(uModel * vec4(aPos, 1.0));

    // 变换法线到世界空间（法线是方向，不是位置，不能直接乘 Model）
    vNormal  = normalize(uNormalMatrix * aNormal);

    gl_Position = uProjection * uView * vec4(vFragPos, 1.0);
}

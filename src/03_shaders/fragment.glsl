#version 330 core

in  vec3 vertColor;   // 从顶点着色器插值而来
out vec4 FragColor;

// uniform：由 CPU 传入的全局变量，所有片段共享同一个值
// 这里用来叠加一个"时间驱动的亮度偏移"，让三角形颜色随时间脉动
uniform float uBrightness;

void main() {
    FragColor = vec4(vertColor * uBrightness, 1.0);
}

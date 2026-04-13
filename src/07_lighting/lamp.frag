#version 330 core

out vec4 FragColor;

// 光源小方块永远显示为纯白色，不受任何光照计算影响
void main() {
    FragColor = vec4(1.0);
}

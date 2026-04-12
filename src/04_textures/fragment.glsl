#version 330 core

in  vec2 vTexCoord;
out vec4 FragColor;

// sampler2D 是纹理采样器类型，值是纹理单元编号（整数）
// CPU 通过 glUniform1i 设置，默认值 0 对应 GL_TEXTURE0
uniform sampler2D uTexture;

void main() {
    // texture() 按 UV 坐标从纹理中采样，返回 vec4(r,g,b,a)
    FragColor = texture(uTexture, vTexCoord);
}

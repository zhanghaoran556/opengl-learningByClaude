#version 330 core

in  vec3 vFragPos;
in  vec3 vNormal;
in  vec2 vTexCoord;
out vec4 FragColor;

// 光照贴图：用纹理替代材质的单一颜色
// diffuse map：每个像素的"本色"（原来是 uMatDiffuse）
// specular map：每个像素的高光强度（亮=有高光，暗=无高光）
uniform sampler2D uDiffuseMap;   // 绑定到纹理单元 0
uniform sampler2D uSpecularMap;  // 绑定到纹理单元 1

uniform vec3  uLightPos;
uniform vec3  uLightColor;
uniform vec3  uViewPos;
uniform float uShininess;

void main() {
    // 从贴图采样，替代 uniform 颜色
    vec3 diffuseColor  = texture(uDiffuseMap,  vTexCoord).rgb;
    vec3 specularColor = texture(uSpecularMap, vTexCoord).rgb;

    vec3 normal   = normalize(vNormal);
    vec3 lightDir = normalize(uLightPos - vFragPos);
    vec3 viewDir  = normalize(uViewPos  - vFragPos);

    // 环境光：使用 diffuse 贴图的颜色，乘以一个低系数
    vec3 ambient = 0.1 * diffuseColor;

    // 漫反射：diffuse 贴图 × 光照角度
    float diff   = max(dot(normal, lightDir), 0.0);
    vec3 diffuse = diffuseColor * uLightColor * diff;

    // 镜面反射：specular 贴图决定这个像素"有多亮"
    // 贴图中黑色区域 specularColor≈(0,0,0)，高光贡献为 0（哑光）
    // 贴图中白色区域 specularColor≈(1,1,1)，高光贡献完整（光滑）
    vec3  reflectDir = reflect(-lightDir, normal);
    float spec       = pow(max(dot(viewDir, reflectDir), 0.0), uShininess);
    vec3  specular   = specularColor * uLightColor * spec;

    FragColor = vec4(ambient + diffuse + specular, 1.0);
}

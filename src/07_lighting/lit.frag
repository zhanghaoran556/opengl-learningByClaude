#version 330 core

in  vec3 vFragPos;
in  vec3 vNormal;
out vec4 FragColor;

// 光源属性
uniform vec3 uLightPos;
uniform vec3 uLightColor;

// 摄像机位置（计算镜面反射需要视线方向）
uniform vec3 uViewPos;

// 材质属性
uniform vec3  uMatAmbient;    // 环境光颜色（物体在无直射光时的底色）
uniform vec3  uMatDiffuse;    // 漫反射颜色（物体本色）
uniform vec3  uMatSpecular;   // 镜面反射颜色（高光颜色）
uniform float uMatShininess;  // 光泽度（值越大，高光越小越锐利）

void main() {
    vec3 normal   = normalize(vNormal);
    vec3 lightDir = normalize(uLightPos - vFragPos);   // 片段指向光源的方向
    vec3 viewDir  = normalize(uViewPos  - vFragPos);   // 片段指向摄像机的方向

    // ── 1. 环境光（Ambient）────────────────────────────────────────────────────
    // 模拟场景中四面八方的间接光，使背光面不完全黑暗
    vec3 ambient = uMatAmbient * uLightColor;

    // ── 2. 漫反射（Diffuse）────────────────────────────────────────────────────
    // 光线越垂直于表面，照度越高。用法线与光方向的点积衡量
    // max(..., 0)：点积为负说明光从背面照来，贡献为 0
    float diff   = max(dot(normal, lightDir), 0.0);
    vec3 diffuse = uMatDiffuse * uLightColor * diff;

    // ── 3. 镜面反射（Specular）─────────────────────────────────────────────────
    // 模拟光滑表面的高光。反射方向与视线方向越接近，高光越强
    // reflect() 计算 lightDir 关于 normal 的镜像方向（注意传入负号：入射方向指向表面）
    vec3  reflectDir = reflect(-lightDir, normal);
    float spec       = pow(max(dot(viewDir, reflectDir), 0.0), uMatShininess);
    vec3  specular   = uMatSpecular * uLightColor * spec;

    FragColor = vec4(ambient + diffuse + specular, 1.0);
}

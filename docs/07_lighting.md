# Lesson 07 — Lighting

> 目标：Phong 光照模型，4 个不同材质的正方体 + 1 个公转光源，WASD 自由飞行观察。

## 运行

```bash
make 07_lighting
./build/07_lighting
```

效果：金、青色塑料、红橡胶、银 4 种材质的正方体，白色光源绕场景公转。
移动摄像机可观察到高光位置随视角变化。

---

## 本节文件结构

```
src/07_lighting/
├── lit.vert   — 被光照物体的顶点着色器（输出世界坐标、法线）
├── lit.frag   — Phong 光照计算（环境光 + 漫反射 + 镜面反射）
├── lamp.vert  — 光源小方块顶点着色器（只做 MVP 变换）
├── lamp.frag  — 光源小方块片段着色器（纯白，不参与光照）
└── main.cpp   — 场景搭建、材质、渲染循环
```

光源和被照物体使用**两套独立的 Shader Program**，是实际项目的标准做法。

---

## 新增概念

### 1. 法线向量（Normal Vector）

法线是垂直于表面的单位向量，用来告诉光照计算"这个面朝哪个方向"。

```
         ↑ 法线 (0,1,0)
─────────┼─────────  表面
```

正方体每个面的法线方向固定，加入顶点数据：

```cpp
// src/07_lighting/main.cpp — 顶点格式：xyz + 法线xyz（6 float）
-0.5f,-0.5f,-0.5f,  0, 0,-1,   // 位置        法线（朝 -Z）
 0.5f,-0.5f,-0.5f,  0, 0,-1,
 ...
```

---

### 2. 法线矩阵（Normal Matrix）

法线不能直接用模型矩阵变换——当模型有**非均匀缩放**时，法线方向会失真。
正确做法是用**法线矩阵**：模型矩阵左上 3×3 的逆转置。

```cpp
// src/07_lighting/main.cpp
glm::mat3 normalMatrix = glm::transpose(glm::inverse(glm::mat3(model)));
glUniformMatrix3fv(glGetUniformLocation(prog, "uNormalMatrix"), 1, GL_FALSE,
                   glm::value_ptr(normalMatrix));
```

```glsl
// src/07_lighting/lit.vert
vNormal = normalize(uNormalMatrix * aNormal);
```

> 如果确定没有非均匀缩放，用 `mat3(uModel)` 也可以，但保持此写法是好习惯。

---

### 3. Phong 光照模型

Phong 把光照拆成三个独立分量，最终颜色是三者之和：

```
最终颜色 = 环境光 + 漫反射 + 镜面反射
```

#### 3.1 环境光（Ambient）

模拟间接光（天空、墙壁反射），使背光面不完全黑暗，是一个常数项：

```glsl
vec3 ambient = uMatAmbient * uLightColor;
```

#### 3.2 漫反射（Diffuse）

光线越垂直于表面照度越高，用**法线与光方向的点积**衡量：

```glsl
vec3  lightDir = normalize(uLightPos - vFragPos);
float diff     = max(dot(normal, lightDir), 0.0);
vec3  diffuse  = uMatDiffuse * uLightColor * diff;
```

直觉：正对光源（dot=1）→ 最亮；平行于光（dot=0）→ 不亮；背对光（dot<0）→ clamp 到 0。

#### 3.3 镜面反射（Specular）

模拟光滑表面的高光，取决于**反射光方向与视线方向的夹角**：

```glsl
vec3  reflectDir = reflect(-lightDir, normal);   // 入射光的镜像方向
float spec       = pow(max(dot(viewDir, reflectDir), 0.0), uMatShininess);
vec3  specular   = uMatSpecular * uLightColor * spec;
```

`shininess`（光泽度）控制高光范围：
- 值小（如 10）：高光大而模糊，像橡皮
- 值大（如 128）：高光小而锐利，像抛光金属

```
shininess=8      shininess=32     shininess=128
  ██████            ████              ██
 ████████          ██████            ████
  ██████            ████              ██
```

---

### 4. 材质（Material）

材质是物体对三种光分量的响应系数，对应三个颜色向量 + 光泽度：

```cpp
// src/07_lighting/main.cpp
struct Material {
    glm::vec3 ambient;    // 环境光颜色（通常是漫反射的暗版）
    glm::vec3 diffuse;    // 漫反射颜色（物体"本色"）
    glm::vec3 specular;   // 高光颜色（金属偏彩色，塑料偏白）
    float     shininess;  // 光泽度
};

// 黄金：高光带金属色，光泽度较高
const Material GOLD = {
    {0.247f, 0.200f, 0.075f},   // ambient  — 暗金
    {0.752f, 0.606f, 0.226f},   // diffuse  — 亮金
    {0.628f, 0.556f, 0.366f},   // specular — 金色高光
    51.2f
};
```

---

### 5. 两套 Shader Program

光源本身不应该受到光照影响（否则会出现光源被自己照亮的悖论），
用独立的 `lamp` 程序让光源始终显示为纯白：

```cpp
// src/07_lighting/main.cpp
GLuint litProg  = createProgram("lit.vert",  "lit.frag");   // Phong 光照
GLuint lampProg = createProgram("lamp.vert", "lamp.frag");  // 纯白输出
```

两者共享同一个 VBO（正方体几何数据），但用不同的 VAO 配置顶点属性。

---

## 光照向量关系图

```
                  ↑ 法线 N
        R ↗       │
（反射）          │
                  │
──────────────────┼────────────────── 表面
        ↗ L               ↗ V
   （光方向）          （视线方向）
   lightDir             viewDir

漫反射强度 ∝ dot(N, L)
镜面强度   ∝ dot(R, V) ^ shininess
```

---

## 下一节预告

**Lesson 08 — Light Maps**：
- 漫反射贴图（Diffuse Map）：用纹理替代单一 diffuse 颜色
- 镜面贴图（Specular Map）：控制表面哪些区域有高光（木纹无高光，金属钉有高光）
- 多纹理单元同时使用

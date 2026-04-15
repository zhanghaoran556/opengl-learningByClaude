# Lesson 08 — Light Maps

> 目标：用 Diffuse Map 和 Specular Map 替代材质的单一颜色，实现逐像素的光照控制。

## 运行

```bash
make 08_light_maps
./build/08_light_maps
```

效果：橙色面板在光源照到时产生高光，深色边框区域无论光从哪里照来都不产生高光——
这就是 Specular Map 的效果：精确控制表面哪里光滑、哪里粗糙。

---

## 核心思想

上一节材质用 `vec3 uMatDiffuse` 定义整块物体的颜色，整个面只有一种颜色。

本节改为从纹理采样，每个**像素**都有独立的颜色和高光强度：

```
Lesson 07：整个物体 diffuse = (0.75, 0.6, 0.22)   ← 统一金色
Lesson 08：每个像素 diffuse = texture(uDiffuseMap, UV).rgb  ← 逐像素
```

---

## 新增概念

### 1. Diffuse Map（漫反射贴图）

用纹理的 RGB 值替代材质的 `diffuse` 颜色：

```glsl
// src/08_light_maps/lit.frag
vec3 diffuseColor = texture(uDiffuseMap, vTexCoord).rgb;

vec3 ambient = 0.1 * diffuseColor;
vec3 diffuse = diffuseColor * uLightColor * max(dot(N, L), 0.0);
```

实际项目中 Diffuse Map 就是美术绘制的贴图（如木箱贴图、砖块贴图）。

---

### 2. Specular Map（镜面贴图）

控制每个像素的**高光强度**：贴图越亮 → 该区域越光滑 → 高光越强：

```glsl
// src/08_light_maps/lit.frag
vec3 specularColor = texture(uSpecularMap, vTexCoord).rgb;

float spec    = pow(max(dot(viewDir, reflectDir), 0.0), uShininess);
vec3 specular = specularColor * uLightColor * spec;
// specularColor ≈ (0,0,0) → 该像素无高光（哑光）
// specularColor ≈ (1,1,1) → 该像素高光完整（光亮）
```

本节的 Specular Map 只有黑白两色，但实际项目中可以用灰度精细控制高光程度。

**本节两张贴图的对应关系：**

```
Diffuse Map            Specular Map
┌──┬──┬──┬──┐         ┌──┬──┬──┬──┐
│橙│暗│橙│暗│         │白│黑│白│黑│   ← 有高光 / 无高光
├──┼──┼──┼──┤         ├──┼──┼──┼──┤
│暗│橙│暗│橙│         │黑│白│黑│白│
└──┴──┴──┴──┘         └──┴──┴──┴──┘
```

高光只出现在橙色面板上，深色边框永远哑光——模拟"金属嵌板 + 木框"的物理效果。

---

### 3. 两个纹理单元同时使用

```cpp
// 绑定：只需一次（Program 不变时 uniform 不需要重复设置）
glUseProgram(litProg);
glUniform1i(glGetUniformLocation(litProg, "uDiffuseMap"),  0);  // → GL_TEXTURE0
glUniform1i(glGetUniformLocation(litProg, "uSpecularMap"), 1);  // → GL_TEXTURE1

// 每帧渲染前激活并绑定
glActiveTexture(GL_TEXTURE0); glBindTexture(GL_TEXTURE_2D, diffuseMap);
glActiveTexture(GL_TEXTURE1); glBindTexture(GL_TEXTURE_2D, specularMap);
```

规律：有几张纹理就用几个纹理单元（GL_TEXTURE0、GL_TEXTURE1…），
`sampler2D` 的 uniform 值就是对应的单元编号（整数）。

---

### 4. 新的顶点格式

加入 UV 坐标，每顶点变为 8 float：

```
位置(xyz) + 法线(xyz) + UV(uv) = 8 float / 顶点

glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 8*sizeof(float), (void*)0               ); // 位置
glVertexAttribPointer(1, 3, GL_FLOAT, GL_FALSE, 8*sizeof(float), (void*)(3*sizeof(float))); // 法线
glVertexAttribPointer(2, 2, GL_FLOAT, GL_FALSE, 8*sizeof(float), (void*)(6*sizeof(float))); // UV
```

---

## 使用真实图片（stb_image）

本节用程序生成纹理。实际项目中用 stb_image 加载图片文件：

```cpp
// 1. 在某个 .cpp 文件中定义实现（只能有一个文件这样做）
#define STB_IMAGE_IMPLEMENTATION
#include "stb_image.h"

// 2. 加载图片
stbi_set_flip_vertically_on_load(true);  // OpenGL UV 原点在左下，图片左上，需翻转
int w, h, ch;
unsigned char* data = stbi_load("assets/diffuse.png", &w, &h, &ch, 0);

GLenum fmt = (ch == 4) ? GL_RGBA : GL_RGB;  // 根据通道数选格式
glTexImage2D(GL_TEXTURE_2D, 0, fmt, w, h, 0, fmt, GL_UNSIGNED_BYTE, data);
stbi_image_free(data);
```

stb_image.h 是单头文件库，只需把文件放入 `include/` 目录，无需额外安装。
可以从 https://github.com/nothings/stb 获取。

---

## 下一节预告

**Lesson 09 — Multiple Lights**：
- 定向光（Directional Light）：模拟太阳，方向固定，无衰减
- 点光源（Point Light）：位置固定，随距离衰减
- 聚光灯（Spotlight）：有方向和角度限制，模拟手电筒
- 多光源叠加：在片段着色器中把多个光源的贡献相加

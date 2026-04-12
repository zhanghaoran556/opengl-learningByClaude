# Lesson 04 — Textures

> 目标：在矩形上贴一张棋盘格纹理。涉及 EBO、UV 坐标、纹理对象、纹理单元。

## 运行

```bash
make 04_textures
./build/04_textures
```

效果：屏幕中央一个白底黑格的棋盘矩形。纹理数据由程序生成，无需外部图片文件。

---

## 新增概念

### 1. EBO — 索引缓冲对象（Element Buffer Object）

**问题**：画矩形需要两个三角形（6 个顶点），但矩形只有 4 个角，有 2 个顶点被重复定义。
顶点复杂时（包含位置 + 法线 + UV + 切线…）重复存储浪费大量显存。

**解决**：用 4 个唯一顶点 + 一份索引列表，让 GPU 按索引重组三角形。

```
顶点数组：  [0, 1, 2, 3]   （4 个顶点）
索引数组：  [0,1,2,  0,2,3] （6 个索引，描述 2 个三角形）

3 ──── 2        三角形A: 0→1→2
│  A/B  │        三角形B: 0→2→3
0 ──── 1
```

```cpp
// src/04_textures/main.cpp — EBO 的创建与绑定
GLuint EBO;
glGenBuffers(1, &EBO);
// 绑 VAO 期间绑定 EBO，VAO 会记住这个关联
glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, EBO);
glBufferData(GL_ELEMENT_ARRAY_BUFFER, sizeof(indices), indices, GL_STATIC_DRAW);
```

绘制时用 `glDrawElements` 替代 `glDrawArrays`：

```cpp
// src/04_textures/main.cpp
glDrawElements(GL_TRIANGLES,   // 图元类型
               6,              // 索引总数
               GL_UNSIGNED_INT,// 索引数据类型
               nullptr);       // EBO 已绑在 VAO 里，偏移传 nullptr
```

---

### 2. UV 坐标（纹理坐标）

UV 坐标定义了顶点对应纹理上的哪个位置，范围通常是 `[0,1] × [0,1]`：

```
纹理空间：            屏幕矩形：
(0,1) ── (1,1)        3 ──── 2
  │          │         │      │
(0,0) ── (1,0)        0 ──── 1

顶点0 → 纹理左下 (0,0)
顶点1 → 纹理右下 (1,0)
顶点2 → 纹理右上 (1,1)
顶点3 → 纹理左上 (0,1)
```

```cpp
// src/04_textures/main.cpp — 顶点数据包含 UV
float vertices[] = {
//   x      y     z      u     v
    -0.7f, -0.7f, 0.0f,  0.0f, 0.0f,   // 左下
     0.7f, -0.7f, 0.0f,  1.0f, 0.0f,   // 右下
     0.7f,  0.7f, 0.0f,  1.0f, 1.0f,   // 右上
    -0.7f,  0.7f, 0.0f,  0.0f, 1.0f,   // 左上
};
// 属性1：UV，2个float，步长5*float，偏移3*float
glVertexAttribPointer(1, 2, GL_FLOAT, GL_FALSE, 5*sizeof(float), (void*)(3*sizeof(float)));
```

UV 超出 `[0,1]` 时的行为由**环绕方式**控制（见下文）。

---

### 3. 纹理对象

```cpp
// src/04_textures/main.cpp — createTexture()

// ① 创建纹理对象
glGenTextures(1, &tex);
glBindTexture(GL_TEXTURE_2D, tex);

// ② 环绕方式：UV 超出 [0,1] 时如何处理
glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_REPEAT);  // U 轴平铺
glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_REPEAT);  // V 轴平铺

// ③ 过滤方式：纹素与像素不一一对应时如何插值
glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);  // 缩小
glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);  // 放大

// ④ 上传像素数据
glTexImage2D(GL_TEXTURE_2D, 0, GL_RGB, w, h, 0, GL_RGB, GL_UNSIGNED_BYTE, data);

// ⑤ 自动生成 Mipmap
glGenerateMipmap(GL_TEXTURE_2D);
```

**环绕方式对比：**

| 值 | 效果 | 适用场景 |
|----|------|---------|
| `GL_REPEAT` | 超出部分平铺重复 | 地面、砖墙 |
| `GL_CLAMP_TO_EDGE` | 超出部分延伸边缘像素 | UI 贴图、精灵 |
| `GL_MIRRORED_REPEAT` | 平铺但每格镜像翻转 | 对称纹理 |

**过滤方式对比：**

| 值 | 效果 | 适用场景 |
|----|------|---------|
| `GL_NEAREST` | 最近邻，像素化锐利 | 像素风游戏 |
| `GL_LINEAR` | 双线性插值，平滑 | 写实风格 |
| `GL_LINEAR_MIPMAP_LINEAR` | 三线性（含 Mipmap），最佳质量 | 3D 场景远处物体 |

---

### 4. 纹理单元与 sampler2D

OpenGL 支持在同一个着色器里同时使用多张纹理（最少 16 个槽位），
通过**纹理单元（Texture Unit）**编号来区分：

```
纹理单元 0 (GL_TEXTURE0) ─── 绑定纹理A ─── sampler2D uTexture (uniform=0)
纹理单元 1 (GL_TEXTURE1) ─── 绑定纹理B ─── sampler2D uNormal  (uniform=1)
```

```cpp
// src/04_textures/main.cpp — 绑定流程

// 一次性：告诉着色器 uTexture 使用纹理单元 0
glUniform1i(glGetUniformLocation(program, "uTexture"), 0);

// 每帧：激活纹理单元，绑定对应的纹理对象
glActiveTexture(GL_TEXTURE0);          // 切换到单元 0
glBindTexture(GL_TEXTURE_2D, texture); // 把纹理绑到这个单元
```

着色器里的 `sampler2D` 值就是纹理单元编号，不是纹理对象 ID：

```glsl
// src/04_textures/fragment.glsl
uniform sampler2D uTexture;   // 值=0，对应 GL_TEXTURE0

void main() {
    FragColor = texture(uTexture, vTexCoord);  // 按 UV 采样
}
```

---

### 5. Mipmap

纹理在 3D 场景中随距离缩小时，直接采样原始分辨率会产生摩尔纹（高频锯齿）。
Mipmap 是同一纹理按 1/2 分辨率逐级预生成的图像集合：

```
原始 128×128
      │
Mip1  64×64
      │
Mip2  32×32
      ...
Mip7   1×1
```

`glGenerateMipmap` 让驱动自动生成所有层级，
配合 `GL_LINEAR_MIPMAP_LINEAR` 过滤时，驱动会根据像素覆盖面积自动选择合适的层级。

---

## 关于加载真实图片

本节用程序生成纹理，聚焦 OpenGL 的 API。
实际项目中使用 **stb_image**（单头文件库）加载 PNG/JPG：

```cpp
// 典型用法（后续课程引入）
#define STB_IMAGE_IMPLEMENTATION
#include "stb_image.h"

int w, h, channels;
stb_image_set_flip_vertically_on_load(true);  // OpenGL UV 原点在左下，图片在左上，需要翻转
unsigned char* data = stbi_load("assets/texture.png", &w, &h, &channels, 0);
glTexImage2D(GL_TEXTURE_2D, 0, GL_RGB, w, h, 0, GL_RGB, GL_UNSIGNED_BYTE, data);
stbi_image_free(data);
```

---

## 下一节预告

**Lesson 05 — Transformations**：
- 引入 GLM 数学库
- 模型矩阵（平移 / 旋转 / 缩放）
- 通过 `glUniformMatrix4fv` 传给着色器，物体动起来

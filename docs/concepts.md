# OpenGL 核心概念手册

> 本文是横跨多节课的概念参考，不按课时顺序，而是按**主题**组织。
> 每个概念都附有项目中对应的代码片段，方便对照理解。
> 随着学习深入，在各节末尾的"扩展"区块中追加内容即可。

---

## 目录

1. [渲染管线总览](#1-渲染管线总览)
2. [GPU 对象模型：句柄与状态机](#2-gpu-对象模型句柄与状态机)
3. [VBO — 顶点缓冲对象](#3-vbo--顶点缓冲对象)
4. [VAO — 顶点数组对象](#4-vao--顶点数组对象)
5. [Shader — 着色器](#5-shader--着色器)
6. [坐标系与 NDC](#6-坐标系与-ndc)
7. [渲染循环与双缓冲](#7-渲染循环与双缓冲)
8. [资源生命周期](#8-资源生命周期)

---

## 1. 渲染管线总览

一次 `glDrawArrays` 调用触发的完整流程：

```
CPU（你的代码）
    │
    │  顶点数据（float[]）
    ▼
┌─────────────────────────────────────────────────────┐
│                      GPU                            │
│                                                     │
│  VBO（原始字节）                                     │
│    │  VAO 告诉 GPU 怎么解读这些字节                   │
│    ▼                                                │
│  顶点着色器       ← 可编程  每顶点执行一次            │
│    │  输出 gl_Position（裁剪空间坐标）               │
│    ▼                                                │
│  图元装配         将顶点连成三角形/线/点              │
│    ▼                                                │
│  光栅化           三角形 → 覆盖的像素片段             │
│    ▼                                                │
│  片段着色器       ← 可编程  每像素执行一次            │
│    │  输出 FragColor（最终颜色）                     │
│    ▼                                                │
│  帧缓冲（显存）                                      │
└─────────────────────────────────────────────────────┘
    │
    │  glfwSwapBuffers
    ▼
  屏幕
```

**两个可编程阶段**是我们写 GLSL 的地方，其余由驱动固定完成。

---

## 2. GPU 对象模型：句柄与状态机

### 句柄（Handle / ID）

OpenGL 里所有 GPU 资源（缓冲、纹理、着色器…）都不返回指针，
而是返回一个 `GLuint` 整数作为**句柄**，实际内存在 GPU 显存中。

```cpp
// src/02_hello_triangle/main.cpp:119
GLuint VAO, VBO;
glGenVertexArrays(1, &VAO);   // 创建，VAO = 1（或其他整数）
glGenBuffers(1, &VBO);        // 创建，VBO = 2
```

类比：就像文件描述符（`int fd = open(...)`），句柄只是个编号，
真正的数据在内核（这里是 GPU 驱动）管理的空间里。

### 绑定 = 激活

OpenGL 是一个**全局状态机**，大多数操作都作用于"当前绑定的对象"。

```
glBindVertexArray(VAO)   →  "当前 VAO = VAO"
glBindBuffer(GL_ARRAY_BUFFER, VBO)  →  "当前 ARRAY_BUFFER = VBO"

之后所有 gl*Buffer* / glVertexAttribPointer 调用都作用于这两个对象
```

传 `0` 解绑：`glBindVertexArray(0)` 表示"当前 VAO = 无"。

---

## 3. VBO — 顶点缓冲对象

**Vertex Buffer Object**：GPU 显存中的一块原始字节数组，用来存储顶点数据。

### 创建 & 上传数据

```cpp
// src/02_hello_triangle/main.cpp:121,127-128
glGenBuffers(1, &VBO);
glBindBuffer(GL_ARRAY_BUFFER, VBO);
glBufferData(GL_ARRAY_BUFFER, sizeof(vertices), vertices, GL_STATIC_DRAW);
//           目标槽位         字节大小           数据指针   使用模式
```

**目标槽位（Target）**，常用的两种：

| 目标 | 用途 |
|------|------|
| `GL_ARRAY_BUFFER` | 顶点属性数据（位置、颜色、UV…） |
| `GL_ELEMENT_ARRAY_BUFFER` | 索引数据（EBO，后续课程） |

**使用模式（Usage）**，影响驱动的内存分配策略：

| 模式 | 含义 |
|------|------|
| `GL_STATIC_DRAW` | 上传一次，频繁读取，几乎不修改（静态网格） |
| `GL_DYNAMIC_DRAW` | 频繁修改，频繁读取（骨骼动画、粒子） |
| `GL_STREAM_DRAW` | 每帧重新上传（实时生成的几何体） |

### VBO 里存的是什么

本节的顶点数组 `vertices` 在内存里是 9 个连续的 float：

```
内存地址:  0     4     8     12    16    20    24    28    32
数据:    -0.5  -0.5   0.0   0.5  -0.5   0.0   0.0   0.5   0.0
         ├──── 顶点0 ────┤ ├──── 顶点1 ────┤ ├──── 顶点2 ────┤
         │  x    y    z  │ │  x    y    z  │ │  x    y    z  │
```

VBO 本身只是字节，**不知道**这些字节代表什么，这正是 VAO 要解决的问题。

---

## 4. VAO — 顶点数组对象

**Vertex Array Object**：记录"如何把 VBO 里的字节解读成顶点属性"的配置。

### 核心函数 glVertexAttribPointer

```cpp
// src/02_hello_triangle/main.cpp:132-133
glVertexAttribPointer(
    0,                  // 属性位置（location），对应 GLSL: layout(location = 0)
    3,                  // 每个顶点该属性有几个分量（这里是 xyz，所以是 3）
    GL_FLOAT,           // 每个分量的数据类型
    GL_FALSE,           // 是否归一化（整数类型才需要，float 填 GL_FALSE）
    3 * sizeof(float),  // 步长（stride）：两个相邻顶点起始地址之间的字节数
    (void*)0            // 偏移（offset）：该属性从每个顶点数据块的第几个字节开始
);
glEnableVertexAttribArray(0);   // 启用 location=0 这个属性槽
```

**步长与偏移的意义**（后续加入颜色属性时会用到）：

```
假设每个顶点数据格式为：[ x, y, z, r, g, b ]
                          ├── 位置 ──┤├─ 颜色 ─┤

位置属性：stride = 6*sizeof(float)，offset = 0
颜色属性：stride = 6*sizeof(float)，offset = 3*sizeof(float)
```

### VAO 记录了什么

绑定 VAO 后，以下操作都会被 VAO"记住"：
- 哪个 VBO 绑定在 `GL_ARRAY_BUFFER`
- 每个属性 location 的 `glVertexAttribPointer` 配置
- 哪些属性 location 被 `glEnableVertexAttribArray` 启用了

```
渲染时：
  glBindVertexArray(VAO)   ← 一行顶替所有配置
  glDrawArrays(...)        ← GPU 按 VAO 里的配置去读 VBO
```

### VAO 与 VBO 的绑定关系

VAO **不复制** VBO 数据，它只记录"当时绑定的是哪个 VBO"。

```
VAO_1
  属性0 → 引用 VBO_A，偏移0，3个float，步长12
  属性1 → 引用 VBO_B，偏移0，2个float，步长8   ← 不同属性可以来自不同 VBO

VAO_2
  属性0 → 引用 VBO_A，偏移0，3个float，步长24  ← 同一个 VBO，不同解读方式
```

---

## 5. Shader — 着色器

### GLSL 基础类型

| 类型 | 含义 | 示例 |
|------|------|------|
| `float` | 单精度浮点 | `float t = 0.5;` |
| `vec2/3/4` | 2/3/4 维向量 | `vec3 pos = vec3(1.0, 0.0, 0.0);` |
| `mat2/3/4` | 矩阵（后续变换用） | `mat4 transform;` |
| `sampler2D` | 2D 纹理采样器 | `uniform sampler2D tex;` |

### 变量限定符

| 限定符 | 作用 | 方向 |
|--------|------|------|
| `in` | 从上一阶段读入 | 顶点着色器中来自 VAO，片段着色器中来自顶点着色器 |
| `out` | 输出到下一阶段 | 顶点着色器 → 片段着色器，片段着色器 → 帧缓冲 |
| `uniform` | CPU 传入的常量，所有顶点/片段共享 | CPU → GPU |

### 顶点着色器

```glsl
// src/02_hello_triangle/main.cpp:12-17
#version 330 core
layout(location = 0) in vec3 aPos;   // VAO 属性0 → 此变量

void main() {
    gl_Position = vec4(aPos, 1.0);   // 输出裁剪空间坐标（内置变量）
}
```

`gl_Position` 是唯一的**内置输出变量**，格式是齐次坐标 `vec4(x, y, z, w)`，
正常情况下 `w=1.0`，透视除法后变成 NDC 坐标 `(x/w, y/w, z/w)`。

### 片段着色器

```glsl
// src/02_hello_triangle/main.cpp:22-27
#version 330 core
out vec4 FragColor;

void main() {
    FragColor = vec4(1.0, 0.5, 0.2, 1.0);   // RGBA，每个分量 [0.0, 1.0]
}
```

### 编译与链接

```cpp
// src/02_hello_triangle/main.cpp:35-83（compileShader + createProgram）

// 流程：
GLuint vert = compileShader(GL_VERTEX_SHADER,   vertexShaderSrc);
GLuint frag = compileShader(GL_FRAGMENT_SHADER, fragmentShaderSrc);
GLuint prog = createProgram(vert, frag);
glDeleteShader(vert);   // 链接完成后可以删除中间产物
glDeleteShader(frag);
```

类比 C++ 编译：`.cpp` → `编译` → `.o` → `链接` → 可执行文件，
着色器也是：GLSL 源码 → `glCompileShader` → 着色器对象 → `glLinkProgram` → Program。

---

## 6. 坐标系与 NDC

### 归一化设备坐标（NDC）

顶点着色器输出的 `gl_Position`（透视除法后）落在这个范围内才可见：

```
X ∈ [-1.0, 1.0]   → 屏幕左到右
Y ∈ [-1.0, 1.0]   → 屏幕下到上（注意：Y 向上为正）
Z ∈ [-1.0, 1.0]   → 深度（-1 近，1 远，Core Profile 默认）
```

```
(-1, 1) ──────── (1, 1)
   │      (0,0)      │
   │     屏幕中心     │
(-1,-1) ──────── (1,-1)
```

本节三角形三顶点均在 z=0 平面，投影到屏幕是：

```
        (0.0, 0.5)
       /           \
(-0.5,-0.5)    (0.5,-0.5)
```

### 视口变换

`glViewport` 把 NDC 坐标映射到像素坐标：

```
NDC x ∈ [-1,1]  →  像素 [0, width]
NDC y ∈ [-1,1]  →  像素 [0, height]
```

```cpp
// src/02_hello_triangle/main.cpp:100
glViewport(0, 0, WIDTH, HEIGHT);
```

### 完整坐标变换链（后续课程会逐步展开）

```
局部空间  →(模型矩阵)→  世界空间  →(视图矩阵)→  观察空间
  →(投影矩阵)→  裁剪空间  →(透视除法)→  NDC  →(视口变换)→  屏幕
```

当前阶段我们直接在顶点着色器里输出 NDC，跳过了中间所有变换。

---

## 7. 渲染循环与双缓冲

```cpp
// src/02_hello_triangle/main.cpp:140-154
while (!glfwWindowShouldClose(window)) {
    // ① 处理输入
    // ② 清空上一帧颜色缓冲（必须，否则上帧残留）
    glClear(GL_COLOR_BUFFER_BIT);
    // ③ 发出绘制命令
    glUseProgram(program);
    glBindVertexArray(VAO);
    glDrawArrays(GL_TRIANGLES, 0, 3);
    // ④ 交换前后缓冲（将后台缓冲显示到屏幕）
    glfwSwapBuffers(window);
    // ⑤ 处理系统事件队列
    glfwPollEvents();
}
```

**双缓冲**：前缓冲显示在屏幕上，后缓冲是正在绘制的当前帧。
`SwapBuffers` 将两者互换，避免观察到半绘制的画面（撕裂）。

**`glClear` 的 Bit 标志**（后续会组合使用）：

| 标志 | 清空内容 | 何时需要 |
|------|---------|---------|
| `GL_COLOR_BUFFER_BIT` | 颜色缓冲 | 几乎每帧 |
| `GL_DEPTH_BUFFER_BIT` | 深度缓冲 | 开启深度测试后 |
| `GL_STENCIL_BUFFER_BIT` | 模板缓冲 | 使用模板测试时 |

---

## 8. 资源生命周期

OpenGL 对象都需要**手动创建和销毁**，驱动不会自动回收。

```cpp
// 创建
glGenBuffers(1, &VBO);
glGenVertexArrays(1, &VAO);
GLuint prog = createProgram(...);

// 使用
// ...

// 销毁（程序退出前）
// src/02_hello_triangle/main.cpp:157-161
glDeleteBuffers(1, &VBO);
glDeleteVertexArrays(1, &VAO);
glDeleteProgram(prog);
glfwDestroyWindow(window);
glfwTerminate();
```

**最小创建/销毁对照表：**

| 创建 | 销毁 |
|------|------|
| `glGenBuffers` | `glDeleteBuffers` |
| `glGenVertexArrays` | `glDeleteVertexArrays` |
| `glGenTextures` | `glDeleteTextures` |
| `glCreateShader` | `glDeleteShader` |
| `glCreateProgram` | `glDeleteProgram` |

> 着色器对象链接进 Program 后就可以立即 `glDeleteShader`，
> Program 内部持有编译结果的引用，不会因此失效。

---

## 扩展区块

> 后续课程学到新概念时，在此处追加对应小节。

<!-- Lesson 03 ✓ -->
### uniform — CPU 向 GPU 传全局变量

`uniform` 在一次 draw call 中对所有顶点/片段只读且值相同，适合传时间、矩阵、颜色参数。

```cpp
// 查询槽位（链接后只需查一次）
GLint loc = glGetUniformLocation(program, "uBrightness");
// 每帧设置（须先 glUseProgram）
glUniform1f(loc, value);         // 1 个 float
// glUniform3f(loc, r, g, b);    // vec3
// glUniformMatrix4fv(loc, 1, GL_FALSE, &mat[0][0]);  // mat4（后续变换用）
```

→ 参见 [src/03_shaders/main.cpp](../src/03_shaders/main.cpp)

### varying — 顶点到片段的插值

顶点着色器 `out` 的变量，片段着色器同名 `in` 接收，光栅化时按重心坐标自动插值：

```glsl
// vertex.glsl
out vec3 vertColor;
// fragment.glsl
in  vec3 vertColor;   // 三顶点颜色的加权混合
```

→ 参见 [src/03_shaders/vertex.glsl](../src/03_shaders/vertex.glsl) · [fragment.glsl](../src/03_shaders/fragment.glsl)

### 交错布局（Interleaved VBO）

多属性混排在同一 VBO 时，步长固定为单顶点总字节数，偏移区分各属性起始位置：

```cpp
// 每顶点 6 float：[ x y z | r g b ]
glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 6*sizeof(float), (void*)0);
glVertexAttribPointer(1, 3, GL_FLOAT, GL_FALSE, 6*sizeof(float), (void*)(3*sizeof(float)));
```

→ 参见 [src/03_shaders/main.cpp:83-89](../src/03_shaders/main.cpp)
<!-- Lesson 04 ✓ -->
### EBO — 索引缓冲对象

用索引复用顶点，避免重复存储：定义 N 个唯一顶点，索引列表描述哪三个顶点组成一个三角形。

```cpp
glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, EBO);   // VAO 会记住这个绑定
glBufferData(GL_ELEMENT_ARRAY_BUFFER, sizeof(indices), indices, GL_STATIC_DRAW);

// 绘制时替代 glDrawArrays
glDrawElements(GL_TRIANGLES, 6, GL_UNSIGNED_INT, nullptr);
```

→ 参见 [src/04_textures/main.cpp](../src/04_textures/main.cpp)

### 纹理对象与纹理单元

```cpp
glGenTextures(1, &tex);
glBindTexture(GL_TEXTURE_2D, tex);
glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_REPEAT);     // U 轴环绕
glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR); // 缩小过滤
glTexImage2D(GL_TEXTURE_2D, 0, GL_RGB, w, h, 0, GL_RGB, GL_UNSIGNED_BYTE, data);
glGenerateMipmap(GL_TEXTURE_2D);

// 渲染时：激活纹理单元，绑定纹理，shader 里 uniform sampler2D 值=单元编号
glActiveTexture(GL_TEXTURE0);
glBindTexture(GL_TEXTURE_2D, tex);
glUniform1i(glGetUniformLocation(program, "uTexture"), 0);
```

→ 参见 [src/04_textures/main.cpp](../src/04_textures/main.cpp) · [fragment.glsl](../src/04_textures/fragment.glsl)

<!-- Lesson 05 ✓ -->
### 模型矩阵与 GLM

GLM 提供 `translate` / `rotate` / `scale` 构造变换矩阵，从单位矩阵开始累乘：

```cpp
glm::mat4 model = glm::mat4(1.0f);                                    // 单位矩阵
model = glm::translate(model, glm::vec3(x, y, 0.0f));                 // 平移
model = glm::rotate(model, angle, glm::vec3(0.0f, 0.0f, 1.0f));      // 旋转（弧度）
model = glm::scale(model, glm::vec3(sx, sy, 1.0f));                   // 缩放
// 代码从上往下写，顶点实际经历顺序从下往上：先缩放→旋转→平移
```

传给着色器：`glUniformMatrix4fv(loc, 1, GL_FALSE, glm::value_ptr(model))`

→ 参见 [src/05_transformations/main.cpp](../src/05_transformations/main.cpp) · [vertex.glsl](../src/05_transformations/vertex.glsl)

<!-- Lesson 06 ✓ -->
### MVP 矩阵

完整变换链：`gl_Position = Projection × View × Model × vertex`

```cpp
// View：lookAt(位置, 目标, 上方向)，每帧随摄像机更新
glm::mat4 view = glm::lookAt(camPos, camPos + camFront, camUp);

// Projection：透视投影，窗口大小不变时只需设一次
glm::mat4 proj = glm::perspective(glm::radians(45.0f), aspect, 0.1f, 100.0f);
```

| 矩阵 | 职责 | 更新时机 |
|------|------|---------|
| Model | 物体在世界中的位置/姿态 | 每物体每帧 |
| View | 摄像机视角（lookAt） | 摄像机移动时 |
| Projection | 透视效果、FOV | 窗口大小改变时 |

→ 参见 [src/06_camera/main.cpp](../src/06_camera/main.cpp) · [vertex.glsl](../src/06_camera/vertex.glsl)

### 深度测试

```cpp
glEnable(GL_DEPTH_TEST);                                  // 初始化时开启
glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);       // 每帧清空两个缓冲
```

→ 参见 [src/06_camera/main.cpp](../src/06_camera/main.cpp)

<!-- Lesson 07: 光照、法线、Phong 模型 -->

# Lesson 02 — Hello Triangle

> 目标：用 VAO、VBO 和着色器在屏幕上画一个橙色三角形。

## 运行

```bash
make 02_hello_triangle
./build/02_hello_triangle
```

---

## 渲染管线总览

现代 OpenGL 的渲染管线（CPU → GPU）分为以下阶段：

```
CPU 内存                GPU
─────────              ──────────────────────────────────────────
顶点数据  ──VBO──►  顶点着色器 → 图元装配 → 光栅化 → 片段着色器 → 帧缓冲
```

其中**顶点着色器**和**片段着色器**是我们可以编程的阶段（GLSL）。

---

## 核心概念

### 1. VBO — 顶点缓冲对象（Vertex Buffer Object）

VBO 是 GPU 上的一块内存，用来存储顶点数据。

```cpp
GLuint VBO;
glGenBuffers(1, &VBO);                                      // 创建
glBindBuffer(GL_ARRAY_BUFFER, VBO);                         // 绑定（激活）
glBufferData(GL_ARRAY_BUFFER, sizeof(vertices), vertices,   // 上传数据
             GL_STATIC_DRAW);
```

`GL_STATIC_DRAW`：数据不会改变（静态几何体）。其他选项：
- `GL_DYNAMIC_DRAW`：数据频繁修改
- `GL_STREAM_DRAW`：每帧都重新上传

---

### 2. VAO — 顶点数组对象（Vertex Array Object）

VAO **记录**了"如何解读 VBO 数据"的配置，核心是 `glVertexAttribPointer`：

```cpp
//         location  分量数  类型       归一化    步长（字节）  偏移
glVertexAttribPointer(0,    3,  GL_FLOAT, GL_FALSE, 3*sizeof(float), (void*)0);
glEnableVertexAttribArray(0);
```

**为什么需要 VAO？**
每次绘制前都重新配置顶点属性很麻烦。VAO 把这些配置"保存"下来，
渲染时只需 `glBindVertexArray(VAO)` 一行，所有配置自动恢复。

**VAO 与 VBO 的关系：**
```
VAO（记录配置）
 └── 属性 0：从 VBO_A 偏移0，读取3个float
 └── 属性 1：从 VBO_B 偏移0，读取2个float（纹理坐标，后续会用）
```

---

### 3. 着色器（Shader）

着色器用 **GLSL**（OpenGL Shading Language）编写，语法类似 C。

**顶点着色器** — 每个顶点执行一次：
```glsl
#version 330 core
layout(location = 0) in vec3 aPos;  // 从 VAO 属性0读取

void main() {
    gl_Position = vec4(aPos, 1.0);  // 输出裁剪空间坐标
}
```

`layout(location = 0)` 对应 `glVertexAttribPointer` 的第一个参数（属性索引）。

**片段着色器** — 每个像素执行一次：
```glsl
#version 330 core
out vec4 FragColor;

void main() {
    FragColor = vec4(1.0, 0.5, 0.2, 1.0);  // R, G, B, A
}
```

**编译 & 链接流程：**
```
GLSL 源码字符串
   ↓ glCompileShader
顶点着色器对象 ──┐
                  ├─ glLinkProgram → 着色器程序（Program）
片段着色器对象 ──┘
```

---

### 4. NDC 坐标系

顶点着色器输出的坐标是**归一化设备坐标（NDC）**：

```
(-1, 1) ────── (1, 1)
   |               |
   |    屏幕中心    |
   |    (0, 0)     |
   |               |
(-1,-1) ────── (1,-1)
```

本节三角形的三个顶点：
```
        (0.0, 0.5)        ← 顶部
       /           \
(-0.5,-0.5)    (0.5,-0.5) ← 左下、右下
```

---

### 5. 绘制调用

```cpp
glUseProgram(program);      // 激活着色器程序
glBindVertexArray(VAO);     // 恢复顶点属性配置
glDrawArrays(GL_TRIANGLES,  // 图元类型：三角形
             0,             // 从第 0 个顶点开始
             3);            // 共 3 个顶点
```

---

## 完整流程图

```
初始化
  ├── 编译顶点着色器
  ├── 编译片段着色器
  ├── 链接为 Program
  ├── 上传顶点数据到 VBO（GPU 内存）
  └── 配置 VAO（记录属性解读方式）

渲染循环（每帧）
  ├── 清空颜色缓冲
  ├── glUseProgram     ← 用哪个着色器
  ├── glBindVertexArray ← 用哪个顶点配置
  ├── glDrawArrays     ← 画！
  └── 交换缓冲
```

---

## 下一节预告

**Lesson 03 — Shaders**：
- 用 `uniform` 变量动态改变三角形颜色
- 在顶点数据中加入颜色属性，实现顶点间颜色插值
- 把着色器代码移到独立的 `.glsl` 文件中

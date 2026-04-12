# Lesson 03 — Shaders

> 目标：彩色渐变三角形 + 亮度随时间脉动。涉及多属性 VBO、uniform 变量、着色器从独立文件加载。

## 运行

```bash
# 必须在项目根目录运行，着色器文件路径是相对路径
make 03_shaders
./build/03_shaders
```

效果：三顶点分别是红/绿/蓝，三角形内部颜色自动插值，整体亮度随时间缓慢脉动。

---

## 新增概念

### 1. 多属性 VBO — 交错布局（Interleaved）

上一节每个顶点只有位置（3 float）。本节每个顶点包含**位置 + 颜色**（6 float）：

```
顶点0                   顶点1                   顶点2
├── x  y  z  r  g  b ──┤├── x  y  z  r  g  b ──┤├── x  y  z  r  g  b ──┤
  0  4  8  12 16 20    24 ...                    48 ...
  （字节偏移）
```

两个属性配置的区别只在**步长**和**偏移**：

```cpp
// src/03_shaders/main.cpp:83-89
// 属性0：位置
glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE,
    6 * sizeof(float),   // 步长：跳过整个顶点（6 float）才到下一个顶点的同属性
    (void*)0);           // 偏移：从字节0开始

// 属性1：颜色
glVertexAttribPointer(1, 3, GL_FLOAT, GL_FALSE,
    6 * sizeof(float),              // 步长：同上
    (void*)(3 * sizeof(float)));    // 偏移：跳过前3个float（位置）才到颜色
```

---

### 2. varying — 顶点到片段的插值

顶点着色器用 `out` 声明的变量，片段着色器用同名 `in` 接收，
光栅化阶段会按**重心坐标**自动在三顶点之间插值：

```glsl
// src/03_shaders/vertex.glsl
out vec3 vertColor;          // 顶点着色器输出
// ...
vertColor = aColor;          // 顶点0=红, 顶点1=绿, 顶点2=蓝

// src/03_shaders/fragment.glsl
in vec3 vertColor;           // 片段着色器接收的是插值结果
// 三角形中心的像素收到 (0.33, 0.33, 0.33) 左右的混合色
```

**重心坐标插值**示意：

```
        蓝(0,0,1)
        /       \
       /  混合色  \
红(1,0,0)────────绿(0,1,0)
```

---

### 3. uniform — CPU 向 GPU 传值

`uniform` 是一种特殊变量：在一次 `glDrawArrays` 期间，所有顶点/片段看到的值**完全相同**。
适合传摄像机矩阵、时间、颜色参数等"全局常量"。

```cpp
// 链接 Program 后查询 uniform 的槽位 ID（只需查询一次）
// src/03_shaders/main.cpp:98
GLint brightnessLoc = glGetUniformLocation(program, "uBrightness");

// 每帧设置值（必须先 glUseProgram）
// src/03_shaders/main.cpp:109-110
glUseProgram(program);
glUniform1f(brightnessLoc, brightness);   // 1f = 1个float
```

**`glUniform*` 命名规则：**

| 函数 | 含义 |
|------|------|
| `glUniform1f` | 1 个 float |
| `glUniform3f` | 3 个 float（vec3，可传颜色/位置） |
| `glUniform1i` | 1 个 int（常用于纹理单元编号） |
| `glUniformMatrix4fv` | 4×4 矩阵（变换矩阵，后续课程） |

---

### 4. 从文件加载着色器

上一节着色器源码硬编码在 `.cpp` 里，修改着色器需要重新编译整个程序。
本节把着色器移到独立的 `.glsl` 文件，运行时读取：

```cpp
// src/03_shaders/main.cpp:15-23
std::string loadFile(const std::string& path) {
    std::ifstream file(path);
    std::ostringstream ss;
    ss << file.rdbuf();   // 整个文件读入 stringstream
    return ss.str();
}
```

**注意**：路径是相对于**可执行文件的工作目录**，而非源文件位置。
执行 `./build/03_shaders` 时工作目录是项目根目录，所以路径写 `src/03_shaders/vertex.glsl`。

---

## 本节代码结构

```
src/03_shaders/
├── main.cpp        — 初始化、VAO/VBO、渲染循环
├── vertex.glsl     — 顶点着色器（位置 + 颜色属性，输出 vertColor）
└── fragment.glsl   — 片段着色器（接收插值颜色，叠加 uniform 亮度）
```

---

## 下一节预告

**Lesson 04 — Textures**：
- EBO（索引缓冲）画矩形，避免顶点重复
- 加载图片文件作为纹理，UV 坐标采样
- `sampler2D` uniform 与纹理单元

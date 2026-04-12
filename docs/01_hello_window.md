# Lesson 01 — Hello Window

> 目标：打开一个深蓝色背景的窗口，按 ESC 退出。

## 运行

```bash
make 01_hello_window
./build/01_hello_window
```

---

## 核心概念

### 1. 整体结构

一个最小的 OpenGL 程序由以下几个阶段组成：

```
初始化 GLFW → 创建窗口 → 初始化 GLEW → 渲染循环 → 清理
```

这不是 OpenGL 独有的设计，几乎所有图形 API（Vulkan、Metal、DirectX）都遵循相同模式。

---

### 2. GLFW vs GLEW — 两个库各做什么？

| 库 | 职责 |
|----|------|
| **GLFW** | 跨平台窗口创建、OpenGL 上下文管理、键盘鼠标输入 |
| **GLEW** | 加载 OpenGL 函数指针（不同显卡驱动实现不同，需要运行时查找） |

OpenGL 本身只是一套**规范**，具体实现由显卡驱动提供。GLEW 的工作就是在运行时找到驱动里这些函数的地址。

---

### 3. OpenGL 版本与 Core Profile

```cpp
glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 3);
glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 3);
glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE);
glfwWindowHint(GLFW_OPENGL_FORWARD_COMPAT, GL_TRUE); // macOS 必须
```

我们选择 **OpenGL 3.3 Core Profile**，原因：

- **3.3** 是现代 OpenGL 的起点，引入了可编程着色器（Shader）的成熟体系
- **Core Profile** 移除了所有旧式 API（glBegin/glEnd 等），强制使用现代方式
- macOS 最高支持到 OpenGL 4.1，`FORWARD_COMPAT` 是 Apple 平台的强制要求

---

### 4. OpenGL 上下文（Context）

```cpp
glfwMakeContextCurrent(window);
```

OpenGL 是一个状态机，所有的 OpenGL 状态都存储在**上下文（Context）**中。
上下文绑定到具体的窗口和线程——同一时刻一个线程只能有一个活跃的上下文。

可以把 Context 理解为"一块画布 + 当前所有绘图设置的总集合"。

---

### 5. 渲染循环（Render Loop）

```cpp
while (!glfwWindowShouldClose(window)) {
    processInput(window);       // 处理输入
    glClearColor(0.1f, 0.1f, 0.2f, 1.0f);
    glClear(GL_COLOR_BUFFER_BIT); // 清空上一帧
    glfwSwapBuffers(window);    // 显示当前帧
    glfwPollEvents();           // 派发系统事件
}
```

渲染循环是整个 OpenGL 程序的核心节奏，每次循环 = 绘制一帧。

**双缓冲（Double Buffering）**：
- **前缓冲**：当前屏幕上显示的画面
- **后缓冲**：正在绘制中的下一帧
- `glfwSwapBuffers` 交换两者，避免画面撕裂

**glClear 的必要性**：
每帧开始必须清空颜色缓冲，否则上一帧的内容会残留。`GL_COLOR_BUFFER_BIT` 表示清空颜色缓冲区（后续还会用到 `GL_DEPTH_BUFFER_BIT`）。

---

### 6. 视口（Viewport）

```cpp
glViewport(0, 0, WIDTH, HEIGHT);
```

视口定义了 OpenGL 将裁剪坐标映射到屏幕坐标的区域。
OpenGL 内部坐标范围是 [-1, 1]（归一化设备坐标，NDC），视口负责把它拉伸到像素坐标。

---

## 坐标系预告

OpenGL 使用**右手坐标系**：
- X 轴向右
- Y 轴向上
- Z 轴朝向屏幕外（指向观察者）

屏幕中心是原点 (0, 0)，屏幕范围是 [-1, 1] × [-1, 1]（NDC）。
这是后续所有几何变换的基础，下一节会深入讲解。

---

## 下一节预告

**Lesson 02 — Hello Triangle**：用 VAO、VBO 和着色器（Shader）在屏幕上画第一个三角形。
这是 OpenGL 渲染管线的核心，也是最重要的一课。

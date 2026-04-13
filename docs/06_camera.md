# Lesson 06 — Camera

> 目标：10 个旋转的正方体场景，WASD + 鼠标自由飞行。完成完整的 MVP 变换链。

## 运行

```bash
make 06_camera
./build/06_camera
```

操作：**WASD** 移动，**鼠标** 转向，**Space/Shift** 上下，**ESC** 退出。

---

## MVP 变换链全貌

```
局部坐标（顶点原始位置）
    │ × uModel
    ▼
世界坐标（物体在场景中的位置）
    │ × uView
    ▼
观察坐标（以摄像机为原点重新描述的坐标）
    │ × uProjection
    ▼
裁剪坐标 → 透视除法 → NDC → 视口变换 → 屏幕
```

```glsl
// src/06_camera/vertex.glsl
gl_Position = uProjection * uView * uModel * vec4(aPos, 1.0);
// 矩阵乘法从右往左读，顶点依次经历 Model → View → Projection
```

---

## 新增概念

### 1. View 矩阵 — glm::lookAt

View 矩阵描述"以摄像机的眼睛看世界"，本质是把整个世界坐标系变换到
以摄像机为原点、朝向为 -Z 的新坐标系。

```cpp
// src/06_camera/main.cpp — 每帧更新
glm::mat4 view = glm::lookAt(
    camPos,           // 摄像机位置（眼睛在哪）
    camPos + camFront,// 看向的目标点
    camUp             // 世界上方向（0,1,0），用来确定"头顶朝上"
);
```

**直觉理解**：想象摄像机不动，而是把整个世界平移+旋转，
使摄像机的视线方向对齐 -Z 轴——这就是 View 矩阵在做的事。

---

### 2. Projection 矩阵 — glm::perspective

透视投影让远处的东西看起来更小（近大远小），这才是真实的 3D 感。

```cpp
// src/06_camera/main.cpp — 只需设置一次
glm::mat4 projection = glm::perspective(
    glm::radians(45.0f),          // 垂直视角（FOV）：人眼约 60-120°，45° 偏保守
    (float)WIDTH / (float)HEIGHT, // 宽高比，保证不变形
    0.1f,                         // 近裁剪面（Near）：比这近的东西不渲染
    100.0f                        // 远裁剪面（Far）：比这远的东西不渲染
);
```

**视锥体（Frustum）**：透视投影定义了一个截锥形可视区域，
只有落在里面的几何体才会被渲染：

```
        近裁剪面
     /──────────\
    /             \
   /   视锥体      \
  /                 \
 /────────────────────\
        远裁剪面
```

FOV 越大，视角越宽，边缘畸变越明显（鱼眼效果）。

---

### 3. 深度测试

进入 3D 后，多个物体可能在屏幕同一位置重叠，需要靠深度判断谁在前面。

```cpp
// 初始化时开启一次
glEnable(GL_DEPTH_TEST);

// 每帧清空（必须！否则上帧的深度值会错误地遮挡当前帧）
glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
```

GPU 维护一张与颜色缓冲等大的**深度缓冲（Z-buffer）**，
存储每个像素目前最近的深度值。每次写入片段颜色前先与 Z-buffer 比较，
更近则写入并更新 Z-buffer，否则丢弃。

---

### 4. 欧拉角摄像机

用**偏航角 yaw**（左右转）和**俯仰角 pitch**（上下看）描述朝向，
比直接操作向量更直观，可以自然地把鼠标偏移量映射成角度变化。

```cpp
// src/06_camera/main.cpp — mouseCallback
yaw   += dx * sensitivity;   // 鼠标水平移动 → 左右转
pitch += dy * sensitivity;   // 鼠标垂直移动 → 上下看

// 限制 pitch 防止超过 ±90°（翻转会导致画面倒置）
pitch = glm::clamp(pitch, -89.0f, 89.0f);

// 从角度还原朝向向量（球坐标 → 笛卡尔坐标）
camFront.x = cos(radians(yaw)) * cos(radians(pitch));
camFront.y = sin(radians(pitch));
camFront.z = sin(radians(yaw)) * cos(radians(pitch));
camFront = normalize(camFront);
```

---

### 5. 帧率无关的移动速度

如果直接用固定步长移动，帧率高的机器移动更快。
用 **deltaTime**（上帧耗时）乘以速度，保证每秒移动距离恒定：

```cpp
// src/06_camera/main.cpp
float deltaTime = currentFrame - lastFrame;   // 本帧耗时（秒）
float speed     = 2.5f * deltaTime;           // 无论帧率，每秒移动 2.5 单位

camPos += speed * camFront;   // W 键向前
```

---

### 6. 叉积求右向量

向左右移动时，需要摄像机的"右方向"向量。
**叉积**（cross product）可以从两个向量求出垂直于它们的第三个向量：

```cpp
// camFront × camUp = 指向右侧的向量（右手定则）
glm::vec3 right = glm::normalize(glm::cross(camFront, camUp));
camPos += right * speed;   // D 键向右
```

归一化是必须的——两个非单位向量的叉积长度不为 1，
移动速度会随朝向改变而变化。

---

## 坐标变换链（完整）

```
局部空间  ──(Model)──►  世界空间  ──(View)──►  观察空间
  ──(Projection)──►  裁剪空间  ──(÷w)──►  NDC  ──(Viewport)──►  屏幕
```

MVP 三矩阵各自的职责：

| 矩阵 | 职责 | 更新频率 |
|------|------|---------|
| Model | 把物体放到世界某处（平移/旋转/缩放） | 每个物体每帧 |
| View | 模拟摄像机视角（lookAt） | 摄像机移动时 |
| Projection | 透视效果，映射到 NDC | 窗口大小改变时 |

---

## 下一节预告

**Lesson 07 — Lighting**：
- 法线向量（Normal）与光照模型
- Phong 光照：环境光 + 漫反射 + 镜面反射
- 点光源与材质 uniform

# Lesson 05 — Transformations

> 目标：用 GLM 构造模型矩阵，让两个矩形分别持续旋转和呼吸缩放。

## 运行

```bash
make 05_transformations
./build/05_transformations
```

效果：右上角矩形持续旋转，左下角矩形大小随时间脉动。同一套 VAO/VBO 数据，
每次 draw call 前更换矩阵，画出两个不同位置/状态的物体。

---

## 新增概念

### 1. 为什么需要矩阵

上一节物体坐标直接写死在顶点数据里，想让物体移动只能重新上传 VBO，代价很高。

矩阵的做法：顶点数据永远不变，只更换一个 uniform 矩阵，
GPU 在顶点着色器里用矩阵乘法把每个顶点变换到正确位置：

```glsl
// src/05_transformations/vertex.glsl
gl_Position = uModel * vec4(aPos, 1.0);
```

---

### 2. 三种基础变换

**平移（Translation）**

```cpp
// 沿 X+0.5, Y+0.5 方向移动
model = glm::translate(model, glm::vec3(0.5f, 0.5f, 0.0f));
```

**旋转（Rotation）**

```cpp
// 绕 Z 轴旋转 t 弧度（t 随时间增大 → 持续旋转）
model = glm::rotate(model, t, glm::vec3(0.0f, 0.0f, 1.0f));
//                  角度(弧度)  旋转轴方向（单位向量）
```

旋转轴含义：
- `(0, 0, 1)`：绕 Z 轴 → 屏幕内平面旋转（2D 常用）
- `(1, 0, 0)`：绕 X 轴 → 上下翻转
- `(0, 1, 0)`：绕 Y 轴 → 左右翻转

**缩放（Scale）**

```cpp
// XY 方向缩小到 50%，Z 方向不变
model = glm::scale(model, glm::vec3(0.5f, 0.5f, 1.0f));
```

---

### 3. 变换顺序：从下往上读

```cpp
// src/05_transformations/main.cpp — 右上角矩形
glm::mat4 model = glm::mat4(1.0f);          // 单位矩阵（不做任何变换）
model = glm::translate(model, glm::vec3(0.5f, 0.5f, 0.0f));  // ③ 最后平移
model = glm::rotate(model, t, glm::vec3(0.0f, 0.0f, 1.0f));  // ② 再旋转
model = glm::scale(model, glm::vec3(0.5f, 0.5f, 1.0f));      // ① 先缩放
```

虽然代码从上往下写，但矩阵乘法是**右结合**的，顶点实际经历的顺序是：

```
顶点  →  ①缩放  →  ②旋转  →  ③平移  →  gl_Position
```

**顺序很重要**——先旋转再平移 ≠ 先平移再旋转：

```
先缩放再平移（正确）：物体以自身中心缩小，然后移到目标位置
先平移再缩放（错误）：物体先移走，缩放时连位移一起被缩放，偏离预期位置
```

---

### 4. GLM 矩阵传给 OpenGL

GLM 的矩阵类型不能直接传给 `glUniformMatrix4fv`，需要用 `glm::value_ptr` 取得底层 `float*`：

```cpp
// src/05_transformations/main.cpp
glUniformMatrix4fv(
    modelLoc,               // uniform 的 location
    1,                      // 矩阵数量（传矩阵数组时用）
    GL_FALSE,               // 是否转置。GLM 和 OpenGL 都用列主序，不需要转置
    glm::value_ptr(model)   // float* 指针，指向矩阵 16 个元素
);
```

---

### 5. 同一 VAO，多次 draw call

本节用同一套 VAO/VBO 画了两个矩形，关键在于每次 draw 前换矩阵：

```cpp
// 物体1
glUniformMatrix4fv(modelLoc, 1, GL_FALSE, glm::value_ptr(model1));
glDrawElements(GL_TRIANGLES, 6, GL_UNSIGNED_INT, nullptr);

// 物体2（矩阵不同，位置/大小不同）
glUniformMatrix4fv(modelLoc, 1, GL_FALSE, glm::value_ptr(model2));
glDrawElements(GL_TRIANGLES, 6, GL_UNSIGNED_INT, nullptr);
```

这是 OpenGL 批量渲染的基础模式：相同的几何体，通过不同的矩阵实例化到场景各处。
（更高效的做法是 Instancing，后续课程涉及。）

---

### 6. 单位矩阵与矩阵组合

`glm::mat4(1.0f)` 是**单位矩阵**，类似数字 1，乘任何矩阵结果不变，作为构建起点：

```
1 0 0 0
0 1 0 0
0 0 1 0
0 0 0 1
```

每次调用 `translate/rotate/scale` 实际上是：

```
model = TranslateMatrix * model
      = TranslateMatrix * RotateMatrix * ScaleMatrix * Identity
```

最终的 `model` 就是三个变换的组合矩阵，顶点只需乘以它一次。

---

## 坐标变换链现状

```
局部空间
   │
   │ × uModel（本节引入）
   ▼
世界空间 / 裁剪空间（当前直接输出，下一节引入 View + Projection）
   │
   ▼
NDC → 屏幕
```

下一节加入**摄像机（View 矩阵）**和**透视投影（Projection 矩阵）**后，
完整的变换链 `gl_Position = Projection * View * Model * vertex` 就建立起来了。

---

## 下一节预告

**Lesson 06 — Camera**：
- View 矩阵：`glm::lookAt`，模拟摄像机位置和朝向
- Projection 矩阵：`glm::perspective`，近大远小的透视效果
- 键盘控制摄像机在 3D 空间中移动

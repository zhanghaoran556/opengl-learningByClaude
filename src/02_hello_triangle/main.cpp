#include <GL/glew.h>
#include <GLFW/glfw3.h>
#include <iostream>

const int WIDTH  = 800;
const int HEIGHT = 600;

// ─── 着色器源码（内嵌字符串）─────────────────────────────────────────────────

// 顶点着色器：把顶点坐标原样传出去（NDC 坐标，不做变换）
const char* vertexShaderSrc = R"(
#version 330 core
layout(location = 0) in vec3 aPos;   // 从 VBO 读取顶点位置

void main() {
    gl_Position = vec4(aPos, 1.0);   // xyz + w=1 → 齐次坐标
}
)";

// 片段着色器：每个像素输出橙色
const char* fragmentShaderSrc = R"(
#version 330 core
out vec4 FragColor;   // 输出颜色（RGBA）

void main() {
    FragColor = vec4(1.0, 0.5, 0.2, 1.0);   // 橙色
}
)";

// ─── 辅助函数：编译单个着色器 ─────────────────────────────────────────────────
// type: 着色器类型，GL_VERTEX_SHADER 或 GL_FRAGMENT_SHADER
// src:  GLSL 源码字符串
// 返回：着色器对象 ID（GPU 侧资源句柄）

GLuint compileShader(GLenum type, const char* src) {
    // 在 GPU 上分配一个着色器对象，返回其 ID
    GLuint shader = glCreateShader(type);

    // 将源码绑定到着色器对象
    // 参数：着色器ID, 字符串数量（支持传多段拼接）, 字符串指针数组, 各段长度（nullptr=自动按\0截断）
    glShaderSource(shader, 1, &src, nullptr);

    // 驱动在 GPU 上编译 GLSL → 机器码
    glCompileShader(shader);

    // 查询编译结果：GL_COMPILE_STATUS 写入 success（GL_TRUE / GL_FALSE）
    int success;
    glGetShaderiv(shader, GL_COMPILE_STATUS, &success);
    if (!success) {
        char log[512];
        // 取编译器错误日志：着色器ID, 缓冲区大小, 实际写入长度（nullptr=不需要）, 日志内容
        glGetShaderInfoLog(shader, 512, nullptr, log);
        std::cerr << "Shader compile error:\n" << log << "\n";
    }
    return shader;
}

// ─── 辅助函数：链接着色器程序 ─────────────────────────────────────────────────
// vert: 已编译的顶点着色器 ID
// frag: 已编译的片段着色器 ID
// 返回：着色器程序 ID，glUseProgram 时使用

GLuint createProgram(GLuint vert, GLuint frag) {
    // 创建一个空的着色器程序对象
    GLuint program = glCreateProgram();

    // 把两个着色器挂载到程序上（可以挂多个，但同类型只能有一个生效）
    glAttachShader(program, vert);
    glAttachShader(program, frag);

    // 链接：把顶点着色器的输出与片段着色器的输入对接，生成最终可执行程序
    glLinkProgram(program);

    // 查询链接结果
    int success;
    glGetProgramiv(program, GL_LINK_STATUS, &success);
    if (!success) {
        char log[512];
        glGetProgramInfoLog(program, 512, nullptr, log);
        std::cerr << "Program link error:\n" << log << "\n";
    }
    return program;
}

// ─── main ────────────────────────────────────────────────────────────────────

int main() {
    // 1. 初始化 GLFW & 窗口
    glfwInit();
    glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 3);
    glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 3);
    glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE);
    glfwWindowHint(GLFW_OPENGL_FORWARD_COMPAT, GL_TRUE);

    GLFWwindow* window = glfwCreateWindow(WIDTH, HEIGHT, "Hello Triangle", nullptr, nullptr);
    glfwMakeContextCurrent(window);

    glewExperimental = GL_TRUE;
    glewInit();
    glViewport(0, 0, WIDTH, HEIGHT);

    // 2. 编译 & 链接着色器
    GLuint vert    = compileShader(GL_VERTEX_SHADER,   vertexShaderSrc);
    GLuint frag    = compileShader(GL_FRAGMENT_SHADER, fragmentShaderSrc);
    GLuint program = createProgram(vert, frag);
    // 链接完成后，独立的着色器对象就不再需要了
    glDeleteShader(vert);
    glDeleteShader(frag);

    // 3. 定义三角形顶点数据（NDC 坐标）
    //    屏幕中心为原点，范围 [-1, 1]
    float vertices[] = {
        -0.5f, -0.5f, 0.0f,   // 左下
         0.5f, -0.5f, 0.0f,   // 右下
         0.0f,  0.5f, 0.0f,   // 顶部
    };

    // 4. 创建 VAO 和 VBO
    GLuint VAO, VBO;
    glGenVertexArrays(1, &VAO);
    glGenBuffers(1, &VBO);

    // 先绑定 VAO，它会记录接下来对 VBO 和顶点属性的所有配置
    glBindVertexArray(VAO);

    // 把顶点数据上传到 GPU（VBO）
    glBindBuffer(GL_ARRAY_BUFFER, VBO);
    glBufferData(GL_ARRAY_BUFFER, sizeof(vertices), vertices, GL_STATIC_DRAW);

    // 告诉 OpenGL 如何解读 VBO 中的数据：
    //   location=0, 3个float, 不归一化, 步长=3*float, 从偏移0开始
    glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 3 * sizeof(float), (void*)0);
    glEnableVertexAttribArray(0);

    // 解绑（可选，好习惯）
    glBindBuffer(GL_ARRAY_BUFFER, 0);
    glBindVertexArray(0);

    // 5. 渲染循环
    while (!glfwWindowShouldClose(window)) {
        if (glfwGetKey(window, GLFW_KEY_ESCAPE) == GLFW_PRESS)
            glfwSetWindowShouldClose(window, true);

        glClearColor(0.1f, 0.1f, 0.2f, 1.0f);
        glClear(GL_COLOR_BUFFER_BIT);

        // 激活着色器程序，绑定 VAO，画三角形
        glUseProgram(program);
        glBindVertexArray(VAO);
        glDrawArrays(GL_TRIANGLES, 0, 3);   // 从第0个顶点开始，画3个

        glfwSwapBuffers(window);
        glfwPollEvents();
    }

    // 6. 清理
    glDeleteVertexArrays(1, &VAO);
    glDeleteBuffers(1, &VBO);
    glDeleteProgram(program);
    glfwDestroyWindow(window);
    glfwTerminate();
    return 0;
}

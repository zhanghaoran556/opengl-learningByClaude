#include <GL/glew.h>
#include <GLFW/glfw3.h>
#include <fstream>
#include <iostream>
#include <sstream>
#include <string>

const int WIDTH  = 800;
const int HEIGHT = 600;

// ─── 从文件读取着色器源码 ───────────────────────────────────────────────────────
// path: .glsl 文件路径（相对于可执行文件所在目录）
// 返回：文件内容字符串

std::string loadFile(const std::string& path) {
    std::ifstream file(path);
    if (!file.is_open()) {
        std::cerr << "Cannot open shader file: " << path << "\n";
        return "";
    }
    std::ostringstream ss;
    ss << file.rdbuf();   // 一次性读取整个文件到 stringstream
    return ss.str();
}

// ─── 编译单个着色器 ────────────────────────────────────────────────────────────
// type: GL_VERTEX_SHADER 或 GL_FRAGMENT_SHADER
// src:  GLSL 源码字符串

GLuint compileShader(GLenum type, const std::string& src) {
    GLuint shader     = glCreateShader(type);
    const char* c_src = src.c_str();
    glShaderSource(shader, 1, &c_src, nullptr);
    glCompileShader(shader);

    int success;
    glGetShaderiv(shader, GL_COMPILE_STATUS, &success);
    if (!success) {
        char log[512];
        glGetShaderInfoLog(shader, 512, nullptr, log);
        std::cerr << "Shader compile error:\n" << log << "\n";
    }
    return shader;
}

// ─── 链接着色器程序 ────────────────────────────────────────────────────────────

GLuint createProgram(GLuint vert, GLuint frag) {
    GLuint program = glCreateProgram();
    glAttachShader(program, vert);
    glAttachShader(program, frag);
    glLinkProgram(program);

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

    GLFWwindow* window = glfwCreateWindow(WIDTH, HEIGHT, "Shaders", nullptr, nullptr);
    glfwMakeContextCurrent(window);
    glewExperimental = GL_TRUE;
    glewInit();
    glViewport(0, 0, WIDTH, HEIGHT);

    // 2. 从文件加载并编译着色器
    std::string vertSrc = loadFile("src/03_shaders/vertex.glsl");
    std::string fragSrc = loadFile("src/03_shaders/fragment.glsl");

    GLuint vert    = compileShader(GL_VERTEX_SHADER,   vertSrc);
    GLuint frag    = compileShader(GL_FRAGMENT_SHADER, fragSrc);
    GLuint program = createProgram(vert, frag);
    glDeleteShader(vert);
    glDeleteShader(frag);

    // 3. 顶点数据：每个顶点包含 位置(xyz) + 颜色(rgb)，共 6 个 float
    float vertices[] = {
    //   x      y     z      r     g     b
        -0.5f, -0.5f, 0.0f,  1.0f, 0.0f, 0.0f,   // 左下 — 红
         0.5f, -0.5f, 0.0f,  0.0f, 1.0f, 0.0f,   // 右下 — 绿
         0.0f,  0.5f, 0.0f,  0.0f, 0.0f, 1.0f,   // 顶部 — 蓝
    };

    // 4. 创建 VAO & VBO
    GLuint VAO, VBO;
    glGenVertexArrays(1, &VAO);
    glGenBuffers(1, &VBO);

    glBindVertexArray(VAO);
    glBindBuffer(GL_ARRAY_BUFFER, VBO);
    glBufferData(GL_ARRAY_BUFFER, sizeof(vertices), vertices, GL_STATIC_DRAW);

    // 属性0：位置，3个float，步长=6*float，从偏移0开始
    glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 6 * sizeof(float), (void*)0);
    glEnableVertexAttribArray(0);

    // 属性1：颜色，3个float，步长=6*float，从偏移 3*float 开始
    glVertexAttribPointer(1, 3, GL_FLOAT, GL_FALSE, 6 * sizeof(float), (void*)(3 * sizeof(float)));
    glEnableVertexAttribArray(1);

    glBindBuffer(GL_ARRAY_BUFFER, 0);
    glBindVertexArray(0);

    // 5. 查询 uniform 的位置（location）
    // glGetUniformLocation 在 Program 链接后调用，返回该 uniform 在程序中的槽位 ID
    // 后续每次修改 uniform 值都用这个 ID，比每次按名字查找更高效
    GLint brightnessLoc = glGetUniformLocation(program, "uBrightness");

    // 6. 渲染循环
    while (!glfwWindowShouldClose(window)) {
        if (glfwGetKey(window, GLFW_KEY_ESCAPE) == GLFW_PRESS)
            glfwSetWindowShouldClose(window, true);

        glClearColor(0.1f, 0.1f, 0.2f, 1.0f);
        glClear(GL_COLOR_BUFFER_BIT);

        glUseProgram(program);

        // 用 sin 让亮度在 [0.2, 1.0] 之间周期性变化，产生"脉动"效果
        // glfwGetTime() 返回程序启动后经过的秒数（double）
        float brightness = 0.6f * (float)std::sin(glfwGetTime()) + 0.6f;
        // 设置 uniform 值：必须先 glUseProgram，再 glUniform*
        glUniform1f(brightnessLoc, brightness);

        glBindVertexArray(VAO);
        glDrawArrays(GL_TRIANGLES, 0, 3);

        glfwSwapBuffers(window);
        glfwPollEvents();
    }

    // 7. 清理
    glDeleteVertexArrays(1, &VAO);
    glDeleteBuffers(1, &VBO);
    glDeleteProgram(program);
    glfwDestroyWindow(window);
    glfwTerminate();
    return 0;
}

#include <GL/glew.h>
#include <GLFW/glfw3.h>
#include <fstream>
#include <iostream>
#include <sstream>
#include <string>
#include <vector>

const int WIDTH  = 800;
const int HEIGHT = 600;

// ─── 工具函数（与 Lesson 03 相同，后续可提取为公共头文件）─────────────────────

std::string loadFile(const std::string& path) {
    std::ifstream file(path);
    if (!file.is_open()) { std::cerr << "Cannot open: " << path << "\n"; return ""; }
    std::ostringstream ss;
    ss << file.rdbuf();
    return ss.str();
}

GLuint compileShader(GLenum type, const std::string& src) {
    GLuint shader = glCreateShader(type);
    const char* c = src.c_str();
    glShaderSource(shader, 1, &c, nullptr);
    glCompileShader(shader);
    int ok; glGetShaderiv(shader, GL_COMPILE_STATUS, &ok);
    if (!ok) { char log[512]; glGetShaderInfoLog(shader, 512, nullptr, log); std::cerr << log << "\n"; }
    return shader;
}

GLuint createProgram(GLuint vert, GLuint frag) {
    GLuint prog = glCreateProgram();
    glAttachShader(prog, vert); glAttachShader(prog, frag);
    glLinkProgram(prog);
    int ok; glGetProgramiv(prog, GL_LINK_STATUS, &ok);
    if (!ok) { char log[512]; glGetProgramInfoLog(prog, 512, nullptr, log); std::cerr << log << "\n"; }
    return prog;
}

// ─── 生成棋盘格纹理（8×8，黑白各32像素）─────────────────────────────────────
// 返回一块 RGB 像素数据，每行 width 个像素，共 height 行，每像素 3 字节
// 本节不依赖图片文件，直接在内存中生成像素，演示 glTexImage2D 的原始用法

std::vector<unsigned char> makeCheckerboard(int w, int h, int cellSize) {
    std::vector<unsigned char> pixels(w * h * 3);
    for (int y = 0; y < h; ++y) {
        for (int x = 0; x < w; ++x) {
            // 根据格子奇偶性决定黑白
            bool white = ((x / cellSize) + (y / cellSize)) % 2 == 0;
            unsigned char c = white ? 230 : 30;
            int idx = (y * w + x) * 3;
            pixels[idx + 0] = c;   // R
            pixels[idx + 1] = c;   // G
            pixels[idx + 2] = c;   // B
        }
    }
    return pixels;
}

// ─── 创建并上传纹理 ────────────────────────────────────────────────────────────

GLuint createTexture() {
    GLuint tex;
    // 在 GPU 上分配一个纹理对象
    glGenTextures(1, &tex);
    // 绑定到 GL_TEXTURE_2D 槽位（激活），后续 gl*Tex* 操作都针对它
    glBindTexture(GL_TEXTURE_2D, tex);

    // 设置纹理环绕方式（UV 超出 [0,1] 时如何处理）
    // GL_REPEAT：平铺重复（常用于地面、砖块）
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_REPEAT);   // U 轴
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_REPEAT);   // V 轴

    // 设置纹理过滤方式
    // GL_NEAREST：最近邻，像素化风格（像素游戏常用）
    // GL_LINEAR： 双线性插值，平滑
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);   // 缩小时
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);   // 放大时

    // 生成像素数据并上传到 GPU
    const int texW = 128, texH = 128, cell = 16;
    auto pixels = makeCheckerboard(texW, texH, cell);

    // 上传像素数据：
    // 目标, Mipmap层级, GPU内部格式, 宽, 高, 边框(必须0), 输入格式, 输入类型, 数据指针
    glTexImage2D(GL_TEXTURE_2D, 0, GL_RGB, texW, texH, 0,
                 GL_RGB, GL_UNSIGNED_BYTE, pixels.data());

    // 自动生成 Mipmap（不同距离使用不同分辨率，减少摩尔纹）
    glGenerateMipmap(GL_TEXTURE_2D);

    return tex;
}

// ─── main ────────────────────────────────────────────────────────────────────

int main() {
    glfwInit();
    glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 3);
    glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 3);
    glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE);
    glfwWindowHint(GLFW_OPENGL_FORWARD_COMPAT, GL_TRUE);

    GLFWwindow* window = glfwCreateWindow(WIDTH, HEIGHT, "Textures", nullptr, nullptr);
    glfwMakeContextCurrent(window);
    glewExperimental = GL_TRUE;
    glewInit();
    glViewport(0, 0, WIDTH, HEIGHT);

    // 着色器
    GLuint vert    = compileShader(GL_VERTEX_SHADER,   loadFile("src/04_textures/vertex.glsl"));
    GLuint frag    = compileShader(GL_FRAGMENT_SHADER, loadFile("src/04_textures/fragment.glsl"));
    GLuint program = createProgram(vert, frag);
    glDeleteShader(vert);
    glDeleteShader(frag);

    // 纹理
    GLuint texture = createTexture();

    // 顶点数据：矩形由 4 个顶点定义（而非 6 个），EBO 负责复用
    // 每顶点：位置(xyz) + UV(st)，共 5 个 float
    //
    //   3 ──── 2        UV:  (0,1) ──── (1,1)
    //   │      │             │               │
    //   0 ──── 1             (0,0) ──── (1,0)
    //
    float vertices[] = {
    //   x      y     z      u     v
        -0.7f, -0.7f, 0.0f,  0.0f, 0.0f,   // 0 左下
         0.7f, -0.7f, 0.0f,  1.0f, 0.0f,   // 1 右下
         0.7f,  0.7f, 0.0f,  1.0f, 1.0f,   // 2 右上
        -0.7f,  0.7f, 0.0f,  0.0f, 1.0f,   // 3 左上
    };

    // EBO 索引：用两个三角形拼出矩形，复用顶点 0 和 2
    unsigned int indices[] = {
        0, 1, 2,   // 右下三角形
        0, 2, 3,   // 左上三角形
    };

    // VAO / VBO / EBO
    GLuint VAO, VBO, EBO;
    glGenVertexArrays(1, &VAO);
    glGenBuffers(1, &VBO);
    glGenBuffers(1, &EBO);   // EBO 也是 Buffer 对象，用不同的 Target 区分

    glBindVertexArray(VAO);

    glBindBuffer(GL_ARRAY_BUFFER, VBO);
    glBufferData(GL_ARRAY_BUFFER, sizeof(vertices), vertices, GL_STATIC_DRAW);

    // VAO 同时也会记录当前绑定的 EBO
    glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, EBO);
    glBufferData(GL_ELEMENT_ARRAY_BUFFER, sizeof(indices), indices, GL_STATIC_DRAW);

    // 属性0：位置（每顶点 5 float，偏移 0）
    glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 5 * sizeof(float), (void*)0);
    glEnableVertexAttribArray(0);

    // 属性1：UV（每顶点 5 float，偏移 3*float）
    glVertexAttribPointer(1, 2, GL_FLOAT, GL_FALSE, 5 * sizeof(float), (void*)(3 * sizeof(float)));
    glEnableVertexAttribArray(1);

    glBindVertexArray(0);

    // 告诉着色器 uTexture 对应纹理单元 0
    glUseProgram(program);
    glUniform1i(glGetUniformLocation(program, "uTexture"), 0);

    // 渲染循环
    while (!glfwWindowShouldClose(window)) {
        if (glfwGetKey(window, GLFW_KEY_ESCAPE) == GLFW_PRESS)
            glfwSetWindowShouldClose(window, true);

        glClearColor(0.1f, 0.1f, 0.2f, 1.0f);
        glClear(GL_COLOR_BUFFER_BIT);

        glUseProgram(program);

        // 激活纹理单元 0，并把纹理对象绑定到它
        // 纹理单元是连接 GPU 纹理对象与着色器 sampler2D 的桥梁
        glActiveTexture(GL_TEXTURE0);
        glBindTexture(GL_TEXTURE_2D, texture);

        glBindVertexArray(VAO);
        // glDrawElements 替代 glDrawArrays，通过 EBO 里的索引取顶点
        // 参数：图元类型, 索引数量, 索引数据类型, 偏移（EBO 已绑定，传 nullptr）
        glDrawElements(GL_TRIANGLES, 6, GL_UNSIGNED_INT, nullptr);

        glfwSwapBuffers(window);
        glfwPollEvents();
    }

    glDeleteVertexArrays(1, &VAO);
    glDeleteBuffers(1, &VBO);
    glDeleteBuffers(1, &EBO);
    glDeleteTextures(1, &texture);
    glDeleteProgram(program);
    glfwDestroyWindow(window);
    glfwTerminate();
    return 0;
}

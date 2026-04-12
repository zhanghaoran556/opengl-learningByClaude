#include <GL/glew.h>
#include <GLFW/glfw3.h>

// GLM：OpenGL Mathematics，专为图形设计的数学库
// 头文件按功能拆分，只引入需要的部分
#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>  // translate / rotate / scale / perspective
#include <glm/gtc/type_ptr.hpp>          // value_ptr：把 glm 矩阵转成 float* 传给 OpenGL

#include <fstream>
#include <iostream>
#include <sstream>
#include <string>
#include <vector>

const int WIDTH  = 800;
const int HEIGHT = 600;

// ─── 工具函数 ─────────────────────────────────────────────────────────────────

std::string loadFile(const std::string& path) {
    std::ifstream f(path);
    if (!f.is_open()) { std::cerr << "Cannot open: " << path << "\n"; return ""; }
    std::ostringstream ss; ss << f.rdbuf(); return ss.str();
}

GLuint compileShader(GLenum type, const std::string& src) {
    GLuint s = glCreateShader(type);
    const char* c = src.c_str();
    glShaderSource(s, 1, &c, nullptr); glCompileShader(s);
    int ok; glGetShaderiv(s, GL_COMPILE_STATUS, &ok);
    if (!ok) { char log[512]; glGetShaderInfoLog(s,512,nullptr,log); std::cerr<<log<<"\n"; }
    return s;
}

GLuint createProgram(GLuint vert, GLuint frag) {
    GLuint p = glCreateProgram();
    glAttachShader(p, vert); glAttachShader(p, frag); glLinkProgram(p);
    int ok; glGetProgramiv(p, GL_LINK_STATUS, &ok);
    if (!ok) { char log[512]; glGetProgramInfoLog(p,512,nullptr,log); std::cerr<<log<<"\n"; }
    return p;
}

std::vector<unsigned char> makeCheckerboard(int w, int h, int cell) {
    std::vector<unsigned char> px(w * h * 3);
    for (int y = 0; y < h; ++y)
        for (int x = 0; x < w; ++x) {
            unsigned char c = ((x/cell + y/cell) % 2 == 0) ? 230 : 60;
            int i = (y*w+x)*3; px[i]=c; px[i+1]=c; px[i+2]=c;
        }
    return px;
}

GLuint createTexture() {
    GLuint tex; glGenTextures(1, &tex); glBindTexture(GL_TEXTURE_2D, tex);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_REPEAT);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_REPEAT);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR_MIPMAP_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
    auto px = makeCheckerboard(128, 128, 16);
    glTexImage2D(GL_TEXTURE_2D, 0, GL_RGB, 128, 128, 0, GL_RGB, GL_UNSIGNED_BYTE, px.data());
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

    GLFWwindow* window = glfwCreateWindow(WIDTH, HEIGHT, "Transformations", nullptr, nullptr);
    glfwMakeContextCurrent(window);
    glewExperimental = GL_TRUE; glewInit();
    glViewport(0, 0, WIDTH, HEIGHT);

    GLuint vert    = compileShader(GL_VERTEX_SHADER,   loadFile("src/05_transformations/vertex.glsl"));
    GLuint frag    = compileShader(GL_FRAGMENT_SHADER, loadFile("src/05_transformations/fragment.glsl"));
    GLuint program = createProgram(vert, frag);
    glDeleteShader(vert); glDeleteShader(frag);

    GLuint texture = createTexture();

    // 矩形顶点：位置(xyz) + UV(uv)
    float vertices[] = {
        -0.5f, -0.5f, 0.0f,  0.0f, 0.0f,
         0.5f, -0.5f, 0.0f,  1.0f, 0.0f,
         0.5f,  0.5f, 0.0f,  1.0f, 1.0f,
        -0.5f,  0.5f, 0.0f,  0.0f, 1.0f,
    };
    unsigned int indices[] = { 0,1,2, 0,2,3 };

    GLuint VAO, VBO, EBO;
    glGenVertexArrays(1, &VAO); glGenBuffers(1, &VBO); glGenBuffers(1, &EBO);
    glBindVertexArray(VAO);
    glBindBuffer(GL_ARRAY_BUFFER, VBO);
    glBufferData(GL_ARRAY_BUFFER, sizeof(vertices), vertices, GL_STATIC_DRAW);
    glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, EBO);
    glBufferData(GL_ELEMENT_ARRAY_BUFFER, sizeof(indices), indices, GL_STATIC_DRAW);
    glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 5*sizeof(float), (void*)0);
    glEnableVertexAttribArray(0);
    glVertexAttribPointer(1, 2, GL_FLOAT, GL_FALSE, 5*sizeof(float), (void*)(3*sizeof(float)));
    glEnableVertexAttribArray(1);
    glBindVertexArray(0);

    glUseProgram(program);
    glUniform1i(glGetUniformLocation(program, "uTexture"), 0);
    GLint modelLoc = glGetUniformLocation(program, "uModel");

    while (!glfwWindowShouldClose(window)) {
        if (glfwGetKey(window, GLFW_KEY_ESCAPE) == GLFW_PRESS)
            glfwSetWindowShouldClose(window, true);

        glClearColor(0.1f, 0.1f, 0.2f, 1.0f);
        glClear(GL_COLOR_BUFFER_BIT);

        glUseProgram(program);
        glActiveTexture(GL_TEXTURE0);
        glBindTexture(GL_TEXTURE_2D, texture);

        float t = (float)glfwGetTime();

        // ── 变换 1：旋转 + 缩放的矩形（右上）────────────────────────────────
        // glm::mat4(1.0f) 创建单位矩阵（什么都不做的变换，相当于乘以1）
        // 变换按"从下往上"读：先缩放，再旋转，再平移
        glm::mat4 model = glm::mat4(1.0f);
        model = glm::translate(model, glm::vec3(0.5f, 0.5f, 0.0f));   // 平移到右上
        model = glm::rotate(model, t, glm::vec3(0.0f, 0.0f, 1.0f));   // 绕 Z 轴旋转，t 是弧度
        model = glm::scale(model, glm::vec3(0.5f, 0.5f, 1.0f));       // 缩放到原来的 50%

        // 把 glm 矩阵传给着色器：
        // glUniformMatrix4fv(location, 矩阵数量, 是否转置, float*指针)
        // glm 是列主序，OpenGL 也是列主序，不需要转置（GL_FALSE）
        glUniformMatrix4fv(modelLoc, 1, GL_FALSE, glm::value_ptr(model));

        glBindVertexArray(VAO);
        glDrawElements(GL_TRIANGLES, 6, GL_UNSIGNED_INT, nullptr);

        // ── 变换 2：只缩放的矩形（左下），大小随时间呼吸 ─────────────────────
        float breathe = 0.3f + 0.15f * glm::sin(t * 2.0f);   // 大小在 [0.15, 0.45] 间脉动
        model = glm::mat4(1.0f);
        model = glm::translate(model, glm::vec3(-0.5f, -0.5f, 0.0f));
        model = glm::scale(model, glm::vec3(breathe, breathe, 1.0f));

        glUniformMatrix4fv(modelLoc, 1, GL_FALSE, glm::value_ptr(model));
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

#include <GL/glew.h>
#include <GLFW/glfw3.h>
#include <iostream>

// 窗口尺寸
const int WIDTH  = 800;
const int HEIGHT = 600;

// 当用户按下 ESC 时关闭窗口
void processInput(GLFWwindow* window) {
    if (glfwGetKey(window, GLFW_KEY_ESCAPE) == GLFW_PRESS)
        glfwSetWindowShouldClose(window, true);
}

int main() {
    // 1. 初始化 GLFW
    if (!glfwInit()) {
        std::cerr << "Failed to initialize GLFW\n";
        return -1;
    }

    // 2. 指定 OpenGL 版本：Core Profile 3.3
    glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 3);
    glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 3);
    glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE);
    glfwWindowHint(GLFW_OPENGL_FORWARD_COMPAT, GL_TRUE); // macOS 必须

    // 3. 创建窗口
    GLFWwindow* window = glfwCreateWindow(WIDTH, HEIGHT, "Hello OpenGL", nullptr, nullptr);
    if (!window) {
        std::cerr << "Failed to create GLFW window\n";
        glfwTerminate();
        return -1;
    }
    glfwMakeContextCurrent(window); // 将 OpenGL 上下文绑定到当前线程

    // 4. 初始化 GLEW（加载 OpenGL 函数指针）
    glewExperimental = GL_TRUE;
    if (glewInit() != GLEW_OK) {
        std::cerr << "Failed to initialize GLEW\n";
        return -1;
    }

    // 打印当前 OpenGL 版本，确认环境正常
    std::cout << "OpenGL Version: " << glGetString(GL_VERSION) << "\n";
    std::cout << "Renderer: "       << glGetString(GL_RENDERER) << "\n";

    // 5. 设置视口（Viewport）：告诉 OpenGL 渲染区域的大小
    glViewport(0, 0, WIDTH, HEIGHT);

    // 6. 渲染循环
    while (!glfwWindowShouldClose(window)) {
        processInput(window);

        // 清空颜色缓冲，用深蓝色填充背景
        glClearColor(0.1f, 0.1f, 0.2f, 1.0f);
        glClear(GL_COLOR_BUFFER_BIT);

        // 交换前后缓冲区（双缓冲机制）
        glfwSwapBuffers(window);
        // 处理键盘/鼠标等事件
        glfwPollEvents();
    }

    // 7. 清理资源
    glfwDestroyWindow(window);
    glfwTerminate();
    return 0;
}

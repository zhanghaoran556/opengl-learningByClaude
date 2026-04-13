#include <GL/glew.h>
#include <GLFW/glfw3.h>
#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtc/type_ptr.hpp>
#include <fstream>
#include <iostream>
#include <sstream>
#include <string>
#include <vector>

const int WIDTH  = 800;
const int HEIGHT = 600;

// ─── 摄像机状态 ───────────────────────────────────────────────────────────────
// 用三个向量描述摄像机在世界空间中的姿态

glm::vec3 camPos   = glm::vec3(0.0f, 0.0f,  3.0f);  // 摄像机位置
glm::vec3 camFront = glm::vec3(0.0f, 0.0f, -1.0f);  // 朝向（单位向量）
glm::vec3 camUp    = glm::vec3(0.0f, 1.0f,  0.0f);  // 上方向（世界上方）

// 欧拉角：用偏航角(yaw) + 俯仰角(pitch) 描述朝向，比存 front 向量更方便鼠标控制
float yaw   = -90.0f;   // 初始朝向 -Z 轴，yaw=0 时朝 +X，所以初始值 -90
float pitch =   0.0f;

// 鼠标状态
float lastX      = WIDTH  / 2.0f;
float lastY      = HEIGHT / 2.0f;
bool  firstMouse = true;   // 第一次收到鼠标事件时不计算偏移（避免视角跳变）

// 帧时间：用于让移动速度与帧率无关
float deltaTime = 0.0f;
float lastFrame = 0.0f;

// ─── 鼠标回调：每次鼠标移动时由 GLFW 调用 ─────────────────────────────────────

void mouseCallback(GLFWwindow*, double xpos, double ypos) {
    if (firstMouse) {
        lastX = (float)xpos;
        lastY = (float)ypos;
        firstMouse = false;
    }

    float dx = (float)xpos - lastX;
    float dy = lastY - (float)ypos;   // 注意：屏幕 Y 轴向下，这里反转使向上看为正
    lastX = (float)xpos;
    lastY = (float)ypos;

    const float sensitivity = 0.1f;
    yaw   += dx * sensitivity;
    pitch += dy * sensitivity;

    // 限制俯仰角，防止万向节死锁（翻转问题）
    if (pitch >  89.0f) pitch =  89.0f;
    if (pitch < -89.0f) pitch = -89.0f;

    // 从欧拉角计算朝向向量
    glm::vec3 front;
    front.x = glm::cos(glm::radians(yaw)) * glm::cos(glm::radians(pitch));
    front.y = glm::sin(glm::radians(pitch));
    front.z = glm::sin(glm::radians(yaw)) * glm::cos(glm::radians(pitch));
    camFront = glm::normalize(front);
}

// ─── 键盘输入处理 ──────────────────────────────────────────────────────────────

void processInput(GLFWwindow* window) {
    const float speed = 2.5f * deltaTime;   // 速度乘以帧时间 → 与帧率无关

    if (glfwGetKey(window, GLFW_KEY_ESCAPE) == GLFW_PRESS)
        glfwSetWindowShouldClose(window, true);

    // WASD：在水平面内移动（front 投影到 XZ 平面）
    if (glfwGetKey(window, GLFW_KEY_W) == GLFW_PRESS)
        camPos += speed * camFront;
    if (glfwGetKey(window, GLFW_KEY_S) == GLFW_PRESS)
        camPos -= speed * camFront;
    // 叉积得到摄像机的右方向向量，归一化后保证速度一致
    if (glfwGetKey(window, GLFW_KEY_A) == GLFW_PRESS)
        camPos -= glm::normalize(glm::cross(camFront, camUp)) * speed;
    if (glfwGetKey(window, GLFW_KEY_D) == GLFW_PRESS)
        camPos += glm::normalize(glm::cross(camFront, camUp)) * speed;

    // 空格/左Shift：垂直上下移动
    if (glfwGetKey(window, GLFW_KEY_SPACE) == GLFW_PRESS)
        camPos += speed * camUp;
    if (glfwGetKey(window, GLFW_KEY_LEFT_SHIFT) == GLFW_PRESS)
        camPos -= speed * camUp;
}

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
    glAttachShader(p,vert); glAttachShader(p,frag); glLinkProgram(p);
    int ok; glGetProgramiv(p, GL_LINK_STATUS, &ok);
    if (!ok) { char log[512]; glGetProgramInfoLog(p,512,nullptr,log); std::cerr<<log<<"\n"; }
    return p;
}

GLuint createTexture() {
    const int W = 128, H = 128, CELL = 16;
    std::vector<unsigned char> px(W*H*3);
    for (int y = 0; y < H; ++y)
        for (int x = 0; x < W; ++x) {
            unsigned char c = ((x/CELL + y/CELL) % 2 == 0) ? 220 : 60;
            int i=(y*W+x)*3; px[i]=c; px[i+1]=c; px[i+2]=c;
        }
    GLuint tex; glGenTextures(1,&tex); glBindTexture(GL_TEXTURE_2D,tex);
    glTexParameteri(GL_TEXTURE_2D,GL_TEXTURE_WRAP_S,GL_REPEAT);
    glTexParameteri(GL_TEXTURE_2D,GL_TEXTURE_WRAP_T,GL_REPEAT);
    glTexParameteri(GL_TEXTURE_2D,GL_TEXTURE_MIN_FILTER,GL_LINEAR_MIPMAP_LINEAR);
    glTexParameteri(GL_TEXTURE_2D,GL_TEXTURE_MAG_FILTER,GL_NEAREST);
    glTexImage2D(GL_TEXTURE_2D,0,GL_RGB,W,H,0,GL_RGB,GL_UNSIGNED_BYTE,px.data());
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

    GLFWwindow* window = glfwCreateWindow(WIDTH, HEIGHT, "Camera (WASD + Mouse)", nullptr, nullptr);
    glfwMakeContextCurrent(window);

    // 隐藏光标，鼠标锁定在窗口内（FPS 风格）
    glfwSetInputMode(window, GLFW_CURSOR, GLFW_CURSOR_DISABLED);
    // 注册鼠标移动回调
    glfwSetCursorPosCallback(window, mouseCallback);

    glewExperimental = GL_TRUE; glewInit();
    glViewport(0, 0, WIDTH, HEIGHT);

    // 开启深度测试：GPU 按深度值决定哪个像素在前面，避免近处物体被远处遮挡
    glEnable(GL_DEPTH_TEST);

    GLuint vert    = compileShader(GL_VERTEX_SHADER,   loadFile("src/06_camera/vertex.glsl"));
    GLuint frag    = compileShader(GL_FRAGMENT_SHADER, loadFile("src/06_camera/fragment.glsl"));
    GLuint program = createProgram(vert, frag);
    glDeleteShader(vert); glDeleteShader(frag);

    GLuint texture = createTexture();

    // 正方体：36 个顶点（6 面 × 2 三角形 × 3 顶点），不用 EBO
    // 每个顶点：xyz + uv（5 float）
    float cubeVerts[] = {
        // 后面 (-Z)
        -0.5f,-0.5f,-0.5f, 0,0,   0.5f,-0.5f,-0.5f, 1,0,   0.5f, 0.5f,-0.5f, 1,1,
         0.5f, 0.5f,-0.5f, 1,1,  -0.5f, 0.5f,-0.5f, 0,1,  -0.5f,-0.5f,-0.5f, 0,0,
        // 前面 (+Z)
        -0.5f,-0.5f, 0.5f, 0,0,   0.5f,-0.5f, 0.5f, 1,0,   0.5f, 0.5f, 0.5f, 1,1,
         0.5f, 0.5f, 0.5f, 1,1,  -0.5f, 0.5f, 0.5f, 0,1,  -0.5f,-0.5f, 0.5f, 0,0,
        // 左面 (-X)
        -0.5f, 0.5f, 0.5f, 1,1,  -0.5f, 0.5f,-0.5f, 0,1,  -0.5f,-0.5f,-0.5f, 0,0,
        -0.5f,-0.5f,-0.5f, 0,0,  -0.5f,-0.5f, 0.5f, 1,0,  -0.5f, 0.5f, 0.5f, 1,1,
        // 右面 (+X)
         0.5f, 0.5f, 0.5f, 1,1,   0.5f, 0.5f,-0.5f, 0,1,   0.5f,-0.5f,-0.5f, 0,0,
         0.5f,-0.5f,-0.5f, 0,0,   0.5f,-0.5f, 0.5f, 1,0,   0.5f, 0.5f, 0.5f, 1,1,
        // 底面 (-Y)
        -0.5f,-0.5f,-0.5f, 0,1,   0.5f,-0.5f,-0.5f, 1,1,   0.5f,-0.5f, 0.5f, 1,0,
         0.5f,-0.5f, 0.5f, 1,0,  -0.5f,-0.5f, 0.5f, 0,0,  -0.5f,-0.5f,-0.5f, 0,1,
        // 顶面 (+Y)
        -0.5f, 0.5f,-0.5f, 0,1,   0.5f, 0.5f,-0.5f, 1,1,   0.5f, 0.5f, 0.5f, 1,0,
         0.5f, 0.5f, 0.5f, 1,0,  -0.5f, 0.5f, 0.5f, 0,0,  -0.5f, 0.5f,-0.5f, 0,1,
    };

    GLuint VAO, VBO;
    glGenVertexArrays(1, &VAO); glGenBuffers(1, &VBO);
    glBindVertexArray(VAO);
    glBindBuffer(GL_ARRAY_BUFFER, VBO);
    glBufferData(GL_ARRAY_BUFFER, sizeof(cubeVerts), cubeVerts, GL_STATIC_DRAW);
    glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 5*sizeof(float), (void*)0);
    glEnableVertexAttribArray(0);
    glVertexAttribPointer(1, 2, GL_FLOAT, GL_FALSE, 5*sizeof(float), (void*)(3*sizeof(float)));
    glEnableVertexAttribArray(1);
    glBindVertexArray(0);

    // 10 个正方体的世界坐标
    glm::vec3 cubePositions[] = {
        { 0.0f,  0.0f,  0.0f}, { 2.0f,  5.0f, -15.0f},
        {-1.5f, -2.2f, -2.5f}, {-3.8f, -2.0f, -12.3f},
        { 2.4f, -0.4f, -3.5f}, {-1.7f,  3.0f, -7.5f},
        { 1.3f, -2.0f, -2.5f}, { 1.5f,  2.0f, -2.5f},
        { 1.5f,  0.2f, -1.5f}, {-1.3f,  1.0f, -1.5f},
    };

    glUseProgram(program);
    glUniform1i(glGetUniformLocation(program, "uTexture"), 0);
    GLint modelLoc = glGetUniformLocation(program, "uModel");
    GLint viewLoc  = glGetUniformLocation(program, "uView");
    GLint projLoc  = glGetUniformLocation(program, "uProjection");

    // 投影矩阵只需设置一次（窗口大小不变时不需要每帧重算）
    // perspective(垂直视角FOV, 宽高比, 近裁剪面, 远裁剪面)
    glm::mat4 projection = glm::perspective(
        glm::radians(45.0f),              // 垂直视角 45°
        (float)WIDTH / (float)HEIGHT,     // 宽高比
        0.1f,                             // 近裁剪面：比这近的东西不渲染
        100.0f                            // 远裁剪面：比这远的东西不渲染
    );
    glUniformMatrix4fv(projLoc, 1, GL_FALSE, glm::value_ptr(projection));

    while (!glfwWindowShouldClose(window)) {
        // 计算帧时间差，用于速度归一化
        float currentFrame = (float)glfwGetTime();
        deltaTime = currentFrame - lastFrame;
        lastFrame = currentFrame;

        processInput(window);

        // 深度缓冲也要每帧清空，否则上帧的深度值会影响这帧的遮挡判断
        glClearColor(0.1f, 0.15f, 0.2f, 1.0f);
        glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

        glUseProgram(program);
        glActiveTexture(GL_TEXTURE0);
        glBindTexture(GL_TEXTURE_2D, texture);

        // View 矩阵：lookAt(摄像机位置, 看向的目标点, 世界上方向)
        glm::mat4 view = glm::lookAt(camPos, camPos + camFront, camUp);
        glUniformMatrix4fv(viewLoc, 1, GL_FALSE, glm::value_ptr(view));

        // 每个正方体用不同的 Model 矩阵放到世界不同位置，并以不同角速度旋转
        glBindVertexArray(VAO);
        for (int i = 0; i < 10; ++i) {
            glm::mat4 model = glm::mat4(1.0f);
            model = glm::translate(model, cubePositions[i]);
            // 每个 cube 旋转轴和速度略有不同，制造层次感
            float angle = (float)glfwGetTime() * glm::radians(20.0f + i * 7.0f);
            model = glm::rotate(model, angle, glm::vec3(1.0f, 0.3f * i, 0.5f));
            glUniformMatrix4fv(modelLoc, 1, GL_FALSE, glm::value_ptr(model));
            glDrawArrays(GL_TRIANGLES, 0, 36);
        }

        glfwSwapBuffers(window);
        glfwPollEvents();
    }

    glDeleteVertexArrays(1, &VAO);
    glDeleteBuffers(1, &VBO);
    glDeleteTextures(1, &texture);
    glDeleteProgram(program);
    glfwDestroyWindow(window);
    glfwTerminate();
    return 0;
}

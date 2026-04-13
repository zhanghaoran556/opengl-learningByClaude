#include <GL/glew.h>
#include <GLFW/glfw3.h>
#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtc/type_ptr.hpp>
#include <fstream>
#include <iostream>
#include <sstream>
#include <string>

const int WIDTH  = 800;
const int HEIGHT = 600;

// ─── 摄像机（与 Lesson 06 相同）──────────────────────────────────────────────

glm::vec3 camPos   = {0.0f, 1.0f,  6.0f};
glm::vec3 camFront = {0.0f, 0.0f, -1.0f};
glm::vec3 camUp    = {0.0f, 1.0f,  0.0f};
float yaw = -90.0f, pitch = 0.0f;
float lastX = WIDTH/2.0f, lastY = HEIGHT/2.0f;
bool firstMouse = true;
float deltaTime = 0.0f, lastFrame = 0.0f;

void mouseCallback(GLFWwindow*, double xpos, double ypos) {
    if (firstMouse) { lastX=(float)xpos; lastY=(float)ypos; firstMouse=false; }
    float dx=(float)xpos-lastX, dy=lastY-(float)ypos;
    lastX=(float)xpos; lastY=(float)ypos;
    yaw+=dx*0.1f; pitch+=dy*0.1f;
    pitch=glm::clamp(pitch,-89.0f,89.0f);
    camFront=glm::normalize(glm::vec3{
        glm::cos(glm::radians(yaw))*glm::cos(glm::radians(pitch)),
        glm::sin(glm::radians(pitch)),
        glm::sin(glm::radians(yaw))*glm::cos(glm::radians(pitch))});
}

void processInput(GLFWwindow* w) {
    float s = 2.5f*deltaTime;
    if (glfwGetKey(w,GLFW_KEY_ESCAPE)==GLFW_PRESS) glfwSetWindowShouldClose(w,true);
    if (glfwGetKey(w,GLFW_KEY_W)==GLFW_PRESS) camPos+=s*camFront;
    if (glfwGetKey(w,GLFW_KEY_S)==GLFW_PRESS) camPos-=s*camFront;
    if (glfwGetKey(w,GLFW_KEY_A)==GLFW_PRESS) camPos-=glm::normalize(glm::cross(camFront,camUp))*s;
    if (glfwGetKey(w,GLFW_KEY_D)==GLFW_PRESS) camPos+=glm::normalize(glm::cross(camFront,camUp))*s;
    if (glfwGetKey(w,GLFW_KEY_SPACE)==GLFW_PRESS) camPos+=s*camUp;
    if (glfwGetKey(w,GLFW_KEY_LEFT_SHIFT)==GLFW_PRESS) camPos-=s*camUp;
}

// ─── 工具函数 ─────────────────────────────────────────────────────────────────

std::string loadFile(const std::string& p) {
    std::ifstream f(p);
    if(!f.is_open()){std::cerr<<"Cannot open: "<<p<<"\n";return"";}
    std::ostringstream ss; ss<<f.rdbuf(); return ss.str();
}
GLuint compileShader(GLenum t, const std::string& s) {
    GLuint sh=glCreateShader(t); const char* c=s.c_str();
    glShaderSource(sh,1,&c,nullptr); glCompileShader(sh);
    int ok; glGetShaderiv(sh,GL_COMPILE_STATUS,&ok);
    if(!ok){char log[512];glGetShaderInfoLog(sh,512,nullptr,log);std::cerr<<log<<"\n";}
    return sh;
}
GLuint createProgram(const std::string& vp, const std::string& fp) {
    GLuint v=compileShader(GL_VERTEX_SHADER,loadFile(vp));
    GLuint f=compileShader(GL_FRAGMENT_SHADER,loadFile(fp));
    GLuint p=glCreateProgram();
    glAttachShader(p,v); glAttachShader(p,f); glLinkProgram(p);
    int ok; glGetProgramiv(p,GL_LINK_STATUS,&ok);
    if(!ok){char log[512];glGetProgramInfoLog(p,512,nullptr,log);std::cerr<<log<<"\n";}
    glDeleteShader(v); glDeleteShader(f);
    return p;
}

// ─── 材质定义 ─────────────────────────────────────────────────────────────────

struct Material {
    glm::vec3 ambient, diffuse, specular;
    float shininess;
};

// 几种经典材质参数（来自 OpenGL 材质数据库）
const Material GOLD        = {{0.247f,0.200f,0.075f},{0.752f,0.606f,0.226f},{0.628f,0.556f,0.366f},51.2f};
const Material CYAN_PLASTIC= {{0.000f,0.100f,0.060f},{0.000f,0.510f,0.510f},{0.502f,0.502f,0.502f},32.0f};
const Material RED_RUBBER  = {{0.050f,0.000f,0.000f},{0.500f,0.400f,0.400f},{0.700f,0.040f,0.040f},10.0f};
const Material SILVER      = {{0.192f,0.192f,0.192f},{0.508f,0.508f,0.508f},{0.508f,0.508f,0.508f},51.2f};

// 设置材质 uniform
void setMaterial(GLuint prog, const Material& m) {
    glUniform3fv(glGetUniformLocation(prog,"uMatAmbient"),  1,glm::value_ptr(m.ambient));
    glUniform3fv(glGetUniformLocation(prog,"uMatDiffuse"),  1,glm::value_ptr(m.diffuse));
    glUniform3fv(glGetUniformLocation(prog,"uMatSpecular"), 1,glm::value_ptr(m.specular));
    glUniform1f (glGetUniformLocation(prog,"uMatShininess"),m.shininess);
}

// ─── main ────────────────────────────────────────────────────────────────────

int main() {
    glfwInit();
    glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR,3);
    glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR,3);
    glfwWindowHint(GLFW_OPENGL_PROFILE,GLFW_OPENGL_CORE_PROFILE);
    glfwWindowHint(GLFW_OPENGL_FORWARD_COMPAT,GL_TRUE);

    GLFWwindow* window=glfwCreateWindow(WIDTH,HEIGHT,"Lighting (Phong)",nullptr,nullptr);
    glfwMakeContextCurrent(window);
    glfwSetInputMode(window,GLFW_CURSOR,GLFW_CURSOR_DISABLED);
    glfwSetCursorPosCallback(window,mouseCallback);
    glewExperimental=GL_TRUE; glewInit();
    glViewport(0,0,WIDTH,HEIGHT);
    glEnable(GL_DEPTH_TEST);

    GLuint litProg  = createProgram("src/07_lighting/lit.vert",  "src/07_lighting/lit.frag");
    GLuint lampProg = createProgram("src/07_lighting/lamp.vert", "src/07_lighting/lamp.frag");

    // 正方体顶点：xyz + 法线 xyz（6 float / 顶点，36 顶点）
    float verts[] = {
        // 后面               法线(0,0,-1)
        -0.5f,-0.5f,-0.5f, 0,0,-1,   0.5f,-0.5f,-0.5f, 0,0,-1,   0.5f, 0.5f,-0.5f, 0,0,-1,
         0.5f, 0.5f,-0.5f, 0,0,-1,  -0.5f, 0.5f,-0.5f, 0,0,-1,  -0.5f,-0.5f,-0.5f, 0,0,-1,
        // 前面               法线(0,0,1)
        -0.5f,-0.5f, 0.5f, 0,0,1,    0.5f,-0.5f, 0.5f, 0,0,1,    0.5f, 0.5f, 0.5f, 0,0,1,
         0.5f, 0.5f, 0.5f, 0,0,1,   -0.5f, 0.5f, 0.5f, 0,0,1,   -0.5f,-0.5f, 0.5f, 0,0,1,
        // 左面               法线(-1,0,0)
        -0.5f, 0.5f, 0.5f,-1,0,0,   -0.5f, 0.5f,-0.5f,-1,0,0,   -0.5f,-0.5f,-0.5f,-1,0,0,
        -0.5f,-0.5f,-0.5f,-1,0,0,   -0.5f,-0.5f, 0.5f,-1,0,0,   -0.5f, 0.5f, 0.5f,-1,0,0,
        // 右面               法线(1,0,0)
         0.5f, 0.5f, 0.5f, 1,0,0,    0.5f, 0.5f,-0.5f, 1,0,0,    0.5f,-0.5f,-0.5f, 1,0,0,
         0.5f,-0.5f,-0.5f, 1,0,0,    0.5f,-0.5f, 0.5f, 1,0,0,    0.5f, 0.5f, 0.5f, 1,0,0,
        // 底面               法线(0,-1,0)
        -0.5f,-0.5f,-0.5f, 0,-1,0,   0.5f,-0.5f,-0.5f, 0,-1,0,   0.5f,-0.5f, 0.5f, 0,-1,0,
         0.5f,-0.5f, 0.5f, 0,-1,0,  -0.5f,-0.5f, 0.5f, 0,-1,0,  -0.5f,-0.5f,-0.5f, 0,-1,0,
        // 顶面               法线(0,1,0)
        -0.5f, 0.5f,-0.5f, 0,1,0,    0.5f, 0.5f,-0.5f, 0,1,0,    0.5f, 0.5f, 0.5f, 0,1,0,
         0.5f, 0.5f, 0.5f, 0,1,0,   -0.5f, 0.5f, 0.5f, 0,1,0,   -0.5f, 0.5f,-0.5f, 0,1,0,
    };

    // 被光照的物体：需要位置(loc=0)和法线(loc=1)
    GLuint litVAO, VBO;
    glGenVertexArrays(1,&litVAO); glGenBuffers(1,&VBO);
    glBindVertexArray(litVAO);
    glBindBuffer(GL_ARRAY_BUFFER,VBO);
    glBufferData(GL_ARRAY_BUFFER,sizeof(verts),verts,GL_STATIC_DRAW);
    glVertexAttribPointer(0,3,GL_FLOAT,GL_FALSE,6*sizeof(float),(void*)0);
    glEnableVertexAttribArray(0);
    glVertexAttribPointer(1,3,GL_FLOAT,GL_FALSE,6*sizeof(float),(void*)(3*sizeof(float)));
    glEnableVertexAttribArray(1);

    // 光源小方块：只需位置，共享同一个 VBO
    GLuint lampVAO;
    glGenVertexArrays(1,&lampVAO);
    glBindVertexArray(lampVAO);
    glBindBuffer(GL_ARRAY_BUFFER,VBO);   // 复用同一 VBO，法线数据有但不用
    glVertexAttribPointer(0,3,GL_FLOAT,GL_FALSE,6*sizeof(float),(void*)0);
    glEnableVertexAttribArray(0);
    glBindVertexArray(0);

    // 4 个物体的位置和对应材质
    glm::vec3 positions[] = {{-2,0,0},{0,0,0},{2,0,0},{0,2,0}};
    const Material* mats[]= {&GOLD, &CYAN_PLASTIC, &RED_RUBBER, &SILVER};

    glm::mat4 projection = glm::perspective(glm::radians(45.0f),(float)WIDTH/HEIGHT,0.1f,100.0f);

    while (!glfwWindowShouldClose(window)) {
        float t = (float)glfwGetTime();
        deltaTime=t-lastFrame; lastFrame=t;
        processInput(window);

        glClearColor(0.05f,0.05f,0.1f,1.0f);
        glClear(GL_COLOR_BUFFER_BIT|GL_DEPTH_BUFFER_BIT);

        glm::mat4 view = glm::lookAt(camPos, camPos+camFront, camUp);

        // 光源绕 Y 轴公转，高度随时间上下浮动
        glm::vec3 lightPos = {
            3.0f * glm::cos(t * 0.8f),
            1.5f * glm::sin(t * 0.5f),
            3.0f * glm::sin(t * 0.8f)
        };
        glm::vec3 lightColor = {1.0f, 1.0f, 1.0f};

        // ── 绘制被光照的物体 ───────────────────────────────────────────────────
        glUseProgram(litProg);
        glUniformMatrix4fv(glGetUniformLocation(litProg,"uView"),       1,GL_FALSE,glm::value_ptr(view));
        glUniformMatrix4fv(glGetUniformLocation(litProg,"uProjection"), 1,GL_FALSE,glm::value_ptr(projection));
        glUniform3fv(glGetUniformLocation(litProg,"uLightPos"),   1,glm::value_ptr(lightPos));
        glUniform3fv(glGetUniformLocation(litProg,"uLightColor"), 1,glm::value_ptr(lightColor));
        glUniform3fv(glGetUniformLocation(litProg,"uViewPos"),    1,glm::value_ptr(camPos));

        glBindVertexArray(litVAO);
        for (int i = 0; i < 4; ++i) {
            glm::mat4 model = glm::mat4(1.0f);
            model = glm::translate(model, positions[i]);
            // 法线矩阵：模型矩阵左上 3×3 的逆转置
            // 此处无缩放，逆转置等于原矩阵，但保留此步骤展示正确写法
            glm::mat3 normalMatrix = glm::transpose(glm::inverse(glm::mat3(model)));
            glUniformMatrix4fv(glGetUniformLocation(litProg,"uModel"),        1,GL_FALSE,glm::value_ptr(model));
            glUniformMatrix3fv(glGetUniformLocation(litProg,"uNormalMatrix"),  1,GL_FALSE,glm::value_ptr(normalMatrix));
            setMaterial(litProg, *mats[i]);
            glDrawArrays(GL_TRIANGLES,0,36);
        }

        // ── 绘制光源小方块（纯白，不参与光照计算）────────────────────────────
        glUseProgram(lampProg);
        glUniformMatrix4fv(glGetUniformLocation(lampProg,"uView"),       1,GL_FALSE,glm::value_ptr(view));
        glUniformMatrix4fv(glGetUniformLocation(lampProg,"uProjection"), 1,GL_FALSE,glm::value_ptr(projection));
        glm::mat4 lampModel = glm::scale(glm::translate(glm::mat4(1.0f), lightPos),
                                         glm::vec3(0.2f));
        glUniformMatrix4fv(glGetUniformLocation(lampProg,"uModel"),1,GL_FALSE,glm::value_ptr(lampModel));
        glBindVertexArray(lampVAO);
        glDrawArrays(GL_TRIANGLES,0,36);

        glfwSwapBuffers(window);
        glfwPollEvents();
    }

    glDeleteVertexArrays(1,&litVAO);
    glDeleteVertexArrays(1,&lampVAO);
    glDeleteBuffers(1,&VBO);
    glDeleteProgram(litProg);
    glDeleteProgram(lampProg);
    glfwDestroyWindow(window);
    glfwTerminate();
    return 0;
}

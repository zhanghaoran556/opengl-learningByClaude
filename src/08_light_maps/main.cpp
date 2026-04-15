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
#include <cmath>

const int WIDTH = 800, HEIGHT = 600;

// ─── 摄像机 ───────────────────────────────────────────────────────────────────
glm::vec3 camPos={0,1,4}, camFront={0,0,-1}, camUp={0,1,0};
float yaw=-90,pitch=0,lastX=WIDTH/2.f,lastY=HEIGHT/2.f,deltaTime=0,lastFrame=0;
bool firstMouse=true;
void mouseCallback(GLFWwindow*,double x,double y){
    if(firstMouse){lastX=(float)x;lastY=(float)y;firstMouse=false;}
    float dx=(float)x-lastX,dy=lastY-(float)y;lastX=(float)x;lastY=(float)y;
    yaw+=dx*.1f;pitch=glm::clamp(pitch+dy*.1f,-89.f,89.f);
    camFront=glm::normalize(glm::vec3{
        cos(glm::radians(yaw))*cos(glm::radians(pitch)),
        sin(glm::radians(pitch)),
        sin(glm::radians(yaw))*cos(glm::radians(pitch))});
}
void processInput(GLFWwindow* w){
    float s=2.5f*deltaTime;
    if(glfwGetKey(w,GLFW_KEY_ESCAPE)==GLFW_PRESS)glfwSetWindowShouldClose(w,true);
    if(glfwGetKey(w,GLFW_KEY_W)==GLFW_PRESS)camPos+=s*camFront;
    if(glfwGetKey(w,GLFW_KEY_S)==GLFW_PRESS)camPos-=s*camFront;
    if(glfwGetKey(w,GLFW_KEY_A)==GLFW_PRESS)camPos-=glm::normalize(glm::cross(camFront,camUp))*s;
    if(glfwGetKey(w,GLFW_KEY_D)==GLFW_PRESS)camPos+=glm::normalize(glm::cross(camFront,camUp))*s;
    if(glfwGetKey(w,GLFW_KEY_SPACE)==GLFW_PRESS)camPos+=s*camUp;
    if(glfwGetKey(w,GLFW_KEY_LEFT_SHIFT)==GLFW_PRESS)camPos-=s*camUp;
}

// ─── 工具函数 ─────────────────────────────────────────────────────────────────
std::string loadFile(const std::string& p){
    std::ifstream f(p); if(!f){std::cerr<<"open:"<<p<<"\n";return"";}
    std::ostringstream s;s<<f.rdbuf();return s.str();
}
GLuint compileShader(GLenum t,const std::string& s){
    GLuint sh=glCreateShader(t);const char* c=s.c_str();
    glShaderSource(sh,1,&c,nullptr);glCompileShader(sh);
    int ok;glGetShaderiv(sh,GL_COMPILE_STATUS,&ok);
    if(!ok){char l[512];glGetShaderInfoLog(sh,512,nullptr,l);std::cerr<<l;}
    return sh;
}
GLuint createProgram(const std::string& vp,const std::string& fp){
    GLuint v=compileShader(GL_VERTEX_SHADER,loadFile(vp));
    GLuint f=compileShader(GL_FRAGMENT_SHADER,loadFile(fp));
    GLuint p=glCreateProgram();
    glAttachShader(p,v);glAttachShader(p,f);glLinkProgram(p);
    int ok;glGetProgramiv(p,GL_LINK_STATUS,&ok);
    if(!ok){char l[512];glGetProgramInfoLog(p,512,nullptr,l);std::cerr<<l;}
    glDeleteShader(v);glDeleteShader(f);return p;
}

// ─── 生成纹理 ─────────────────────────────────────────────────────────────────
// 生成一张"橙色面板 + 黑色边框"风格的 diffuse map
// 以及对应的 specular map（只有橙色区域是亮的，边框区域是暗的）

// 橙色面板纹理：每 4×4 格中，中间 3×3 是橙色，外圈是深灰
GLuint makeDiffuseMap() {
    const int W=64, H=64, CELL=16, MARGIN=2;
    std::vector<unsigned char> px(W*H*3);
    for(int y=0;y<H;y++) for(int x=0;x<W;x++){
        int cx=x%CELL, cy=y%CELL;
        bool panel = cx>=MARGIN && cx<CELL-MARGIN && cy>=MARGIN && cy<CELL-MARGIN;
        int i=(y*W+x)*3;
        if(panel){ px[i]=230;px[i+1]=110;px[i+2]=30; }  // 橙色
        else     { px[i]=30; px[i+1]=25; px[i+2]=20; }  // 深棕（木头感）
    }
    GLuint tex;glGenTextures(1,&tex);glBindTexture(GL_TEXTURE_2D,tex);
    glTexParameteri(GL_TEXTURE_2D,GL_TEXTURE_WRAP_S,GL_REPEAT);
    glTexParameteri(GL_TEXTURE_2D,GL_TEXTURE_WRAP_T,GL_REPEAT);
    glTexParameteri(GL_TEXTURE_2D,GL_TEXTURE_MIN_FILTER,GL_LINEAR_MIPMAP_LINEAR);
    glTexParameteri(GL_TEXTURE_2D,GL_TEXTURE_MAG_FILTER,GL_LINEAR);
    glTexImage2D(GL_TEXTURE_2D,0,GL_RGB,W,H,0,GL_RGB,GL_UNSIGNED_BYTE,px.data());
    glGenerateMipmap(GL_TEXTURE_2D);
    return tex;
}

// specular map：橙色面板区域是亮白（有高光），边框区域是纯黑（无高光）
// 这模拟了"金属嵌板 + 哑光木框"的效果
GLuint makeSpecularMap() {
    const int W=64, H=64, CELL=16, MARGIN=2;
    std::vector<unsigned char> px(W*H*3);
    for(int y=0;y<H;y++) for(int x=0;x<W;x++){
        int cx=x%CELL, cy=y%CELL;
        bool panel = cx>=MARGIN && cx<CELL-MARGIN && cy>=MARGIN && cy<CELL-MARGIN;
        unsigned char c = panel ? 230 : 10;   // 亮=有高光，暗=无高光
        int i=(y*W+x)*3;px[i]=px[i+1]=px[i+2]=c;
    }
    GLuint tex;glGenTextures(1,&tex);glBindTexture(GL_TEXTURE_2D,tex);
    glTexParameteri(GL_TEXTURE_2D,GL_TEXTURE_WRAP_S,GL_REPEAT);
    glTexParameteri(GL_TEXTURE_2D,GL_TEXTURE_WRAP_T,GL_REPEAT);
    glTexParameteri(GL_TEXTURE_2D,GL_TEXTURE_MIN_FILTER,GL_LINEAR_MIPMAP_LINEAR);
    glTexParameteri(GL_TEXTURE_2D,GL_TEXTURE_MAG_FILTER,GL_LINEAR);
    glTexImage2D(GL_TEXTURE_2D,0,GL_RGB,W,H,0,GL_RGB,GL_UNSIGNED_BYTE,px.data());
    glGenerateMipmap(GL_TEXTURE_2D);
    return tex;
}

// ─── main ────────────────────────────────────────────────────────────────────
int main(){
    glfwInit();
    glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR,3);
    glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR,3);
    glfwWindowHint(GLFW_OPENGL_PROFILE,GLFW_OPENGL_CORE_PROFILE);
    glfwWindowHint(GLFW_OPENGL_FORWARD_COMPAT,GL_TRUE);
    GLFWwindow* win=glfwCreateWindow(WIDTH,HEIGHT,"Light Maps",nullptr,nullptr);
    glfwMakeContextCurrent(win);
    glfwSetInputMode(win,GLFW_CURSOR,GLFW_CURSOR_DISABLED);
    glfwSetCursorPosCallback(win,mouseCallback);
    glewExperimental=GL_TRUE;glewInit();
    glViewport(0,0,WIDTH,HEIGHT);
    glEnable(GL_DEPTH_TEST);

    GLuint litProg  = createProgram("src/08_light_maps/lit.vert", "src/08_light_maps/lit.frag");
    GLuint lampProg = createProgram("src/08_light_maps/lamp.vert","src/08_light_maps/lamp.frag");

    // 顶点格式：位置(3) + 法线(3) + UV(2) = 8 float
    float verts[] = {
        // 位置                法线         UV
        // 后面
        -0.5f,-0.5f,-0.5f,  0, 0,-1,  0,0,   0.5f,-0.5f,-0.5f,  0, 0,-1,  1,0,
         0.5f, 0.5f,-0.5f,  0, 0,-1,  1,1,   0.5f, 0.5f,-0.5f,  0, 0,-1,  1,1,
        -0.5f, 0.5f,-0.5f,  0, 0,-1,  0,1,  -0.5f,-0.5f,-0.5f,  0, 0,-1,  0,0,
        // 前面
        -0.5f,-0.5f, 0.5f,  0, 0, 1,  0,0,   0.5f,-0.5f, 0.5f,  0, 0, 1,  1,0,
         0.5f, 0.5f, 0.5f,  0, 0, 1,  1,1,   0.5f, 0.5f, 0.5f,  0, 0, 1,  1,1,
        -0.5f, 0.5f, 0.5f,  0, 0, 1,  0,1,  -0.5f,-0.5f, 0.5f,  0, 0, 1,  0,0,
        // 左面
        -0.5f, 0.5f, 0.5f, -1, 0, 0,  1,1,  -0.5f, 0.5f,-0.5f, -1, 0, 0,  0,1,
        -0.5f,-0.5f,-0.5f, -1, 0, 0,  0,0,  -0.5f,-0.5f,-0.5f, -1, 0, 0,  0,0,
        -0.5f,-0.5f, 0.5f, -1, 0, 0,  1,0,  -0.5f, 0.5f, 0.5f, -1, 0, 0,  1,1,
        // 右面
         0.5f, 0.5f, 0.5f,  1, 0, 0,  1,1,   0.5f, 0.5f,-0.5f,  1, 0, 0,  0,1,
         0.5f,-0.5f,-0.5f,  1, 0, 0,  0,0,   0.5f,-0.5f,-0.5f,  1, 0, 0,  0,0,
         0.5f,-0.5f, 0.5f,  1, 0, 0,  1,0,   0.5f, 0.5f, 0.5f,  1, 0, 0,  1,1,
        // 底面
        -0.5f,-0.5f,-0.5f,  0,-1, 0,  0,1,   0.5f,-0.5f,-0.5f,  0,-1, 0,  1,1,
         0.5f,-0.5f, 0.5f,  0,-1, 0,  1,0,   0.5f,-0.5f, 0.5f,  0,-1, 0,  1,0,
        -0.5f,-0.5f, 0.5f,  0,-1, 0,  0,0,  -0.5f,-0.5f,-0.5f,  0,-1, 0,  0,1,
        // 顶面
        -0.5f, 0.5f,-0.5f,  0, 1, 0,  0,1,   0.5f, 0.5f,-0.5f,  0, 1, 0,  1,1,
         0.5f, 0.5f, 0.5f,  0, 1, 0,  1,0,   0.5f, 0.5f, 0.5f,  0, 1, 0,  1,0,
        -0.5f, 0.5f, 0.5f,  0, 1, 0,  0,0,  -0.5f, 0.5f,-0.5f,  0, 1, 0,  0,1,
    };

    GLuint litVAO,VBO;
    glGenVertexArrays(1,&litVAO);glGenBuffers(1,&VBO);
    glBindVertexArray(litVAO);
    glBindBuffer(GL_ARRAY_BUFFER,VBO);
    glBufferData(GL_ARRAY_BUFFER,sizeof(verts),verts,GL_STATIC_DRAW);
    const int stride=8*sizeof(float);
    glVertexAttribPointer(0,3,GL_FLOAT,GL_FALSE,stride,(void*)0);               glEnableVertexAttribArray(0);
    glVertexAttribPointer(1,3,GL_FLOAT,GL_FALSE,stride,(void*)(3*sizeof(float)));glEnableVertexAttribArray(1);
    glVertexAttribPointer(2,2,GL_FLOAT,GL_FALSE,stride,(void*)(6*sizeof(float)));glEnableVertexAttribArray(2);

    GLuint lampVAO;
    glGenVertexArrays(1,&lampVAO);
    glBindVertexArray(lampVAO);
    glBindBuffer(GL_ARRAY_BUFFER,VBO);
    glVertexAttribPointer(0,3,GL_FLOAT,GL_FALSE,stride,(void*)0);glEnableVertexAttribArray(0);
    glBindVertexArray(0);

    GLuint diffuseMap  = makeDiffuseMap();
    GLuint specularMap = makeSpecularMap();

    // 绑定纹理到固定的纹理单元，只需一次
    glUseProgram(litProg);
    glUniform1i(glGetUniformLocation(litProg,"uDiffuseMap"),  0);  // GL_TEXTURE0
    glUniform1i(glGetUniformLocation(litProg,"uSpecularMap"), 1);  // GL_TEXTURE1
    glUniform1f(glGetUniformLocation(litProg,"uShininess"), 64.0f);

    glm::mat4 projection=glm::perspective(glm::radians(45.f),(float)WIDTH/HEIGHT,0.1f,100.f);

    // 几个正方体的位置
    glm::vec3 positions[]={{0,0,0},{2.5f,0,0},{-2.5f,0,0},{0,0,-3.f}};

    while(!glfwWindowShouldClose(win)){
        float t=(float)glfwGetTime();
        deltaTime=t-lastFrame;lastFrame=t;
        processInput(win);
        glClearColor(0.05f,0.05f,0.1f,1);
        glClear(GL_COLOR_BUFFER_BIT|GL_DEPTH_BUFFER_BIT);

        glm::mat4 view=glm::lookAt(camPos,camPos+camFront,camUp);

        // 光源绕场景公转
        glm::vec3 lightPos={2.5f*cos(t*.7f), 1.5f*sin(t*.4f), 2.5f*sin(t*.7f)};

        // ── 绘制有贴图的正方体 ────────────────────────────────────────────────
        glUseProgram(litProg);
        glUniformMatrix4fv(glGetUniformLocation(litProg,"uView"),       1,GL_FALSE,glm::value_ptr(view));
        glUniformMatrix4fv(glGetUniformLocation(litProg,"uProjection"), 1,GL_FALSE,glm::value_ptr(projection));
        glUniform3fv(glGetUniformLocation(litProg,"uLightPos"),  1,glm::value_ptr(lightPos));
        glUniform3fv(glGetUniformLocation(litProg,"uLightColor"),1,glm::value_ptr(glm::vec3(1)));
        glUniform3fv(glGetUniformLocation(litProg,"uViewPos"),   1,glm::value_ptr(camPos));

        // 激活两个纹理单元
        glActiveTexture(GL_TEXTURE0); glBindTexture(GL_TEXTURE_2D,diffuseMap);
        glActiveTexture(GL_TEXTURE1); glBindTexture(GL_TEXTURE_2D,specularMap);

        glBindVertexArray(litVAO);
        for(auto& pos : positions){
            glm::mat4 model=glm::rotate(
                glm::translate(glm::mat4(1),pos),
                t*0.4f, glm::vec3(0.3f,1,0.5f));
            glm::mat3 nm=glm::transpose(glm::inverse(glm::mat3(model)));
            glUniformMatrix4fv(glGetUniformLocation(litProg,"uModel"),       1,GL_FALSE,glm::value_ptr(model));
            glUniformMatrix3fv(glGetUniformLocation(litProg,"uNormalMatrix"),1,GL_FALSE,glm::value_ptr(nm));
            glDrawArrays(GL_TRIANGLES,0,36);
        }

        // ── 绘制光源小方块 ────────────────────────────────────────────────────
        glUseProgram(lampProg);
        glUniformMatrix4fv(glGetUniformLocation(lampProg,"uView"),       1,GL_FALSE,glm::value_ptr(view));
        glUniformMatrix4fv(glGetUniformLocation(lampProg,"uProjection"), 1,GL_FALSE,glm::value_ptr(projection));
        glm::mat4 lm=glm::scale(glm::translate(glm::mat4(1),lightPos),glm::vec3(0.15f));
        glUniformMatrix4fv(glGetUniformLocation(lampProg,"uModel"),1,GL_FALSE,glm::value_ptr(lm));
        glBindVertexArray(lampVAO);
        glDrawArrays(GL_TRIANGLES,0,36);

        glfwSwapBuffers(win);glfwPollEvents();
    }
    glDeleteVertexArrays(1,&litVAO);glDeleteVertexArrays(1,&lampVAO);
    glDeleteBuffers(1,&VBO);
    glDeleteTextures(1,&diffuseMap);glDeleteTextures(1,&specularMap);
    glDeleteProgram(litProg);glDeleteProgram(lampProg);
    glfwDestroyWindow(win);glfwTerminate();
    return 0;
}

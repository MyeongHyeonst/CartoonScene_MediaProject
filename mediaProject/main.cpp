#define _CRT_SECURE_NO_WARNINGS

/* nuklear - 1.32.0 - public domain */
#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <stdarg.h>
#include <string.h>
#include <math.h>
#include <assert.h>
#include <math.h>
#include <limits.h>
#include <time.h>

#include <GL/glew.h>
#include <GLFW/glfw3.h>

#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtc/type_ptr.hpp>

#include <iostream>
#include <vector>

#include "shader.h"
#include "camera.h"
#include "model.h"
#include "framebuffer.h"

#define GLEW_STATIC

// nuklear flag
#define NK_INCLUDE_FIXED_TYPES
#define NK_INCLUDE_STANDARD_IO
#define NK_INCLUDE_STANDARD_VARARGS
#define NK_INCLUDE_DEFAULT_ALLOCATOR
#define NK_INCLUDE_VERTEX_BUFFER_OUTPUT
#define NK_INCLUDE_FONT_BAKING
#define NK_INCLUDE_DEFAULT_FONT
#define NK_IMPLEMENTATION
#define NK_GLFW_GL3_IMPLEMENTATION
#define NK_KEYSTATE_BASED_INPUT

#include <Nuklear/nuklear.h>
#include <Nuklear/nuklear_glfw_gl3.h>
#define MAX_VERTEX_BUFFER 512 * 1024
#define MAX_ELEMENT_BUFFER 128 * 1024

void framebuffer_size_callback(GLFWwindow* window, int width, int height);
void mouse_callback(GLFWwindow* window, double xpos, double ypos);
void scroll_callback(GLFWwindow* window, double xoffset, double yoffset);
void processInput(GLFWwindow* window);
void renderUI();
void renderOBJ();
void drawWindow();
void applyGaussian(int order);
void applySobel(int order);
void calculateGM();
void calculateETF0();
void calculateETF();
void applyDOG();
void applyXDOG();
void init();

// settings
unsigned int SCR_WIDTH = 800;
unsigned int SCR_HEIGHT = 600;

const unsigned int UI_WIDTH = 200;
const unsigned int UI_HEIGHT = 600;

// camera
Camera camera(glm::vec3(0.0f, 0.0f, 3.0f));
float lastX = SCR_WIDTH / 2.0f;
float lastY = SCR_HEIGHT / 2.0f;
float step = 1;
// timing
float deltaTime = 0.0f;
float lastFrame = 0.0f;

// lighting
glm::vec3 lightPos(1.2f, 1.0f, 2.0f);

// nuklear setting
struct nk_glfw glfw = { 0 };
struct nk_context* ctx;
struct nk_colorf bg;

Shader ourShader, drawShader, gaussianProgram, sobelProgram, gmProgram, etf0Program, etfProgram, dogProgram, xdogProgram;
Model ourModel;

Framebuffer originalBuffer, switchBuffer, switchBuffer2, sobelBufferX, sobelBufferY, gmBuffer, etf0Buffer, dogBuffer, edgeBuffer;
GLuint vao, vbo, ebo;

// XDOG parameter
float sigma = 0.2f;
float sigma_c = 1.7f;
float sigma_m = 5.0f;
float k = 1.4f;
float p = 39.f;
float ep = 0.6f;
float phi = 0.016f;

int main()
{
    // glfw: initialize and configure
    glfwInit();
    glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 3);
    glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 3);
    glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE);

    // glfw window creation
    GLFWwindow* window = glfwCreateWindow(SCR_WIDTH, SCR_HEIGHT, "Viewer", NULL, NULL);
    if (window == NULL)
    {
        std::cout << "Failed to create GLFW window" << std::endl;
        glfwTerminate();
        return -1;
    }
    GLFWwindow* UI = glfwCreateWindow(UI_WIDTH, UI_HEIGHT, "UI", NULL, NULL);
    if (UI == NULL)
    {
        std::cout << "Failed to create GLFW window" << std::endl;
        glfwTerminate();
        return -1;
    }

    glfwMakeContextCurrent(window);
    glfwSetFramebufferSizeCallback(window, framebuffer_size_callback);
    glfwSetCursorPosCallback(window, mouse_callback);
    glfwSetScrollCallback(window, scroll_callback);

    //glew init
    glewExperimental = GL_TRUE;
    GLenum errorCode = glewInit();
    if (GLEW_OK != errorCode)
    {
        cerr << "Error : GLEW Init Fail" << glewGetErrorString(errorCode) << endl;

        glfwTerminate();
        return -1;
    }

    init();

    // load models
    ourModel.Load("C:/model/backpack/backpack.obj");
    //ourModel.Load("C:/model/stanford-bunny.obj");
    // ourModel.Load("C:/model/chicken/scene.gltf");


    // nuklear //
    glfwMakeContextCurrent(UI);
    ctx = nk_glfw3_init(&glfw, UI, NK_GLFW3_DEFAULT);
    // Font
    struct nk_font_atlas* atlas;
    nk_glfw3_font_stash_begin(&glfw, &atlas);
    nk_glfw3_font_stash_end(&glfw);

    // render loop
    // -----------
    while (!glfwWindowShouldClose(window))
    {
        glfwMakeContextCurrent(UI);
        renderUI();
        glfwSwapBuffers(UI);

        glfwMakeContextCurrent(window);
        processInput(window);

        renderOBJ();
        applyGaussian(0);
        applyGaussian(1);

        applySobel(0);
        applySobel(1);

        calculateGM();

        calculateETF0();
        calculateETF();

        applyDOG();
        applyXDOG();

        drawWindow();

        glfwSwapBuffers(window);
        glfwPollEvents();
    }
    nk_glfw3_shutdown(&glfw);
    originalBuffer.deleteFramebuffer();

    glfwTerminate();
    return 0;
}
float etf = 7;
void renderUI()
{
    // NuKlear GUI 
    nk_glfw3_new_frame(&glfw);

    if (nk_begin(ctx, "UI", nk_rect(30, 30, UI_WIDTH-30, UI_HEIGHT/3.f),
        NK_WINDOW_BORDER | NK_WINDOW_MOVABLE | NK_WINDOW_SCALABLE |
        NK_WINDOW_MINIMIZABLE | NK_WINDOW_TITLE))
    {
        // Contents
        static const float ratio[] = { 100, 100 };
        nk_layout_row(ctx, NK_STATIC, 25, 2, ratio);

        nk_labelf(ctx, NK_TEXT_LEFT, "sigma = %.1f", sigma);
        nk_slider_float(ctx, 0.0f, &sigma, 1.0f, 0.01f);

        nk_labelf(ctx, NK_TEXT_LEFT, "eft time = %.1f", etf);
        nk_slider_float(ctx, 0.0f, &etf, 10.0f, 1.0f);

        nk_labelf(ctx, NK_TEXT_LEFT, "sigma_m = %.1f", sigma_m);
        nk_slider_float(ctx, 0.0f, &sigma_m, 50.0f, 0.1f);

        nk_labelf(ctx, NK_TEXT_LEFT, "sigma_c = %.1f", sigma_c);
        nk_slider_float(ctx, 0.0f, &sigma_c, 50.0f, 0.1f);

        nk_labelf(ctx, NK_TEXT_LEFT, "Thickness(k) = %.1f", k);
        nk_slider_float(ctx, 0.0f, &k, 10.0f, 0.1f);

        nk_labelf(ctx, NK_TEXT_LEFT, "Detail(p) = %.1f", p);
        nk_slider_float(ctx, 0.0f, &p, 100.0f, 0.1f);

        nk_labelf(ctx, NK_TEXT_LEFT, "ep = %.2f", ep);
        nk_slider_float(ctx, 0.0f, &ep, 1.0f, 0.01f);

        nk_labelf(ctx, NK_TEXT_LEFT, "phi = %.3f", phi);
        nk_slider_float(ctx, 0.0f, &phi, 0.1f, 0.001f);
    }
    nk_end(ctx);

    nk_glfw3_render(&glfw, NK_ANTI_ALIASING_ON, MAX_VERTEX_BUFFER, MAX_ELEMENT_BUFFER);
}

void renderOBJ()
{
    float currentFrame = static_cast<float>(glfwGetTime());
    deltaTime = currentFrame - lastFrame;
    lastFrame = currentFrame;

    originalBuffer.bindBuffer();
    glEnable(GL_DEPTH_TEST);

    glClearColor(1.0f, 0.5f, 0.5f, 1.0f);
    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

    // model rendering
    ourShader.use();
    ourShader.setVec3("objectColor", 1.0f, 0.5f, 0.31f);
    ourShader.setVec3("lightColor", 1.0f, 1.0f, 1.0f);
    ourShader.setVec3("lightPos", lightPos);
    ourShader.setVec3("viewPos", camera.Position);

    // view/projection transformations
    glm::mat4 projection = glm::perspective(glm::radians(camera.Zoom), (float)SCR_WIDTH / (float)SCR_HEIGHT, 0.1f, 100.0f);
    glm::mat4 view = camera.GetViewMatrix();
    ourShader.setMat4("projection", projection);
    ourShader.setMat4("view", view);

    // render the loaded model
    glm::mat4 model = glm::mat4(1.0f);
    model = glm::translate(model, glm::vec3(0.0f, 0.0f, 0.0f));
    //model = glm::rotate(model, (float)glfwGetTime(), glm::vec3(0.0, 1.0, 0.0));
    model = glm::scale(model, (glm::vec3(1.0f, 1.0f, 1.0f)));// *glm::vec3(3)));
    ourShader.setMat4("model", model);
    ourModel.Draw(ourShader);
}
void applyGaussian(int order)
{
    if (order == 0) switchBuffer.bindBuffer();
    else switchBuffer2.bindBuffer();

    glDisable(GL_DEPTH_TEST);
    glClearColor(0.5f, 0.5f, 0.5f, 1.0f);
    glClear(GL_COLOR_BUFFER_BIT);

    gaussianProgram.use();
    glBindVertexArray(vao);

    if (order == 0) originalBuffer.bind();
    else switchBuffer.bind();

    gaussianProgram.setInt("len", SCR_WIDTH);
    gaussianProgram.setInt("order", order);
    gaussianProgram.setFloat("sigma", sigma);

    glDrawElements(GL_TRIANGLES, 6, GL_UNSIGNED_INT, 0);
}
void applySobel(int order)
{
    if (order == 0) sobelBufferX.bindBuffer();
    else sobelBufferY.bindBuffer();

    glClearColor(0.5f, 0.5f, 0.5f, 1.0f);
    glClear(GL_COLOR_BUFFER_BIT);

    sobelProgram.use();
    glBindVertexArray(vao);

    switchBuffer2.bind();

    sobelProgram.setInt("width", SCR_WIDTH);
    sobelProgram.setInt("height", SCR_HEIGHT);
    sobelProgram.setInt("order", order);

    glDrawElements(GL_TRIANGLES, 6, GL_UNSIGNED_INT, 0);
}
// gradient magnitude 
void calculateGM()
{
    gmBuffer.bindBuffer();
    glClearColor(0.5f, 0.5f, 0.5f, 1.0f);
    glClear(GL_COLOR_BUFFER_BIT);

    gmProgram.use();
    glBindVertexArray(vao);

    glActiveTexture(GL_TEXTURE0 + 1);
    sobelBufferX.bind();
    gmProgram.setInt("sobelx", 1);

    glActiveTexture(GL_TEXTURE0);
    sobelBufferY.bind();
    gmProgram.setInt("sobely", 0);

    glDrawElements(GL_TRIANGLES, 6, GL_UNSIGNED_INT, 0);
}
void calculateETF0()
{
    etf0Buffer.bindBuffer();
    glClearColor(0.5f, 0.5f, 0.5f, 1.0f);
    glClear(GL_COLOR_BUFFER_BIT);

    etf0Program.use();
    glBindVertexArray(vao);

    glActiveTexture(GL_TEXTURE0 + 2);
    gmBuffer.bind();
    etf0Program.setInt("magnitude", 2);

    glActiveTexture(GL_TEXTURE0 + 1);
    sobelBufferX.bind();
    etf0Program.setInt("sobelx", 1);

    glActiveTexture(GL_TEXTURE0);
    sobelBufferY.bind();
    etf0Program.setInt("sobely", 0);

    glDrawElements(GL_TRIANGLES, 6, GL_UNSIGNED_INT, 0);
}
void calculateETF()
{
    for (int i = 0; i <etf; i++)
    {
        if (i % 2 == 0)
        {
            switchBuffer.bindBuffer();
            glClearColor(0.5f, 0.5f, 0.5f, 1.0f);
            glClear(GL_COLOR_BUFFER_BIT);

            etfProgram.use();
            glBindVertexArray(vao);

            glActiveTexture(GL_TEXTURE0 + 1);
            etf0Buffer.bind();
            etfProgram.setInt("ETF", 1);

            glActiveTexture(GL_TEXTURE0);
            gmBuffer.bind();
            etfProgram.setInt("magnitude", 0);

            etfProgram.setInt("width", SCR_WIDTH);

            glDrawElements(GL_TRIANGLES, 6, GL_UNSIGNED_INT, 0);
        }
        else
        {
            etf0Buffer.bindBuffer();
            glClearColor(0.5f, 0.5f, 0.5f, 1.0f);
            glClear(GL_COLOR_BUFFER_BIT);

            etfProgram.use();
            glBindVertexArray(vao);

            glActiveTexture(GL_TEXTURE0 + 1);
            switchBuffer.bind();
            etfProgram.setInt("ETF", 1);

            glActiveTexture(GL_TEXTURE0);
            gmBuffer.bind();
            etfProgram.setInt("magnitude", 0);

            etfProgram.setInt("width", SCR_WIDTH);

            glDrawElements(GL_TRIANGLES, 6, GL_UNSIGNED_INT, 0);
        }  
    }
}
void applyDOG()
{
    dogBuffer.bindBuffer();
    glClearColor(0.5f, 0.5f, 0.5f, 1.0f);
    glClear(GL_COLOR_BUFFER_BIT);

    dogProgram.use();
    glBindVertexArray(vao);

    glActiveTexture(GL_TEXTURE0 + 1);
    etf0Buffer.bind();
    dogProgram.setInt("ETF", 1);

    glActiveTexture(GL_TEXTURE0);
    originalBuffer.bind();
    dogProgram.setInt("source", 0);

    dogProgram.setInt("width", SCR_WIDTH);
    dogProgram.setFloat("sigma_c", sigma_c);
    dogProgram.setFloat("k", k);
    dogProgram.setFloat("p", p);

    glDrawElements(GL_TRIANGLES, 6, GL_UNSIGNED_INT, 0);
}
void applyXDOG()
{
    edgeBuffer.bindBuffer();
    glClearColor(0.5f, 0.5f, 0.5f, 1.0f);
    glClear(GL_COLOR_BUFFER_BIT);

    xdogProgram.use();
    glBindVertexArray(vao);

    glActiveTexture(GL_TEXTURE0 + 1);
    etf0Buffer.bind();
    xdogProgram.setInt("ETF", 1);

    glActiveTexture(GL_TEXTURE0);
    dogBuffer.bind();
    xdogProgram.setInt("dog", 0);

    xdogProgram.setInt("width", SCR_WIDTH);
    xdogProgram.setFloat("sigma_m", sigma_m);
    xdogProgram.setFloat("epsilon", ep);
    xdogProgram.setFloat("phi", phi);

    glDrawElements(GL_TRIANGLES, 6, GL_UNSIGNED_INT, 0);
}
void drawWindow()
{
    glBindFramebuffer(GL_FRAMEBUFFER, 0);
    
    glClearColor(0.5f, 1.0f, 0.5f, 1.0f);
    glClear(GL_COLOR_BUFFER_BIT);

    drawShader.use();
    glBindVertexArray(vao);
    edgeBuffer.bind();

    glDrawElements(GL_TRIANGLES, 6, GL_UNSIGNED_INT, 0);
}

void init()
{
    ourShader.loadShaders("model.vert", "model.frag");
    drawShader.loadShaders("draw.vert", "draw.frag");
    gaussianProgram.loadShaders("draw.vert", "gaussian.frag");
    sobelProgram.loadShaders("draw.vert", "sobel.frag");
    gmProgram.loadShaders("draw.vert", "gradientMagnitude.frag");
    etf0Program.loadShaders("draw.vert", "etf0.frag");
    etfProgram.loadShaders("draw.vert", "ETF.frag");
    dogProgram.loadShaders("draw.vert", "DoG.frag");
    xdogProgram.loadShaders("draw.vert", "XDoG.frag");

    float vertices[] = {
        1.0f,  1.0f, 0.0f, 1.0f, 1.0f,   // top right
        1.0f, -1.0f, 0.0f, 1.0f, 0.0f,   // bottom right
       -1.0f, -1.0f, 0.0f, 0.0f, 0.0f,   // bottom left
       -1.0f,  1.0f, 0.0f, 0.0f, 1.0f    // top left 
    };

    unsigned int indices[] = {
          3, 2, 1,
          3, 1, 0
    };

    glGenVertexArrays(1, &vao);
    glGenBuffers(1, &vbo);
    glGenBuffers(1, &ebo);

    glBindVertexArray(vao);

    glBindBuffer(GL_ARRAY_BUFFER, vbo);
    glBufferData(GL_ARRAY_BUFFER, sizeof(vertices), vertices, GL_STATIC_DRAW);

    glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, ebo);
    glBufferData(GL_ELEMENT_ARRAY_BUFFER, sizeof(indices), indices, GL_STATIC_DRAW);

    //position
    glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 5 * sizeof(float), (void*)0);
    glEnableVertexAttribArray(0);

    //texture
    glVertexAttribPointer(1, 2, GL_FLOAT, GL_FALSE, 5 * sizeof(float), (void*)(3 * sizeof(float)));
    glEnableVertexAttribArray(1);

    originalBuffer.generateBufferwithDepth(SCR_WIDTH, SCR_HEIGHT);
    switchBuffer.generateBuffer(SCR_WIDTH, SCR_HEIGHT);
    switchBuffer2.generateBuffer(SCR_WIDTH, SCR_HEIGHT);
    sobelBufferX.generateBuffer(SCR_WIDTH, SCR_HEIGHT);
    sobelBufferY.generateBuffer(SCR_WIDTH, SCR_HEIGHT);
    gmBuffer.generateBuffer(SCR_WIDTH, SCR_HEIGHT);
    etf0Buffer.generateBuffer(SCR_WIDTH, SCR_HEIGHT);
    dogBuffer.generateBuffer(SCR_WIDTH, SCR_HEIGHT);
    edgeBuffer.generateBuffer(SCR_WIDTH, SCR_HEIGHT);
}

// process all input: query GLFW whether relevant keys are pressed/released this frame and react accordingly
// ---------------------------------------------------------------------------------------------------------
void processInput(GLFWwindow* window)
{
    if (glfwGetKey(window, GLFW_KEY_ESCAPE) == GLFW_PRESS)
        glfwSetWindowShouldClose(window, true);

    if (glfwGetKey(window, GLFW_KEY_W) == GLFW_PRESS)
        camera.ProcessKeyboard(FORWARD, deltaTime);
    if (glfwGetKey(window, GLFW_KEY_S) == GLFW_PRESS)
        camera.ProcessKeyboard(BACKWARD, deltaTime);
    if (glfwGetKey(window, GLFW_KEY_A) == GLFW_PRESS)
        camera.ProcessKeyboard(LEFT, deltaTime);
    if (glfwGetKey(window, GLFW_KEY_D) == GLFW_PRESS)
        camera.ProcessKeyboard(RIGHT, deltaTime);
    if (glfwGetKey(window, GLFW_KEY_Q) == GLFW_PRESS)
        camera.ProcessKeyboard(UP, deltaTime);
    if (glfwGetKey(window, GLFW_KEY_E) == GLFW_PRESS)
        camera.ProcessKeyboard(DOWN, deltaTime);
}

// glfw: whenever the window size changed (by OS or user resize) this callback function executes
// ---------------------------------------------------------------------------------------------
void framebuffer_size_callback(GLFWwindow* window, int width, int height)
{
    glViewport(0, 0, width, height);

    SCR_WIDTH = width;
    SCR_HEIGHT = height;

    originalBuffer.generateBufferwithDepth(SCR_WIDTH, SCR_HEIGHT);
}

// glfw: whenever the mouse moves, this callback is called
// -------------------------------------------------------
void mouse_callback(GLFWwindow* window, double xposIn, double yposIn)
{
    float xpos = static_cast<float>(xposIn);
    float ypos = static_cast<float>(yposIn);

    if (glfwGetMouseButton(window, GLFW_MOUSE_BUTTON_LEFT) == GLFW_RELEASE)
    {
        lastX = xpos;
        lastY = ypos;
    }

    float xoffset = xpos - lastX;
    float yoffset = lastY - ypos; // reversed since y-coordinates go from bottom to top

    lastX = xpos;
    lastY = ypos;

    camera.ProcessMouseMovement(xoffset, yoffset);
}

// glfw: whenever the mouse scroll wheel scrolls, this callback is called
// ----------------------------------------------------------------------
void scroll_callback(GLFWwindow* window, double xoffset, double yoffset)
{
    camera.ProcessMouseScroll(static_cast<float>(yoffset));
}
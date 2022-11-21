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
#include "XDOG.h"

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
void renderOBJ(Framebuffer f, Shader shader);
void drawWindow(Framebuffer colorB, Framebuffer edgeB, bool isEdge);

void init();

// settings
unsigned int SCR_WIDTH = 1200;
unsigned int SCR_HEIGHT = 600;

unsigned int VIEW_WIDTH;
unsigned int VIEW_HEIGHT;

const unsigned int UI_WIDTH = 400;
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
float lx = 1.2f, ly = 1.0f, lz = 2.0f;
glm::vec3 lightPos(1.2f, 1.0f, 2.0f);

// nuklear setting
struct nk_glfw glfw = { 0 };
struct nk_context* ctx;
struct nk_colorf bg;

Shader ourShader, drawEdgeShader, drawColorShader, goochShader;
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
float etf = 7;
XDOG xdog;
GLFWwindow *window;

int shading_num = 0;
int drawing_step = 0;
int main()
{
    
    init();

    // load models
    //ourModel.Load("C:/model/backpack/backpack.obj");
    ourModel.Load("C:/model/autumn_house/scene.gltf");
    //ourModel.Load("C:/model/modern_bedroom/scene.gltf");
    //ourModel.Load("C:/model/cafe-misti/scene.gltf");
    //ourModel.Load("C:/model/cafe/scene.gltf");
    //ourModel.Load("C:/model/stanford-bunny.obj");


    // nuklear //

    glfwMakeContextCurrent(window);
    ctx = nk_glfw3_init(&glfw, window, NK_GLFW3_DEFAULT);
    // Font
    struct nk_font_atlas* atlas;
    nk_glfw3_font_stash_begin(&glfw, &atlas);
    nk_glfw3_font_stash_end(&glfw);

    VIEW_WIDTH = SCR_WIDTH - UI_WIDTH;
    VIEW_HEIGHT = SCR_HEIGHT;

    // render loop
    // -----------
    while (!glfwWindowShouldClose(window))
    {
        processInput(window);

        switch (shading_num)
        {
        case 0:
            renderOBJ(originalBuffer, goochShader);
            break;
        case 1:
            renderOBJ(originalBuffer, ourShader);
            break;
        }
        
        switch (drawing_step)
        {
        case 0:
            glDisable(GL_DEPTH_TEST);
            drawWindow(originalBuffer, edgeBuffer, false);
            break;
        case 1:
            xdog.applyGaussian(switchBuffer, switchBuffer2, originalBuffer, 0, sigma);
            xdog.applyGaussian(switchBuffer, switchBuffer2, originalBuffer, 1, sigma);

            xdog.applySobel(sobelBufferX, sobelBufferY, switchBuffer2, 0);
            xdog.applySobel(sobelBufferX, sobelBufferY, switchBuffer2, 1);

            xdog.calculateGM(gmBuffer, sobelBufferX, sobelBufferY);

            xdog.calculateETF0(etf0Buffer, gmBuffer, sobelBufferX, sobelBufferY);
            xdog.calculateETF(switchBuffer, etf0Buffer, gmBuffer, etf);

            xdog.applyDOG(dogBuffer, etf0Buffer, originalBuffer, sigma_c, k, p);
            xdog.applyXDOG(edgeBuffer, etf0Buffer, dogBuffer, sigma_m, ep, phi);

            drawWindow(switchBuffer2, edgeBuffer, true);
            break;
        }

        renderUI();
        glfwSwapBuffers(window);
        glfwPollEvents();
    }
    nk_glfw3_shutdown(&glfw);
    originalBuffer.deleteFramebuffer();

    glfwTerminate();
    return 0;
}
float lr = 1, lg = 1, lb = 1;
float sr = 0, sg = 0, sb = 0;
void renderUI()
{
    // NuKlear GUI 
    nk_glfw3_new_frame(&glfw);

    if (nk_begin(ctx, "UI", nk_rect(5, 5, UI_WIDTH-10, UI_HEIGHT-10),
        NK_WINDOW_BORDER | NK_WINDOW_MOVABLE | NK_WINDOW_SCALABLE |
        NK_WINDOW_MINIMIZABLE | NK_WINDOW_TITLE))
    {
        // Contents
        static const float ratio[] = { 100, 100 };
        nk_layout_row(ctx, NK_STATIC, 25, 2, ratio);

        nk_labelf(ctx, NK_TEXT_LEFT, "sigma = %.2f", sigma);
        nk_slider_float(ctx, 0.0f, &sigma, 1.0f, 0.01f);

        nk_labelf(ctx, NK_TEXT_LEFT, "eft time = %.1f", etf);
        nk_slider_float(ctx, 0.0f, &etf, 10.0f, 1.0f);

        nk_labelf(ctx, NK_TEXT_LEFT, "sigma_m = %.1f", sigma_m);
        nk_slider_float(ctx, 0.0f, &sigma_m, 50.0f, 0.1f);

        nk_labelf(ctx, NK_TEXT_LEFT, "sigma_c = %.2f", sigma_c);
        nk_slider_float(ctx, 0.0f, &sigma_c, 10.0f, 0.01f);

        nk_labelf(ctx, NK_TEXT_LEFT, "Thickness(k) = %.1f", k);
        nk_slider_float(ctx, 0.0f, &k, 10.0f, 0.1f);

        nk_labelf(ctx, NK_TEXT_LEFT, "Detail(p) = %.1f", p);
        nk_slider_float(ctx, 0.0f, &p, 100.0f, 0.1f);

        nk_labelf(ctx, NK_TEXT_LEFT, "ep = %.2f", ep);
        nk_slider_float(ctx, 0.0f, &ep, 1.0f, 0.01f);

        nk_labelf(ctx, NK_TEXT_LEFT, "phi = %.3f", phi);
        nk_slider_float(ctx, 0.0f, &phi, 0.1f, 0.001f);
        
        nk_labelf(ctx, NK_TEXT_LEFT, "light r = %.1f", lr);
        nk_slider_float(ctx, 0.0f, &lr, 1.0f, 0.1f);

        nk_labelf(ctx, NK_TEXT_LEFT, "light g = %.1f", lg);
        nk_slider_float(ctx, 0.0f, &lg, 1.0f, 0.1f);

        nk_labelf(ctx, NK_TEXT_LEFT, "light b = %.1f", lb);
        nk_slider_float(ctx, 0.0f, &lb, 1.0f, 0.1f);

        nk_labelf(ctx, NK_TEXT_LEFT, "shade r = %.1f", sr);
        nk_slider_float(ctx, 0.0f, &sr, 1.0f, 0.1f);

        nk_labelf(ctx, NK_TEXT_LEFT, "shade g = %.1f", sg);
        nk_slider_float(ctx, 0.0f, &sg, 1.0f, 0.1f);

        nk_labelf(ctx, NK_TEXT_LEFT, "shade b = %.1f", sb);
        nk_slider_float(ctx, 0.0f, &sb, 1.0f, 0.1f);

        if (nk_button_label(ctx, "Gooch Shading")) shading_num = 0;
        if (nk_button_label(ctx, "Phong Shading")) shading_num = 1;
        if (nk_button_label(ctx, "Original")) drawing_step = 0;
        if (nk_button_label(ctx, "XDOG")) drawing_step = 1;

    }
    nk_end(ctx);

    //glClearColor(0.2f, 0.2f, 0.2f, 1.0f);
    //glClear(GL_COLOR_BUFFER_BIT);


    nk_glfw3_render(&glfw, NK_ANTI_ALIASING_ON, MAX_VERTEX_BUFFER, MAX_ELEMENT_BUFFER);
}
float s = 1;

void renderOBJ(Framebuffer f, Shader shader)
{
    float currentFrame = static_cast<float>(glfwGetTime());
    deltaTime = currentFrame - lastFrame;
    lastFrame = currentFrame;

    f.bindBuffer();

    glEnable(GL_DEPTH_TEST);

    glClearColor(1.0f, 0.5f, 0.5f, 1.0f);
    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

    // model rendering
    shader.use();
    shader.setVec3("objectColor", sr, sg, sb);
    shader.setVec3("lightColor", lr, lg, lb);
    shader.setVec3("lightPos", lightPos);
    shader.setVec3("viewPos", camera.Position);

    // view/projection transformations
    glm::mat4 projection = glm::perspective(glm::radians(camera.Zoom), (float)SCR_WIDTH / (float)SCR_HEIGHT, 0.1f, 100.0f);
    glm::mat4 view = camera.GetViewMatrix();
    shader.setMat4("projection", projection);
    shader.setMat4("view", view);

    // render the loaded model
    glm::mat4 model = glm::mat4(1.0f);
    model = glm::translate(model, glm::vec3(0.0f, 0.0f, 0.0f));
    //model = glm::rotate(model, (float)glfwGetTime(), glm::vec3(0.0, 1.0, 0.0));
    model = glm::scale(model, (glm::vec3(1.0f, 1.0f, 1.0f)*glm::vec3(s)));
    shader.setMat4("model", model);
    ourModel.Draw(shader);
}
void drawWindow(Framebuffer colorB, Framebuffer edgeB, bool isEdge)
{
    glBindFramebuffer(GL_FRAMEBUFFER, 0);
    
    glClearColor(0.5f, 1.0f, 0.5f, 1.0f);
    glClear(GL_COLOR_BUFFER_BIT);

    drawColorShader.use();
    glBindVertexArray(vao);
    colorB.bind();

    glDrawElements(GL_TRIANGLES, 6, GL_UNSIGNED_INT, 0);

    if (isEdge)
    {
        glEnable(GL_BLEND);
        glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);

        drawEdgeShader.use();
        glBindVertexArray(vao);
        edgeB.bind();

        glDrawElements(GL_TRIANGLES, 6, GL_UNSIGNED_INT, 0);
    }
}
void init()
{
    // glfw: initialize and configure
    glfwInit();
    glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 3);
    glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 3);
    glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE);

    // glfw window creation
    window = glfwCreateWindow(SCR_WIDTH, SCR_HEIGHT, "Viewer", NULL, NULL);
    if (window == NULL)
    {
        std::cout << "Failed to create GLFW window" << std::endl;
        glfwTerminate();
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
    }

    // For blend
    glEnable(GL_DEPTH_TEST);

    ourShader.loadShaders("model.vert", "model.frag");
    goochShader.loadShaders("model.vert", "GoochShading.frag");
    drawColorShader.loadShaders("draw.vert", "draw.frag");
    drawEdgeShader.loadShaders("draw.vert", "drawEdge.frag");
 
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

    xdog.init();
    xdog.setWidth(SCR_WIDTH); xdog.setHeight(SCR_HEIGHT);
}

// process all input: query GLFW whether relevant keys are pressed/released this frame and react accordingly
// ---------------------------------------------------------------------------------------------------------
void processInput(GLFWwindow* window)
{
    if (glfwGetKey(window, GLFW_KEY_ESCAPE) == GLFW_PRESS)
        glfwSetWindowShouldClose(window, true);

    // Camera Position
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

    // Light Position
    if (glfwGetKey(window, GLFW_KEY_U) == GLFW_PRESS)
    {
        lz += 0.1f;
        lightPos = glm::vec3(lx, ly, lz);
    }
    if (glfwGetKey(window, GLFW_KEY_J) == GLFW_PRESS)
    {
        lz -= 0.1f;
        lightPos = glm::vec3(lx, ly, lz);
    }
    if (glfwGetKey(window, GLFW_KEY_H) == GLFW_PRESS)
    {
        lx += 0.1f;
        lightPos = glm::vec3(lx, ly, lz);
    }
    if (glfwGetKey(window, GLFW_KEY_K) == GLFW_PRESS)
    {
        lx -= 0.1f;
        lightPos = glm::vec3(lx, ly, lz);
    }
    if (glfwGetKey(window, GLFW_KEY_Y) == GLFW_PRESS)
    {
        ly += 0.1f;
        lightPos = glm::vec3(lx, ly, lz);
    }
    if (glfwGetKey(window, GLFW_KEY_I) == GLFW_PRESS)
    {
        ly -= 0.1f;
        lightPos = glm::vec3(lx, ly, lz);
    }


    if (glfwGetKey(window, GLFW_KEY_MINUS) == GLFW_PRESS)
        s -= 0.01f;
    if (glfwGetKey(window, GLFW_KEY_EQUAL) == GLFW_PRESS)
        s+= 0.01f;
}

// glfw: whenever the window size changed (by OS or user resize) this callback function executes
// ---------------------------------------------------------------------------------------------
void framebuffer_size_callback(GLFWwindow* window, int width, int height)
{
    //glViewport(0, 0, width, height);

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

    if(xpos > UI_WIDTH)
        camera.ProcessMouseMovement(xoffset, yoffset);
}

// glfw: whenever the mouse scroll wheel scrolls, this callback is called
// ----------------------------------------------------------------------
void scroll_callback(GLFWwindow* window, double xoffset, double yoffset)
{
    camera.ProcessMouseScroll(static_cast<float>(yoffset));
}
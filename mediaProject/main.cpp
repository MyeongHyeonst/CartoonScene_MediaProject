// ToDo : camera 회전 -> model 회전 바꾸기

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

#include <opencv2/core.hpp>
#include <opencv2/opencv.hpp>

#include <iostream>
#include <vector>

#include "shader.h"
#include "camera.h"
#include "model.h"
#include "framebuffer.h"
#include "XDOG.h"
#include "export.h"
#include "guidedfilter.hpp"

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
void drawWindow(Framebuffer colorB, Framebuffer edgeB, bool isEdge, bool blend);

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
float r = 1;
int theta = 45;
int theta2 = 0;
float lx = 1.2f, ly = 1.0f, lz = 2.0f;
glm::vec3 lightPos(1.2f, 1.0f, 2.0f);

int posMAX = 20;
glm::vec3 objPos = glm::vec3(posMAX/2, posMAX / 2, posMAX / 2);
glm::vec3 objRot = glm::vec3(0, 1, 0);

// nuklear setting
struct nk_glfw glfw = { 0 };
struct nk_context* ctx;
struct nk_colorf bg;

Shader ourShader, drawEdgeShader, drawColorShader, goochShader, goochToonShader, phongToonShader, blendShader;
Model ourModel;
cv::Mat source;

Framebuffer originalBuffer, switchBuffer, switchBuffer2, sobelBufferX, sobelBufferY, gmBuffer, etf0Buffer, dogBuffer, edgeBuffer;
GLuint vao, vbo, ebo;

// XDOG parameter
float sigma = 0.2f;
float sigma_c = 0.9f;
float sigma_m = 7.5f;
float k = 1.2f;
float p = 20.8f;
float ep = 0.66f;
float phi = 0.02f;
float etf = 7;
XDoG xdog;
GLFWwindow *window, *UI;

int shading_num = 0;
int drawing_step = 0;

int levels = 1;

char exportPath[] = "output/output.png";
char edgePath[] = "output/edge.png";
char colorPath[] = "output/color.png";

nk_colorf backcol = { 1, 0.5, 0.5, 1 }; // Background color
nk_colorf lightcol = { 1, 0.5, 0.5, 1 }; // light color
nk_colorf shadingcol = { 0, 0, 0.5, 1 }; // shading color

enum { GOOCH, PHONG, GOOCHTOON, PHONGTOON };
enum { ORIGINAL, XDOG, EDGE };


int main()
{ 
    // image
    source = cv::imread("input/tower.jpg");

    source.convertTo(source, CV_32F, 1 / 255.f);
    cv::Mat result = GuidedFilter(source, source, cv::Size(2, 2), 0.1f);
    cvtColor(result, result, cv::COLOR_RGB2BGR);
    flip(result, result, 0);

    init();
    int inputType = 1;
    // load models
    ourModel.Load("input/autumn_house/scene.gltf");

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
        renderUI();

        glfwMakeContextCurrent(window);

        processInput(window);
        switch (inputType) {
        case 0 : 
            switch (shading_num)
            {
            case GOOCH:
                renderOBJ(originalBuffer, goochShader);
                break;
            case PHONG:
                renderOBJ(originalBuffer, ourShader);
                break;
            case GOOCHTOON:
                renderOBJ(originalBuffer, goochToonShader);
                break;
            case PHONGTOON:
                renderOBJ(originalBuffer, phongToonShader);
                break;
            }

            switch (drawing_step)
            {
            case ORIGINAL:
                glDisable(GL_DEPTH_TEST);
                drawWindow(originalBuffer, edgeBuffer, false, false);
                break;
            case XDOG:
                xdog.applyGaussian(switchBuffer, switchBuffer2, originalBuffer, 0, sigma);
                xdog.applyGaussian(switchBuffer, switchBuffer2, originalBuffer, 1, sigma);

                xdog.applySobel(sobelBufferX, sobelBufferY, switchBuffer2, 0);
                xdog.applySobel(sobelBufferX, sobelBufferY, switchBuffer2, 1);

                xdog.calculateGM(gmBuffer, sobelBufferX, sobelBufferY);

                xdog.calculateETF0(etf0Buffer, gmBuffer, sobelBufferX, sobelBufferY);
                xdog.calculateETF(switchBuffer, etf0Buffer, gmBuffer, etf);

                xdog.applyDOG(dogBuffer, etf0Buffer, originalBuffer, sigma_c, k, p);
                xdog.applyXDOG(edgeBuffer, etf0Buffer, dogBuffer, sigma_m, ep, phi);

                drawWindow(switchBuffer2, edgeBuffer, true, false);
                break;
            case EDGE:
                xdog.applyGaussian(switchBuffer, switchBuffer2, originalBuffer, 0, sigma);
                xdog.applyGaussian(switchBuffer, switchBuffer2, originalBuffer, 1, sigma);

                xdog.applySobel(sobelBufferX, sobelBufferY, switchBuffer2, 0);
                xdog.applySobel(sobelBufferX, sobelBufferY, switchBuffer2, 1);

                xdog.calculateGM(gmBuffer, sobelBufferX, sobelBufferY);

                xdog.calculateETF0(etf0Buffer, gmBuffer, sobelBufferX, sobelBufferY);
                xdog.calculateETF(switchBuffer, etf0Buffer, gmBuffer, etf);

                xdog.applyDOG(dogBuffer, etf0Buffer, originalBuffer, sigma_c, k, p);
                xdog.applyXDOG(edgeBuffer, etf0Buffer, dogBuffer, sigma_m, ep, phi);

                drawWindow(edgeBuffer, edgeBuffer, false, false);
            }
            break;
        case 1:
            originalBuffer.setHDR(result);
            glDisable(GL_DEPTH_TEST);

            xdog.applyGaussian(switchBuffer, switchBuffer2, originalBuffer, 0, sigma);
            xdog.applyGaussian(switchBuffer, switchBuffer2, originalBuffer, 1, sigma);

            xdog.applySobel(sobelBufferX, sobelBufferY, switchBuffer2, 0);
            xdog.applySobel(sobelBufferX, sobelBufferY, switchBuffer2, 1);

            xdog.calculateGM(gmBuffer, sobelBufferX, sobelBufferY);

            xdog.calculateETF0(etf0Buffer, gmBuffer, sobelBufferX, sobelBufferY);
            xdog.calculateETF(switchBuffer, etf0Buffer, gmBuffer, etf);

            xdog.applyDOG(dogBuffer, etf0Buffer, originalBuffer, sigma_c, k, p);
            xdog.applyXDOG(edgeBuffer, etf0Buffer, dogBuffer, sigma_m, ep, phi);

            drawWindow(originalBuffer, edgeBuffer, true, false);
            break;
        }

        glfwSwapBuffers(window);
        glfwPollEvents();
    }
    nk_glfw3_shutdown(&glfw);
    originalBuffer.deleteFramebuffer();

    glfwTerminate();
    return 0;
}

void renderUI()
{
    glfwMakeContextCurrent(UI);
    // NuKlear GUI 
    nk_glfw3_new_frame(&glfw);

    if (nk_begin(ctx, "UI", nk_rect(5, 5, UI_WIDTH-10, UI_HEIGHT-10),
        NK_WINDOW_BORDER | NK_WINDOW_MOVABLE | NK_WINDOW_SCALABLE |
        NK_WINDOW_MINIMIZABLE | NK_WINDOW_TITLE))
    {
        // Contents
        static const float ratio[] = { 120, 200 };
        static const float ratio2[] = { 320 };
        static const float ratio3[] = { 160, 160 };
        static const float ratio4[] = { 20, 100, 10, 100, 20, 100 };
        nk_layout_row(ctx, NK_STATIC, 15, 2, ratio);

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

        nk_labelf(ctx, NK_TEXT_LEFT, "levels = %d", levels);
        nk_slider_int(ctx, 1, &levels, 10, 1);

        nk_layout_row(ctx, NK_STATIC, 15, 1, ratio2);
        nk_labelf(ctx, NK_TEXT_LEFT, "model Position (%.1f, %.1f, %.1f)", objPos.x, objPos.y, objPos.z);
      
        nk_layout_row(ctx, NK_STATIC, 15, 6, ratio4);
        nk_labelf(ctx, NK_TEXT_LEFT, "x : ");
        nk_slider_float(ctx, 0, &objPos.x, posMAX, 0.1f);
        nk_labelf(ctx, NK_TEXT_LEFT, "y : ");
        nk_slider_float(ctx, 0, &objPos.y, posMAX, 0.1f);
        nk_labelf(ctx, NK_TEXT_LEFT, "z : ");
        nk_slider_float(ctx, 0, &objPos.z, posMAX, 0.1f);

        nk_layout_row(ctx, NK_STATIC, 15, 1, ratio2);
        nk_labelf(ctx, NK_TEXT_LEFT, "model Rotation (%.1f, %.1f, %.1f)", objRot.x, objRot.y, objRot.z);
        nk_layout_row(ctx, NK_STATIC, 15, 6, ratio4);
        nk_labelf(ctx, NK_TEXT_LEFT, "x : ");
        nk_slider_float(ctx, 0, &objRot.x, 1, 0.1f);
        nk_labelf(ctx, NK_TEXT_LEFT, "y : ");
        nk_slider_float(ctx, 0, &objRot.y, 1, 0.1f);
        nk_labelf(ctx, NK_TEXT_LEFT, "z : ");
        nk_slider_float(ctx, 0, &objRot.z, 1, 0.1f);

        nk_layout_row(ctx, NK_STATIC, 15, 2, ratio);
        nk_labelf(ctx, NK_TEXT_LEFT, "light r = %.1f", r);
        nk_slider_float(ctx, 0, &r, 20, 0.1f);
        nk_labelf(ctx, NK_TEXT_LEFT, "light theta = %d", theta);
        nk_slider_int(ctx, 0, &theta, 360, 5);
        nk_labelf(ctx, NK_TEXT_LEFT, "light theta2 = %d", theta2);
        nk_slider_int(ctx, 0, &theta2, 360, 5);
        nk_layout_row(ctx, NK_STATIC, 15, 1, ratio2);
        nk_labelf(ctx, NK_TEXT_LEFT, "light Position (%.1f, %.1f, %.1f)", lightPos.x, lightPos.y, lightPos.z);

        nk_layout_row(ctx, NK_STATIC, 15, 2, ratio3);
        if (nk_button_label(ctx, "Gooch Shading")) shading_num = GOOCH;
        if (nk_button_label(ctx, "Phong Shading")) shading_num = PHONG;
        if (nk_button_label(ctx, "Gooch-toon Shading")) shading_num = GOOCHTOON;
        if (nk_button_label(ctx, "Phong-toon Shading")) shading_num = PHONGTOON;
        if (nk_button_label(ctx, "Original")) drawing_step = ORIGINAL;
        if (nk_button_label(ctx, "XDOG")) drawing_step = XDOG;
        if (nk_button_label(ctx, "Edge")) drawing_step = EDGE;

        nk_label(ctx, "Exprot : O", NK_TEXT_ALIGN_LEFT);

        // Color picker
        enum {BACK, LIGHT, SHADE};
        static int colorpck = BACK;

        nk_layout_row_dynamic(ctx, 30, 2);
        if (nk_option_label(ctx, "Background", colorpck == BACK)) colorpck = BACK;
        if (nk_option_label(ctx, "Light", colorpck == LIGHT)) colorpck = LIGHT;
        if (nk_option_label(ctx, "Shade", colorpck == SHADE)) colorpck = SHADE;

        nk_layout_space_begin(ctx, NK_DYNAMIC, 60, INT_MAX);
        nk_layout_space_push(ctx, nk_rect(0.0, 0.0, 0.25, 1.0));
        if(colorpck == BACK)
            nk_label(ctx, "Background Color:", NK_TEXT_LEFT);
        else if (colorpck == LIGHT)
            nk_label(ctx, "Light Color:", NK_TEXT_LEFT);
        else if (colorpck == SHADE)
            nk_label(ctx, "Shade Color:", NK_TEXT_LEFT);
        nk_layout_space_push(ctx, nk_rect(0.35 , 0.0, 0.30, 1.0));
        if (colorpck == BACK)
            nk_color_pick(ctx, &backcol, NK_RGBA);
        else if (colorpck == LIGHT)
            nk_color_pick(ctx, &lightcol, NK_RGBA);
        else if (colorpck == SHADE)
            nk_color_pick(ctx, &shadingcol, NK_RGBA);
        nk_layout_space_push(ctx, nk_rect(0.75, 0.0, 0.25, 1.0));

    }
    nk_end(ctx);

    glClearColor(0.2f, 0.2f, 0.2f, 1.0f);
    glClear(GL_COLOR_BUFFER_BIT);

    nk_glfw3_render(&glfw, NK_ANTI_ALIASING_ON, MAX_VERTEX_BUFFER, MAX_ELEMENT_BUFFER);
    glfwSwapBuffers(UI);
}
float s = 1;
const float Pi = 3.1415f;

void renderOBJ(Framebuffer f, Shader shader)
{
    float currentFrame = static_cast<float>(glfwGetTime());
    deltaTime = currentFrame - lastFrame;
    lastFrame = currentFrame;

    f.bindBuffer();

    glEnable(GL_DEPTH_TEST);

    glClearColor(backcol.r, backcol.g, backcol.b, backcol.a);
    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

    lightPos.z = r * cos(Pi * theta / 360.f);
    lightPos.y = r * sin(Pi * theta / 360.f);
    lightPos.x = r * cos(Pi * theta2 / 360.f);

    // model rendering
    shader.use();
    shader.setVec3("objectColor", shadingcol.r, shadingcol.g, shadingcol.b);
    shader.setVec3("lightColor", lightcol.r, lightcol.g, lightcol.b);
    shader.setVec3("lightPos", lightPos);
    shader.setVec3("viewPos", camera.Position);
    shader.setInt("levels", levels);

    // view/projection transformations
    glm::mat4 projection = glm::perspective(glm::radians(camera.Zoom), (float)SCR_WIDTH / (float)SCR_HEIGHT, 0.1f, 100.0f);
    glm::mat4 view = camera.GetViewMatrix();
    shader.setMat4("projection", projection);
    shader.setMat4("view", view);

    // render the loaded model
    glm::mat4 model = glm::mat4(1.0f);
    model = glm::translate(model, objPos - glm::vec3(posMAX/2));
    model = glm::rotate(model, glm::radians(90.f), objRot);
    model = glm::scale(model, (glm::vec3(1.0f, 1.0f, 1.0f)*glm::vec3(s)));
    shader.setMat4("model", model);
    ourModel.Draw(shader);
}
void drawWindow(Framebuffer colorB, Framebuffer edgeB, bool isEdge, bool blend)
{
    glBindFramebuffer(GL_FRAMEBUFFER, 0);
    
    glClearColor(0.5f, 1.0f, 0.5f, 1.0f);
    glClear(GL_COLOR_BUFFER_BIT);

    if (!blend)
    {
        drawColorShader.use();
        glBindVertexArray(vao);
        colorB.bind();
    }
    else
    {
        blendShader.use();

        glActiveTexture(GL_TEXTURE0 + 1);
        colorB.bind();
        blendShader.setInt("color", 1);

        glActiveTexture(GL_TEXTURE0);
        edgeB.bind();
        blendShader.setInt("edge", 0);
    }

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
    std::cout << SCR_WIDTH << " " << SCR_HEIGHT;
    if (window == NULL)
    {
        std::cout << "Failed to create GLFW window" << std::endl;
        glfwTerminate();
    }
    UI = glfwCreateWindow(UI_WIDTH, UI_HEIGHT, "UI", NULL, NULL);
    if (UI == NULL)
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
    goochShader.loadShaders("model.vert", "goochShading.frag");
    goochToonShader.loadShaders("model.vert", "toonShading.frag");
    phongToonShader.loadShaders("model.vert", "phongToonShading.frag");
    drawColorShader.loadShaders("draw.vert", "draw.frag");
    drawEdgeShader.loadShaders("draw.vert", "drawEdge.frag");
    blendShader.loadShaders("draw.vert", "blend.frag");
 
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

    if (glfwGetKey(window, GLFW_KEY_MINUS) == GLFW_PRESS)
        s -= 0.01f;
    if (glfwGetKey(window, GLFW_KEY_EQUAL) == GLFW_PRESS)
        s+= 0.01f;

    if (glfwGetKey(window, GLFW_KEY_O) == GLFW_PRESS)
    {
        std::cout << drawing_step << std::endl;
        if (drawing_step == 0) saveImage(colorPath, window);
        if (drawing_step == 1) saveImage(exportPath, window);
        if (drawing_step == 2)  saveImage(edgePath, window);
    }
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

    camera.ProcessMouseMovement(xoffset, yoffset);
}

// glfw: whenever the mouse scroll wheel scrolls, this callback is called
// ----------------------------------------------------------------------
void scroll_callback(GLFWwindow* window, double xoffset, double yoffset)
{
    camera.ProcessMouseScroll(static_cast<float>(yoffset));
}
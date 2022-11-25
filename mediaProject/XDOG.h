#ifndef XDOG_H
#define XDOG_H

#include <GL/glew.h>

#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>

#include <glm/gtc/type_ptr.hpp>

#include "shader.h"
#include "framebuffer.h"

class XDoG
{
public :
    int SCR_WIDTH, SCR_HEIGHT;
    GLuint vao, vbo, ebo;

    Shader gaussianProgram, sobelProgram, gmProgram, etf0Program, etfProgram, dogProgram, xdogProgram;

    void init()
    {
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
    }

    void setWidth(int width)
    {
        SCR_WIDTH = width;
    }
    void setHeight(int height)
    {
        SCR_HEIGHT = height;
    }

void applyGaussian(Framebuffer swichB, Framebuffer switchB2, Framebuffer orignB, int order, float sigma)
{
    if (order == 0) swichB.bindBuffer();
    else switchB2.bindBuffer();

    glDisable(GL_DEPTH_TEST);
    glClearColor(0.5f, 0.5f, 0.5f, 1.0f);
    glClear(GL_COLOR_BUFFER_BIT);

    gaussianProgram.use();
    glBindVertexArray(vao);

    if (order == 0) orignB.bind();
    else swichB.bind();

    gaussianProgram.setInt("len", SCR_WIDTH);
    gaussianProgram.setInt("order", order);
    gaussianProgram.setFloat("sigma", sigma);

    glDrawElements(GL_TRIANGLES, 6, GL_UNSIGNED_INT, 0);
}
void applySobel(Framebuffer sobelxB, Framebuffer sobelyB, Framebuffer sourceB, int order)
{
    if (order == 0) sobelxB.bindBuffer();
    else sobelyB.bindBuffer();

    glClearColor(0.5f, 0.5f, 0.5f, 1.0f);
    glClear(GL_COLOR_BUFFER_BIT);

    sobelProgram.use();
    glBindVertexArray(vao);

    sourceB.bind();

    sobelProgram.setInt("width", SCR_WIDTH);
    sobelProgram.setInt("height", SCR_HEIGHT);
    sobelProgram.setInt("order", order);

    glDrawElements(GL_TRIANGLES, 6, GL_UNSIGNED_INT, 0);
}
void calculateGM(Framebuffer gmB, Framebuffer sobelxB, Framebuffer sobelyB)
{
    gmB.bindBuffer();
    glClearColor(0.5f, 0.5f, 0.5f, 1.0f);
    glClear(GL_COLOR_BUFFER_BIT);

    gmProgram.use();
    glBindVertexArray(vao);

    glActiveTexture(GL_TEXTURE0 + 1);
    sobelxB.bind();
    gmProgram.setInt("sobelx", 1);

    glActiveTexture(GL_TEXTURE0);
    sobelyB.bind();
    gmProgram.setInt("sobely", 0);

    glDrawElements(GL_TRIANGLES, 6, GL_UNSIGNED_INT, 0);
}
void calculateETF0(Framebuffer etf0B, Framebuffer gmB, Framebuffer sobelxB, Framebuffer sobelyB)
{
    etf0B.bindBuffer();
    glClearColor(0.5f, 0.5f, 0.5f, 1.0f);
    glClear(GL_COLOR_BUFFER_BIT);

    etf0Program.use();
    glBindVertexArray(vao);

    glActiveTexture(GL_TEXTURE0 + 2);
    gmB.bind();
    etf0Program.setInt("magnitude", 2);

    glActiveTexture(GL_TEXTURE0 + 1);
    sobelxB.bind();
    etf0Program.setInt("sobelx", 1);

    glActiveTexture(GL_TEXTURE0);
    sobelyB.bind();
    etf0Program.setInt("sobely", 0);

    glDrawElements(GL_TRIANGLES, 6, GL_UNSIGNED_INT, 0);
}
void calculateETF(Framebuffer switchB, Framebuffer etf0B, Framebuffer gmB, float etf)
{
    for (int i = 0; i < etf; i++)
    {
        if (i % 2 == 0)
        {
            switchB.bindBuffer();
            glClearColor(0.5f, 0.5f, 0.5f, 1.0f);
            glClear(GL_COLOR_BUFFER_BIT);

            etfProgram.use();
            glBindVertexArray(vao);

            glActiveTexture(GL_TEXTURE0 + 1);
            etf0B.bind();
            etfProgram.setInt("ETF", 1);

            glActiveTexture(GL_TEXTURE0);
            gmB.bind();
            etfProgram.setInt("magnitude", 0);

            etfProgram.setInt("width", SCR_WIDTH);

            glDrawElements(GL_TRIANGLES, 6, GL_UNSIGNED_INT, 0);
        }
        else
        {
            etf0B.bindBuffer();
            glClearColor(0.5f, 0.5f, 0.5f, 1.0f);
            glClear(GL_COLOR_BUFFER_BIT);

            etfProgram.use();
            glBindVertexArray(vao);

            glActiveTexture(GL_TEXTURE0 + 1);
            switchB.bind();
            etfProgram.setInt("ETF", 1);

            glActiveTexture(GL_TEXTURE0);
            gmB.bind();
            etfProgram.setInt("magnitude", 0);

            etfProgram.setInt("width", SCR_WIDTH);

            glDrawElements(GL_TRIANGLES, 6, GL_UNSIGNED_INT, 0);
        }
    }
}
void applyDOG(Framebuffer dogB, Framebuffer etf0B, Framebuffer originB, float sigma_c, float k, float p)
{
    dogB.bindBuffer();
    glClearColor(0.5f, 0.5f, 0.5f, 1.0f);
    glClear(GL_COLOR_BUFFER_BIT);

    dogProgram.use();
    glBindVertexArray(vao);

    glActiveTexture(GL_TEXTURE0 + 1);
    etf0B.bind();
    dogProgram.setInt("ETF", 1);

    glActiveTexture(GL_TEXTURE0);
    originB.bind();
    dogProgram.setInt("source", 0);

    dogProgram.setInt("width", SCR_WIDTH);
    dogProgram.setFloat("sigma_c", sigma_c);
    dogProgram.setFloat("k", k);
    dogProgram.setFloat("p", p);

    glDrawElements(GL_TRIANGLES, 6, GL_UNSIGNED_INT, 0);
}
void applyXDOG(Framebuffer edgeB, Framebuffer etf0B, Framebuffer dogB, float sigma_m, float epsilon, float phi)
{
    edgeB.bindBuffer();
    glClearColor(0.5f, 0.5f, 0.5f, 1.0f);
    glClear(GL_COLOR_BUFFER_BIT);

    xdogProgram.use();
    glBindVertexArray(vao);

    glActiveTexture(GL_TEXTURE0 + 1);
    etf0B.bind();
    xdogProgram.setInt("ETF", 1);

    glActiveTexture(GL_TEXTURE0);
    dogB.bind();
    xdogProgram.setInt("dog", 0);

    xdogProgram.setInt("width", SCR_WIDTH);
    xdogProgram.setFloat("sigma_m", sigma_m);
    xdogProgram.setFloat("epsilon", epsilon);
    xdogProgram.setFloat("phi", phi);

    glDrawElements(GL_TRIANGLES, 6, GL_UNSIGNED_INT, 0);
}

};


#endif
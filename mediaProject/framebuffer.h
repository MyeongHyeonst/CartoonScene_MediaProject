#ifndef FRAMEBUFFER_H
#define FRAMEBUFFER_H

#include <glad/glad.h>

#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>

#include <glm/gtc/type_ptr.hpp>
#include <opencv2/core.hpp>
#include <opencv2/opencv.hpp>
#include <opencv2/highgui.hpp>

#include "texture.h"

class Framebuffer :public filter::Texture {
private:
	GLuint fbo;

public:
	void generateBuffer(int width, int height)
	{
		glGenFramebuffers(1, &fbo);
		this->bindBuffer();
		this->setNULL(width, height);
		this->attach();
	}

	void bindBuffer()
	{
		glBindFramebuffer(GL_FRAMEBUFFER, fbo);
	}
	void attach()
	{
		glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_TEXTURE_2D, texture, 0);
	}
};

#endif
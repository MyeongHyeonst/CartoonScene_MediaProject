#ifndef FRAMEBUFFER_H
#define FRAMEBUFFER_H

#include <GL/glew.h>

#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>

#include <glm/gtc/type_ptr.hpp>

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

		if (glCheckFramebufferStatus(GL_FRAMEBUFFER) != GL_FRAMEBUFFER_COMPLETE)
			cout << "ERROR::FRAMEBUFFER:: Framebuffer is not complete!" << endl;
		glBindFramebuffer(GL_FRAMEBUFFER, 0);
	}
	void generateBufferwithHDR(int width, int height)
	{
		glGenFramebuffers(1, &fbo);
		this->bindBuffer();
		this->setNULLHDR(width, height);
		this->attach();

		if (glCheckFramebufferStatus(GL_FRAMEBUFFER) != GL_FRAMEBUFFER_COMPLETE)
			cout << "ERROR::FRAMEBUFFER:: Framebuffer is not complete!" << endl;
		glBindFramebuffer(GL_FRAMEBUFFER, 0);
	}
	void generateBufferwithDepth(int width, int height)
	{
		glGenFramebuffers(1, &fbo);
		this->bindBuffer();
		this->setNULL(width, height);
		this->attach();
		this->setDepth(width, height);
		this->attachDepth();
		if (glCheckFramebufferStatus(GL_FRAMEBUFFER) != GL_FRAMEBUFFER_COMPLETE)
			cout << "ERROR::FRAMEBUFFER:: Framebuffer is not complete!" << endl;
	}

	void bindBuffer()
	{
		glBindFramebuffer(GL_FRAMEBUFFER, fbo);
	}
	void attach()
	{
		glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_TEXTURE_2D, texture, 0);
	}
	void attachDepth()
	{
		glFramebufferTexture2D(GL_FRAMEBUFFER, GL_DEPTH_STENCIL_ATTACHMENT, GL_TEXTURE_2D, depth, 0);
	}
	void deleteFramebuffer()
	{
		glDeleteFramebuffers(1, &fbo);
	}
};

#endif
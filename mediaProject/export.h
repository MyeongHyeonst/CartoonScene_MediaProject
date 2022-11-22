#ifndef EXPORT_H
#define EXPORT_H

#define STBI_MSC_SECURE_CRT
#define STB_IMAGE_WRITE_IMPLEMENTATION

#include <GL/glew.h>
#include <GLFW/glfw3.h>

#include <stb/stb_image_write.h>

#include <iostream>
#include <vector>

void saveImage(char*filepath, GLFWwindow*w)
{
	int width, height;
	
	glfwGetFramebufferSize(w, &width, &height);
	GLsizei nrChannels = 3;
	GLsizei stride = nrChannels * width;
	stride += (stride % 4) ? (4 - stride % 4) : 0;
	GLsizei bufferSize = stride * height;
	std::vector<char> buffer(bufferSize);
	glPixelStorei(GL_PACK_ALIGNMENT, 4);
	glReadBuffer(GL_FRONT);
	glReadPixels(0, 0, width, height, GL_RGB, GL_UNSIGNED_BYTE, buffer.data());
	stbi_flip_vertically_on_write(true);
	stbi_write_png(filepath, width, height, nrChannels, buffer.data(), stride);

	std::cout << "Export image to " << filepath << std::endl;
}





#endif
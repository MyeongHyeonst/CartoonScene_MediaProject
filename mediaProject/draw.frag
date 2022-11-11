#version 330 core

out vec4 FragColor;

in vec2 TexCoord;

uniform sampler2D image;

void main(void)
{		
	FragColor = vec4(texture(image, TexCoord));
	//FragColor = vec4(1.0, 1.0, 1.0, 1.0);
}

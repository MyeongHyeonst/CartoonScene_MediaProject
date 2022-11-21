#version 330 core

out vec4 FragColor;

in vec2 TexCoord;

uniform sampler2D sobelx;
uniform sampler2D sobely;

float intensity(vec4 color)
{
	return color.x;
}

void main(void)
{
	float color = sqrt(pow(intensity(texture(sobelx, TexCoord)), 2) + pow(intensity(texture(sobely, TexCoord)), 2));
	FragColor = vec4(color, color, color, 1.0);
}

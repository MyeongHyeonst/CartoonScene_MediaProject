#version 330 core

out vec4 FragColor;

in vec2 TexCoord;

uniform sampler2D color;

void main(void)
{		
	FragColor = vec4(texture(color, TexCoord).rgb, 1.0);
}

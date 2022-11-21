#version 330 core

out vec4 FragColor;

in vec2 TexCoord;

uniform sampler2D edge;

void main(void)
{		
	FragColor = vec4(0, 0, 0, 1-texture(edge, TexCoord).r);
}

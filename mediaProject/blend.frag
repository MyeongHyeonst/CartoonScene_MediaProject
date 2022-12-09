#version 330 core

out vec4 FragColor;

in vec2 TexCoord;

uniform sampler2D color;
uniform sampler2D edge;

void main(void)
{		
	FragColor = mix(texture(edge, TexCoord), texture(color, TexCoord), 0.5);
}

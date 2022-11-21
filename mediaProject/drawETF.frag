#version 330 core

out vec4 FragColor;

in vec2 TexCoord;

uniform sampler2D ourTexture;


void main(void)
{		
	FragColor = texture(ourTexture, TexCoord) *.5 +.5;
}

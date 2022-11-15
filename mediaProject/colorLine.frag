#version 330 core

out vec4 FragColor;

in vec2 TexCoord;

uniform sampler2D xdog;
uniform sampler2D color;
uniform float ratio;

void main(void)
{
	vec4 inverse = vec4(1, 1, 1, 0) - texture(xdog, TexCoord);

	FragColor = vec4(texture(color, TexCoord).xyz, inverse);
}

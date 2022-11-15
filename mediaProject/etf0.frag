#version 330 core

out vec4 FragColor;

in vec2 TexCoord;

uniform sampler2D sobelx;
uniform sampler2D sobely;
uniform sampler2D magnitude;

float intensity(vec4 color)
{
	return color.x;
}

void main(void)
{
	float sx = intensity(texture(sobelx, TexCoord));
	float sy = intensity(texture(sobely, TexCoord));
	float mag = intensity(texture(magnitude, TexCoord));

	if(mag != 0)
	{
		FragColor = vec4(-sy / mag , sx / mag , 0, 0);
	}
	else
		FragColor = vec4(0);
}

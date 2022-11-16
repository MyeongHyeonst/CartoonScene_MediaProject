#version 330 core

out vec4 FragColor;

in vec2 TexCoord;

uniform sampler2D ETF;
uniform sampler2D magnitude;
uniform int width;

int spatialWeight(vec2 tc, vec2 n, float r) // gaussian으로 바꾸기
{
	if (pow(tc.x - n.x, 2) + pow(tc.y - n.y, 2) < r*r)
		return 1;
	else
		return 0;
}

float magnitudeWeight(float x, float y)
{
	return (1 + atan(y - x)) / 2; // eta = 1
}

void main(void)
{
	float r = 3.0f/width;
	float wx ,wy = 0;

	for(float a = -r; a <= r; a += 1.0f/width) 
	for(float b = -r; b <= r; b += 1.0f/width)
	{
		vec2 n = vec2(TexCoord.x + a, TexCoord.y + b);

		float dot = texture(ETF, TexCoord).x * texture(ETF, n).x
					+ texture(ETF, TexCoord).y * texture(ETF, n).y;

		float weight = spatialWeight(TexCoord, n, r)
					* magnitudeWeight(texture(magnitude, TexCoord).x, texture(magnitude, n).x)
					* dot;

		wx += texture(ETF,n).x * weight;
		wy += texture(ETF,n).y * weight;
	}

	float len = sqrt(wx * wx + wy * wy);
	if (len == 0) len = 1;

	FragColor = vec4(wx/len, wy/len, 0, 1.0);
}


#version 330 core

out vec4 FragColor;

in vec2 TexCoord;

uniform sampler2D ETF;
uniform sampler2D source;

uniform int width;
uniform float sigma_c;
uniform float k;
uniform float p;

float intensity(vec4 color)
{
	return (color.x + color.y + color.z)/3;//sqrt(color.x * color.x + color.y + color.y + color.z + color.z);
}

void main(void)
{
	int T = int(4 * sigma_c);

	
	vec2 etf = texture(ETF, TexCoord).xy;
	vec2 etf_p = vec2(etf.y, -etf.x);

	float nsum_t = 0;
	float sum_c = 0;
	float sum_k = 0;

	for (int t = -T; t <= T; t++)
	{
		sum_c += exp(-(t * t) / (2. * sigma_c * sigma_c));
		sum_k += exp(-(t * t) / (2. * sigma_c * k * sigma_c * k));
	}

	for (int t = -T; t <= T; t++)
	{
		vec2 next = TexCoord + etf_p * t/width;

		nsum_t += ((1 + p) * exp(-(t * t) / (2. * sigma_c * sigma_c))/sum_c 
						 - p * exp(-(t * t) / (2. * sigma_c * k * sigma_c * k))/sum_k)
						   *intensity(texture(source, next));
	}

	FragColor = vec4(nsum_t, nsum_t, nsum_t, 0);
}


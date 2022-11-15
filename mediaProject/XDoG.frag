#version 330 core

out vec4 FragColor;

in vec2 TexCoord;

uniform sampler2D ETF;
uniform sampler2D dog;
uniform int width;
uniform float sigma_m;
uniform float epsilon;
uniform float phi;


void main(void)
{
	int S = int(4 * sigma_m);
	
	vec2 next = TexCoord;
	float sum_s = 0;
	vec4 nsum_s = vec4(0, 0, 0, 0);

	for(int s = 0; s<=S; s++)
	{
		vec2 etf = texture(ETF, next).xy;
		nsum_s += exp(-(s * s) / (2. * sigma_m * sigma_m)) * texture(dog, next);
		sum_s += exp(-(s * s) / (2. * sigma_m * sigma_m));
		next += etf/width;
	}

	next = TexCoord - texture(ETF, TexCoord).xy / width;
	
	
	for(int s = -1; s>=-S; s--)
	{
		vec2 etf = texture(ETF, next).xy;
		nsum_s += exp(-(s * s) / (2. * sigma_m * sigma_m)) * texture(dog, next);
		sum_s += exp(-(s * s) / (2. * sigma_m * sigma_m));
		next -= etf/width;
	}
	
	float t = nsum_s.x/sum_s;

	if (t >= epsilon)
		FragColor = vec4(1, 1, 1, 0);
	else
		FragColor = vec4(1 + 100 * atan(phi * (t - epsilon)), 1 + 100 * atan(phi * (t - epsilon)), 1 + 100 * atan(phi * (t - epsilon)), 0); 
}


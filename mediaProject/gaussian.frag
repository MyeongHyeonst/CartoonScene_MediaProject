#version 330 core

out vec4 FragColor;

in vec2 TexCoord;

uniform sampler2D ourTexture;
uniform int len;
uniform float sigma;
uniform int order;

void main(void)
{
	float sigma_r = sigma * 4;
	vec4 gx = vec4(0.0,0.0,0.0,0.0);
	vec4 sum = vec4(0.0,0.0,0.0,0.0);
	float nsum = 0;

	vec2 p;
	
	for(float t = -sigma_r/len; t <= sigma_r/len; t += 1.0f/len)
	{
		if(order == 0)
		{
			p = vec2(TexCoord.x+t, TexCoord.y);
			sum += exp(-(t * t) / (2. * sigma * sigma)) 
							* texture(ourTexture, p);
			nsum += exp(-(t * t) / (2. * sigma * sigma));
		}
		else
		{
			p = vec2(TexCoord.x, TexCoord.y+t);
			sum += exp(-(t * t) / (2. * sigma * sigma))
							* texture(ourTexture, p);
			nsum += exp(-(t * t) / (2. * sigma * sigma));
		}
	}
	
	FragColor = sum/nsum;
}

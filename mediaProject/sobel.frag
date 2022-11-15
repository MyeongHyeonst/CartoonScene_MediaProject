#version 330 core

out vec4 FragColor;

in vec2 TexCoord;

uniform sampler2D ourTexture;
uniform int width;
uniform int height;
uniform int order;

float intensity(vec4 color)
{
	return (color.x + color.y + color.z)/3;//sqrt(color.x * color.x + color.y + color.y + color.z + color.z);
}

void main(void)
{
	float dx = 1.0f / width;
	float dy = 1.0f / height;
	float color;
	if(order == 0)
		color = intensity(texture(ourTexture, vec2(TexCoord.x-dx, TexCoord.y-dy)))
				+ 2 * intensity(texture(ourTexture, vec2(TexCoord.x-dx, TexCoord.y)))
				+ intensity(texture(ourTexture, vec2(TexCoord.x-dx, TexCoord.y+dy)))
				+ (-1) * intensity(texture(ourTexture, vec2(TexCoord.x+dx, TexCoord.y-dy)))
				+ (-2) * intensity(texture(ourTexture, vec2(TexCoord.x+dx, TexCoord.y)))
				+ (-1) * intensity(texture(ourTexture, vec2(TexCoord.x+dx, TexCoord.y+dy)));		
	else
		color = intensity(texture(ourTexture, vec2(TexCoord.x-dx, TexCoord.y-dy)))
				+ 2 * intensity(texture(ourTexture, vec2(TexCoord.x, TexCoord.y-dy)))
				+ intensity(texture(ourTexture, vec2(TexCoord.x+dx, TexCoord.y-dy)))
				+ (-1) * intensity(texture(ourTexture, vec2(TexCoord.x-dx, TexCoord.y+dy)))
				+ (-2) * intensity(texture(ourTexture, vec2(TexCoord.x, TexCoord.y+dy)))
				+ (-1) * intensity(texture(ourTexture, vec2(TexCoord.x+dx, TexCoord.y+dy)));	
	
	FragColor = vec4(color, color, color, 0);
}

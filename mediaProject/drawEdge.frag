#version 330 core

out vec4 FragColor;

in vec2 TexCoord;

uniform sampler2D edge;

void main(void)
{		
	//FragColor = vec4(vec3(1.0 - texture(image, TexCoord)), 1.0);
	//FragColor = vec4(texture(edge, TexCoord).rgb, 1.0);
	//FragColor = vec4(1.0, 1.0, 1.0, 1.0);

	FragColor = vec4(0, 0, 0, 1-texture(edge, TexCoord).r);

	//FragColor = mix(texture(edge, TexCoord), texture(color, TexCoord), 0.5);
}

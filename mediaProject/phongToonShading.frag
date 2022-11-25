#version 330 core
out vec4 FragColor;

in vec3 Normal;
in vec3 FragPos;
in vec2 TexCoords;

uniform vec3 lightPos; 
uniform vec3 viewPos; 
uniform vec3 lightColor;
uniform vec3 objectColor;
uniform int levels;
uniform sampler2D texture_diffuse1;

void main()
{   
    float scaleFactor = 1.0/levels;

       // ambient
    float ambientStrength = 0.1;
    vec3 ambient = ambientStrength * lightColor;
  	
    // diffuse 
    vec3 norm = normalize(Normal);
    vec3 lightDir = normalize(lightPos - FragPos);
    float diff = max(dot(norm, lightDir), 0.0);
    vec3 diffuse = diff * lightColor * floor(dot(norm, lightDir) * levels) * scaleFactor;
 
    // specular
    float specularStrength = 0.5;
    vec3 viewDir = normalize(viewPos - FragPos);
    vec3 reflectDir = reflect(-lightDir, norm);  
    float spec = pow(max(dot(viewDir, reflectDir), 0.0), 32);
    vec3 specular = specularStrength * spec * lightColor;  
        
    vec3 result = (ambient + diffuse + specular) * texture(texture_diffuse1, TexCoords).rgb;

    FragColor = vec4(result, 1.0);
}
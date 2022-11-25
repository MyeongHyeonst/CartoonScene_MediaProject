#version 330 core
out vec4 FragColor;

in vec3 Normal;
in vec3 FragPos;
in vec2 TexCoords;

uniform vec3 lightPos; 
uniform vec3 viewPos; 
uniform vec3 lightColor;
uniform vec3 objectColor;
uniform sampler2D texture_diffuse1;

void main()
{   
    vec3 norm = normalize(Normal);
    vec3 lightDir = normalize(lightPos - FragPos);
    vec3 viewDir = normalize(viewPos - FragPos);

    float t = (dot(norm, lightDir)+ 1)/2;
    vec3 r = 2 * dot(norm, lightDir) * norm - lightDir;
    float s = clamp(100*dot(r, viewDir)-97, 0, 1);

    // ambient
    float ambientStrength = 0.1;
    vec3 ambient = ambientStrength * vec3(1, 1, 1);

    // diffuse 
    float diff = max(dot(norm, lightDir), 0.0);
    vec3 diffuse = diff * vec3(1, 1, 1);

    // specular
    float specularStrength = 0.5;
    vec3 reflectDir = reflect(-lightDir, norm);  
    float spec = pow(max(dot(viewDir, reflectDir), 0.0), 32);
    vec3 specular = specularStrength * spec * vec3(1, 1, 1);  

    vec3 light = ambient + diffuse + specular;
    vec3 result =  (light * lightColor + (1-light) * objectColor) * texture(texture_diffuse1, TexCoords).rgb; //(t*warmColor+(1-t)*coolColor);

    FragColor = vec4(result, 1.0);
}
#version 330 core

in vec3 vColor;
in vec3 vNormal;
in vec3 FragPos;

out vec4 FragColor;

vec3 lightColor = vec3(1.0);
vec3 lightDir = -normalize(vec3(0.0, 100.0, 0.0) - vec3(32.0));

float ambientStrength = 0.05;
float diffuseStrength = 0.7;
float specularStrength = 0.25;

uniform vec3 viewPos;
uniform float u_time;

void main()
{
    // offset the normal a tiny bit, so that the color of faces opposing lightDir is not completely
    // flat
    vec3 normal = normalize(vNormal);

    // Blinn-Phong lighting
    // Ambient
    vec3 ambient = lightColor*vColor;

    // Diffuse
    float diff = max(dot(normal, lightDir), 0.0);
    vec3 diffuse = vColor * diff;

    // Blinn Specular
    vec3 viewDir = normalize(viewPos - FragPos);
    vec3 halfwayDir = normalize(lightDir + viewDir);
    float spec = pow(max(dot(normal, halfwayDir), 0.0), 64.0);
    vec3 specular = lightColor * vColor * spec;

    // Final color
    vec3 color = ambient * ambientStrength + diffuse * diffuseStrength + specular * specularStrength;
    
    float gamma = 2.2;
    FragColor.rgb = pow(color.rgb, vec3(1.0/gamma));
}

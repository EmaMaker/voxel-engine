#version 330 core

out vec4 FragColor;

in vec3 TexCoord;
in vec3 Normal;
in vec3 FragPos;

vec3 lightColor = vec3(1.0);
vec3 lightDir = -normalize(vec3(0.0, 100.0, 0.0) - vec3(32.0));

float ambientStrength = 0.1;
float diffuseStrength = 0.8;
float specularStrength = 0.1;

uniform vec3 viewPos;
uniform float u_time;
uniform sampler2DArray textureArray;

float gamma = 2.2;

void main(){
    // Load the texture
    // anti-gamma-correction of the texture. Without this it would be gamma corrected twice!
    vec3 vColor = pow(texture(textureArray, TexCoord).rgb, vec3(gamma));
    
    vec3 normal = normalize(Normal);

    /* Start of Blinn-Phong lighting */
    // Ambient
    vec3 ambient = lightColor*vColor;

    // Diffuse
    float diff = max(dot(normal, lightDir), 0.0);
    vec3 diffuse = vColor * diff;

    // Blinn Specular
    vec3 viewDir = normalize(viewPos - FragPos);
    vec3 halfwayDir = normalize(lightDir + viewDir);
    float spec = pow(max(dot(normal, halfwayDir), 0.0), 32.0);
    vec3 specular = lightColor * vColor * spec;

    // Final color
    vec3 color = ambient * ambientStrength + diffuse * diffuseStrength + specular * specularStrength;
    FragColor.rgb = pow(color, vec3(1.0/gamma));
}

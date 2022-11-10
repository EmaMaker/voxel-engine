#version 330 core

in vec4 vColor;
out vec4 FragColor;

void main()
{
    FragColor = vColor;
    // FragColor=  vec4(1.0f, 1.0f, 1.0f, 1.0f);
}
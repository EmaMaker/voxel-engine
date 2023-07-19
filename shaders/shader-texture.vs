#version 330 core

layout (location = 0) in uvec3 aPos;
layout (location = 1) in uvec3 aExtents;
layout (location = 2) in uvec2 aInfo;

uniform mat4 model;

out VS_OUT {
    vec3 Extents;
    vec3 Normal;
    float BlockType;
} vs_out;

void main()
{
    //vNormal = mat3(transpose(inverse(model))) * aNormal;
    vs_out.Extents = aExtents;
    vs_out.BlockType = aInfo.y;

    if(aExtents.x == 0.0) vs_out.Normal = vec3(1.0 - 2.0*aInfo.x, 0.0, 0.0);
    else if(aExtents.y == 0.0) vs_out.Normal = vec3(0.0, 1.0 - 2.0*aInfo.x, 0.0);
    else vs_out.Normal = vec3(0.0, 0.0, 1.0 - 2.0*aInfo.x);
    vs_out.Normal = mat3(transpose(inverse(model))) * vs_out.Normal;

    gl_Position = model * vec4(aPos, 1.0);
}

#version 330 core

layout (points) in;
layout (triangle_strip, max_vertices = 4) out;

in VS_OUT{
    vec3 Extents;
    vec3 Normal;
    float BlockType;
} gs_in[];

out vec3 TexCoord;
out vec3 Normal;
out vec3 FragPos;

uniform mat4 view;
uniform mat4 projection;

void main(){
    Normal = gs_in[0].Normal;

    TexCoord = vec3(0.0, 0.0, gs_in[0].BlockType);
    gl_Position = gl_in[0].gl_Position;
    FragPos = vec3(gl_Position);
    gl_Position = projection * view * gl_Position;
    EmitVertex();

    if(gs_in[0].Extents.x == 0){
	TexCoord = vec3(0.0, gs_in[0].Extents.z, gs_in[0].BlockType);
	gl_Position = gl_in[0].gl_Position + vec4(0.0, 0.0, gs_in[0].Extents.z, 0.0);
	FragPos = vec3(gl_Position);
	gl_Position = projection * view * gl_Position;
	EmitVertex();

	TexCoord = vec3(gs_in[0].Extents.y, 0.0,  gs_in[0].BlockType);
	gl_Position = gl_in[0].gl_Position + vec4(0.0, gs_in[0].Extents.y, 0.0, 0.0);
	FragPos = vec3(gl_Position);
	gl_Position = projection * view * gl_Position;
	EmitVertex();

	TexCoord = vec3(gs_in[0].Extents.y, gs_in[0].Extents.z,  gs_in[0].BlockType);
    }
    else if(gs_in[0].Extents.y == 0){
	TexCoord = vec3(0.0, gs_in[0].Extents.z, gs_in[0].BlockType);
	gl_Position = gl_in[0].gl_Position + vec4(0.0, 0.0, gs_in[0].Extents.z, 0.0);
	FragPos = vec3(gl_Position);
	gl_Position = projection * view * gl_Position;
	EmitVertex();

	TexCoord = vec3(gs_in[0].Extents.x, 0.0, gs_in[0].BlockType);
	gl_Position = gl_in[0].gl_Position + vec4(gs_in[0].Extents.x, 0.0, 0.0, 0.0);
	FragPos = vec3(gl_Position);
	gl_Position = projection * view * gl_Position;
	EmitVertex();

	TexCoord = vec3(gs_in[0].Extents.x, gs_in[0].Extents.z,  gs_in[0].BlockType);
    }
    else{
	TexCoord = vec3(gs_in[0].Extents.x, 0.0, gs_in[0].BlockType);
	gl_Position = gl_in[0].gl_Position + vec4(gs_in[0].Extents.x, 0.0, 0.0, 0.0);
	FragPos = vec3(gl_Position);
	gl_Position = projection * view * gl_Position;
	EmitVertex();

	TexCoord = vec3(0.0, gs_in[0].Extents.y, gs_in[0].BlockType);
	gl_Position = gl_in[0].gl_Position + vec4(0.0, gs_in[0].Extents.y, 0.0, 0.0);
	FragPos = vec3(gl_Position);
	gl_Position = projection * view * gl_Position;
	EmitVertex();

	TexCoord = vec3(gs_in[0].Extents.x, gs_in[0].Extents.y,  gs_in[0].BlockType);
    }

    gl_Position = gl_in[0].gl_Position + vec4(gs_in[0].Extents, 0.0);
    FragPos = vec3(gl_Position);
    gl_Position = projection * view * gl_Position;
    EmitVertex();

    EndPrimitive();
}

#version 460 core
layout (location = 0) in vec3 aPos;
layout (location = 1) in vec2 aTexCoords;

layout (std140) uniform Matrices
{
    mat4 projection;
    mat4 view;
};

out vec2 TexCoords;

uniform mat4 model;

void main()
{
    vec3 pos = vec3(model * vec4(aPos, 1.0)); 
    gl_Position = projection * view * vec4(pos, 1.0);
    TexCoords = aTexCoords;
}
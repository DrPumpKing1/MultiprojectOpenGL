#version 460 core
layout (location = 0) in vec3 aPos;
layout (location = 1) in vec3 aNormal;
layout (location = 2) in vec2 aTexCoords;
layout (location = 3) in mat4 aInstanceMatrix;

layout (std140) uniform Matrices
{
    mat4 projection;
    mat4 view;
};

out VS_OUT {
    vec3 FragPos;
    vec3 Normal;
    vec2 TexCoords;
} vs_out;

void main()
{
    vs_out.TexCoords = aTexCoords;
    vs_out.Normal = aNormal;
    vs_out.FragPos = vec3(aInstanceMatrix * vec4(aPos, 1.0));
    gl_Position = projection * view * vec4(vs_out.FragPos, 1.0); 
}
#version 460 core
layout (location = 0) in vec3 aPos;
layout (location = 1) in vec3 aNormal;
layout (location = 2) in vec2 aTexCoords;

#define MAX_NR_SHADOWS 4

out VS_OUT {
    vec4 FragPosLightSpace[MAX_NR_SHADOWS];
    vec3 FragPos;
    vec3 Normal;
    vec2 TexCoords;
} vs_out;

uniform mat4 model;
uniform mat4 projection;
uniform mat4 view;

uniform int numShadows;
uniform mat4 lightSpaceMatrix[MAX_NR_SHADOWS];

void main()
{
    vs_out.FragPos = vec3(model * vec4(aPos, 1.0));
    vs_out.Normal = mat3(transpose(inverse(model))) * aNormal;  
    vs_out.TexCoords = aTexCoords;
    for(int i = 0; i < numShadows; i++)
    {
        vs_out.FragPosLightSpace[i] = lightSpaceMatrix[i] * vec4(vs_out.FragPos, 1.0);
    }
    gl_Position = projection * view * vec4(vs_out.FragPos, 1.0);
}
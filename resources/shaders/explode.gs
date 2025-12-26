#version 460 core
layout (triangles) in;
layout (triangle_strip, max_vertices = 3) out;

in VS_OUT {
    vec3 FragPos;
    vec3 Normal;
    vec2 TexCoords;
} gs_in[];

out VS_OUT {
    vec3 FragPos;
    vec3 Normal;
    vec2 TexCoords;
} gs_out;

const float MAGNITUDE = 2.0;

uniform float time;

vec4 explode(vec4 position, vec3 normal)
{
    vec3 direction = normal * ((sin(time) + 1.0) / 2.0) * MAGNITUDE;
    return position + vec4(direction, 0.0);
}

vec3 GetNormal()
{
    vec3 a = vec3(gl_in[0].gl_Position) - vec3(gl_in[1].gl_Position);
    vec3 b = vec3(gl_in[2].gl_Position) - vec3(gl_in[1].gl_Position);
    return normalize(cross(a, b));
}

void main()
{
    vec3 normal = GetNormal();

    gl_Position = explode(gl_in[0].gl_Position, normal);
    gs_out.TexCoords = gs_in[0].TexCoords;
    gs_out.Normal = gs_in[0].Normal;
	EmitVertex();

    gl_Position = explode(gl_in[1].gl_Position, normal);
    gs_out.TexCoords = gs_in[1].TexCoords;
    gs_out.Normal = gs_in[1].Normal;
	EmitVertex();

    gl_Position = explode(gl_in[2].gl_Position, normal);
    gs_out.TexCoords = gs_in[2].TexCoords;
    gs_out.Normal = gs_in[2].Normal;
	EmitVertex();

	EndPrimitive();
}

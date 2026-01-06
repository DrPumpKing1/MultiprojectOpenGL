#version 460 core
layout (triangles) in;
layout (line_strip, max_vertices = 2) out;

layout (std140) uniform Matrices
{
    mat4 projection;
    mat4 view;
};

in VS_OUT {
    vec3 Normal;
} gs_in[];

const float MAGNITUDE = 1.0;

void main()
{
    vec4 faceCenter = (gl_in[0].gl_Position + gl_in[1].gl_Position + gl_in[2].gl_Position) / 3.0;
    vec4 centerNormal = vec4(normalize(gs_in[0].Normal + gs_in[1].Normal + gs_in[2].Normal) / 3.0, 0.0);
    gl_Position = projection * faceCenter;
    EmitVertex();
    gl_Position = projection * (faceCenter + centerNormal * MAGNITUDE);
    EmitVertex();
    EndPrimitive();
}

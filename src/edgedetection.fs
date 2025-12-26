#version 460 core
out vec4 FragColor;

in vec2 TexCoords;

uniform sampler2D screenTexture;

const float offset = 1.0 / 800.0; // adjust accordingly

void main()
{
	vec2 offsets[9] = vec2[](
		vec2(-offset,  offset), // top-left
		vec2(0.0f,    offset), // top-center
		vec2(offset,   offset), // top-right
		vec2(-offset,  0.0f), // center-left
		vec2(0.0f,    0.0f), // center-center
		vec2(offset,   0.0f), // center-right
		vec2(-offset, -offset), // bottom-left
		vec2(0.0f,   -offset), // bottom-center
		vec2(offset,  -offset)  // bottom-right
	);

	float gx[9] = float[](
		-1, 0, 1,
		-2, 0, 2,
		-1, 0, 1
	);

	float gy[9] = float[](
		-1, -2, -1,
		0, 0, 0,
		1, 2, 1
	);

	vec3 sampleTex[9];
	vec3 horizontal, vertical;
	for(int i = 0; i < 9; i++)
	{
		sampleTex[i] = vec3(texture(screenTexture, TexCoords.st + offsets[i]));
		horizontal += gx[i] * sampleTex[i];
		vertical += gy[i] * sampleTex[i];
	}
	horizontal = horizontal * horizontal;
	vertical = vertical * vertical;

	vec3 edgeDetection = sqrt(horizontal + vertical);

	FragColor = vec4(edgeDetection, 1.0);
}
#pragma once

#include <glad/gl.h>

#include "cubemap.h"
#include "shader.h"

float skyboxVertices[] =
{
	//   Coordinates
	-1.0f, -1.0f,  1.0f,//        7--------6
	 1.0f, -1.0f,  1.0f,//       /|       /|
	 1.0f, -1.0f, -1.0f,//      4--------5 |
	-1.0f, -1.0f, -1.0f,//      | |      | |
	-1.0f,  1.0f,  1.0f,//      | 3------|-2
	 1.0f,  1.0f,  1.0f,//      |/       |/
	 1.0f,  1.0f, -1.0f,//      0--------1
	-1.0f,  1.0f, -1.0f
};

unsigned int skyboxIndices[] =
{
	// Right
	1, 2, 6,
	6, 5, 1,
	// Left
	0, 4, 7,
	7, 3, 0,
	// Top
	4, 5, 6,
	6, 7, 4,
	// Bottom
	0, 3, 2,
	2, 1, 0,
	// Back
	0, 1, 5,
	5, 4, 0,
	// Front
	3, 7, 6,
	6, 2, 3
};

class Skybox
{
	public:
		Cubemap cubemap;

		Skybox(const char *posx, const char *negx, const char *posy, const char *negy, const char *posz, const char *negz, unsigned int textureUnit)
		{
			this->cubemap = Cubemap(posx, negx, posy, negy, posz, negz, textureUnit);
			setupSkybox();
		}

		void Draw(Shader &shader)
		{
			glDepthFunc(GL_LEQUAL);
			glBindVertexArray(VAO);
			cubemap.Bind();
			cubemap.SetShaderUniform(shader, "skybox");
			glDrawElements(GL_TRIANGLES, 36, GL_UNSIGNED_INT, 0);
			cubemap.Unbind();
			glBindVertexArray(0);
			glDepthFunc(GL_LESS);
		}

	private:
		unsigned int VAO, VBO, EBO;

		void setupSkybox()
		{
			glGenVertexArrays(1, &VAO);
			glGenBuffers(1, &VBO);
			glGenBuffers(1, &EBO);
			glBindVertexArray(VAO);
			glBindBuffer(GL_ARRAY_BUFFER, VBO);
			glBufferData(GL_ARRAY_BUFFER, sizeof(skyboxVertices), &skyboxVertices, GL_STATIC_DRAW);
			glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, EBO);
			glBufferData(GL_ELEMENT_ARRAY_BUFFER, sizeof(skyboxIndices), &skyboxIndices, GL_STATIC_DRAW);
			glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 3 * sizeof(float), (void*)0);
			glEnableVertexAttribArray(0);
			glBindBuffer(GL_ARRAY_BUFFER, 0);
			glBindVertexArray(0);
			glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, 0);
		}
};

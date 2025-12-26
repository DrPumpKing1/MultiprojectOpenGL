#pragma once

#include "glad_loader.h"
#include <stb/stb_image.h>

#include "shader.h"

#include <string>
#include <vector>

class Cubemap
{
	public:
		unsigned int ID;
		unsigned int textureUnit;

        Cubemap() 
        {
            ID = 0;
			textureUnit = 0;
        }

		Cubemap(const char* posx, const char *negx, const char *posy, const char *negy, const char *posz, const char *negz, int textureUnit)
		{
			this->textureUnit = textureUnit;
			glActiveTexture(GL_TEXTURE0 + textureUnit);
			SetupCubeMap(posx, negx, posy, negy, posz, negz);
		}

        void SetupCubeMap(const char* posx, const char *negx, const char *posy, const char *negy, const char *posz, const char *negz)
        {
            std::vector<std::string> faces
            {
                std::string(posx),
                std::string(negx),
                std::string(posy),
                std::string(negy),
                std::string(posz),
                std::string(negz)
			};

			loadCubemap(faces);
        }

        void SetShaderUniform(Shader& shader, const char *uniformName)
        {
            shader.setInt(uniformName, textureUnit);
		}

        void Bind()
        {
			glActiveTexture(GL_TEXTURE0 + textureUnit);
            glBindTexture(GL_TEXTURE_CUBE_MAP, ID);
        }

        void Unbind()
        {
            glBindTexture(GL_TEXTURE_CUBE_MAP, 0);
		}

	private:
        unsigned int loadCubemap(std::vector<std::string> faces)
        {
			stbi_set_flip_vertically_on_load(false);

            glGenTextures(1, &ID);
            glBindTexture(GL_TEXTURE_CUBE_MAP, ID);

            int width, height, nrChannels;
            GLint internalFormat;
            GLenum format;
            for (unsigned int i = 0; i < faces.size(); i++)
            {
                unsigned char* data = stbi_load(faces[i].c_str(), &width, &height, &nrChannels, 0);

                if (nrChannels == 4)
                {
                    internalFormat = GL_SRGB_ALPHA;
                    format = GL_RGBA;
                }
                else if (nrChannels == 3)
                {
                    internalFormat = GL_SRGB;
                    format = GL_RGB;
                }
                else if (nrChannels == 1)
                {
                    internalFormat = GL_SRGB;
                    format = GL_RED;
                }
                else
                    throw std::invalid_argument("Automatic Texture type recognition failed");

                if (data)
                {
                    glTexImage2D(GL_TEXTURE_CUBE_MAP_POSITIVE_X + i,
                        0, internalFormat, width, height, 0, format, GL_UNSIGNED_BYTE, data
                    );
                    stbi_image_free(data);
                }
                else
                {
                    std::cout << "Cubemap tex failed to load at path: " << faces[i] << std::endl;
                    stbi_image_free(data);
                }
            }
            glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
            glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
            glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
            glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
            glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_WRAP_R, GL_CLAMP_TO_EDGE);

			stbi_set_flip_vertically_on_load(true);

            return ID;
        }
};

#pragma once

#include <glm/glm/glm.hpp>
#include "shader.h"

struct DirectionalLightData {
    glm::vec3 direction;
  
    glm::vec3 ambient;
    glm::vec3 diffuse;
    glm::vec3 specular;
};  

struct PointLightData {    
    glm::vec3 position;
    
    float constant;
    float linear;
    float quadratic;  

    glm::vec3 ambient;
    glm::vec3 diffuse;
    glm::vec3 specular;
}; 

struct SpotLightData {
    glm::vec3 position;
    glm::vec3 direction;
    float cutOff;
    float outerCutOff;
  
    float constant;
    float linear;
    float quadratic;
  
    glm::vec3 ambient;
    glm::vec3 diffuse;
    glm::vec3 specular;       
};

class DirectionalLight
{
    public:
        glm::vec3 direction;

        glm::vec3 ambient;
        glm::vec3 diffuse;
        glm::vec3 specular;

        DirectionalLight(DirectionalLightData dirLight)
        {
            direction = dirLight.direction;
            ambient = dirLight.ambient;
            diffuse = dirLight.diffuse;
            specular = dirLight.specular;
        }

        void setInShader(Shader &shader, const std::string &name)
        {
            shader.setVec3(name + ".direction", direction);
            shader.setVec3(name + ".ambient", ambient);
            shader.setVec3(name + ".diffuse", diffuse);
            shader.setVec3(name + ".specular", specular);
        }
};

class PointLight
{
    public:
        glm::vec3 position;

        float constant;
        float linear;
        float quadratic;

        glm::vec3 ambient;
        glm::vec3 diffuse;
        glm::vec3 specular;

        PointLight(PointLightData pointLight)
        {
            position = pointLight.position;
            constant = pointLight.constant;
            linear = pointLight.linear;
            quadratic = pointLight.quadratic;
            ambient = pointLight.ambient;
            diffuse = pointLight.diffuse;
            specular = pointLight.specular;
        }

        void setInShader(Shader &shader, const std::string &name)
        {
            shader.setVec3(name + ".position", position);
            shader.setFloat(name + ".constant", constant);
            shader.setFloat(name + ".linear", linear);
            shader.setFloat(name + ".quadratic", quadratic);
            shader.setVec3(name + ".ambient", ambient);
            shader.setVec3(name + ".diffuse", diffuse);
            shader.setVec3(name + ".specular", specular);
        }
};

class SpotLight
{
    public:
        glm::vec3 position;
        glm::vec3 direction;
        float cutOff;
        float outerCutOff;

        float constant;
        float linear;
        float quadratic;

        glm::vec3 ambient;
        glm::vec3 diffuse;
        glm::vec3 specular;

        SpotLight(SpotLightData spotLight)
        {
            position = spotLight.position;
            direction = spotLight.direction;
            cutOff = spotLight.cutOff;
            outerCutOff = spotLight.outerCutOff;
            constant = spotLight.constant;
            linear = spotLight.linear;
            quadratic = spotLight.quadratic;
            ambient = spotLight.ambient;
            diffuse = spotLight.diffuse;
            specular = spotLight.specular;
        }

        void setInShader(Shader &shader, const std::string &name)
        {
            shader.setVec3(name + ".position", position);
            shader.setVec3(name + ".direction", direction);
            shader.setFloat(name + ".cutOff", cutOff);
            shader.setFloat(name + ".outerCutOff", outerCutOff);
            shader.setFloat(name + ".constant", constant);
            shader.setFloat(name + ".linear", linear);
            shader.setFloat(name + ".quadratic", quadratic);
            shader.setVec3(name + ".ambient", ambient);
            shader.setVec3(name + ".diffuse", diffuse);
            shader.setVec3(name + ".specular", specular);
        }
};
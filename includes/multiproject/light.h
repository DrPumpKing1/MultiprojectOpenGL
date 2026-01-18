#pragma once

#include <glm/glm.hpp>
#include "shader.h"

struct DirectionalLightData {
    bool hasShadow;

    glm::vec3 direction;

    glm::vec3 ambient;
    glm::vec3 diffuse;
    glm::vec3 specular;

    unsigned int shadowMap;
};

struct PointLightData {
    bool hasShadow;
    unsigned int shadowMap;

    float constant;
    float linear;
    float quadratic;

    glm::vec3 position;

    glm::vec3 ambient;
    glm::vec3 diffuse;
    glm::vec3 specular;
};

struct SpotLightData {
    bool hasShadow;
    unsigned int shadowMap;

    float cutOff;
    float outerCutOff;

    float constant;
    float linear;
    float quadratic;

    glm::vec3 position;
    glm::vec3 direction;

    glm::vec3 ambient;
    glm::vec3 diffuse;
    glm::vec3 specular;
};

class Light
{
    public:
        glm::vec3 ambient;
        glm::vec3 diffuse;
        glm::vec3 specular;

        Light(glm::vec3 ambient, glm::vec3 diffuse, glm::vec3 specular) {
            this->ambient = ambient;
            this->diffuse = diffuse;
            this->specular = specular;
        }

        virtual void setInShader(Shader &shader, const std::string &name){
            shader.setVec3(name + ".ambient", ambient);
            shader.setVec3(name + ".diffuse", diffuse);
            shader.setVec3(name + ".specular", specular);
        }

        void setInShader(Shader &shader, const std::string &name, int index) {
            std::string fullName = name + "[" + std::to_string(index) + "]";
            setInShader(shader, fullName);
        }
};

class LightShadow : public Light {
    public:
        bool hasShadow;
        unsigned int shadowMap;

        LightShadow(glm::vec3 ambient, glm::vec3 diffuse, glm::vec3 specular, bool hasShadow, unsigned int shadowMap) : Light(ambient, diffuse, specular) {
            this->hasShadow = hasShadow;
            this->shadowMap = shadowMap;

            if(hasShadow)
                initShadowMap();
        }

        using Light::setInShader;
        void setInShader(Shader &shader, const std::string &name) override {
            Light::setInShader(shader, name);
            shader.setBool(name + ".hasShadow", hasShadow);
            shader.setInt(name + ".shadowMap", shadowMap);
        }

        void bindShadowMap() {
            glActiveTexture(GL_TEXTURE0 + shadowMap);
            glBindTexture(GL_TEXTURE_2D, depthMap);
        }

        template<typename T>
        void renderDepthMap(T renderSceneFunc) {
            if(!hasShadow)
                return;

            glViewport(0, 0, SHADOW_WIDTH, SHADOW_HEIGHT);
            glBindFramebuffer(GL_FRAMEBUFFER, this->depthMapFBO);
            glClear(GL_DEPTH_BUFFER_BIT);
            glCullFace(GL_FRONT);
            renderSceneFunc();
            glCullFace(GL_BACK);
            glBindFramebuffer(GL_FRAMEBUFFER, 0);
        }

        virtual void getLightSpaceMatrix(glm::mat4 &lightSpaceMatrix) = 0;
    protected:
        unsigned int depthMapFBO;
        unsigned int depthMap;
        const unsigned int SHADOW_WIDTH = 1024;
        const unsigned int SHADOW_HEIGHT = 1024;

        virtual void initShadowMap() = 0;
};

class LightAttenuation : public LightShadow {
    public:
        float constant;
        float linear;
        float quadratic;

        LightAttenuation(glm::vec3 ambient, glm::vec3 diffuse, glm::vec3 specular, bool hasShadow, unsigned int shadowMap, float constant, float linear, float quadratic) : LightShadow(ambient, diffuse, specular, hasShadow, shadowMap) {
            this->constant = constant;
            this->linear = linear;
            this->quadratic = quadratic;
        }

        using LightShadow::setInShader;
        void setInShader(Shader &shader, const std::string &name) override {
            LightShadow::setInShader(shader, name);
            shader.setFloat(name + ".constant", constant);
            shader.setFloat(name + ".linear", linear);
            shader.setFloat(name + ".quadratic", quadratic);
        }
};

class DirectionalLight : public LightShadow {
    public:
        glm::vec3 direction;

        DirectionalLight(glm::vec3 ambient, glm::vec3 diffuse, glm::vec3 specular, bool hasShadow, unsigned int shadowMap, glm::vec3 direction)
            : LightShadow(ambient, diffuse, specular, hasShadow, shadowMap) {
            this->direction = direction;
        }

        using LightShadow::setInShader;
        void setInShader(Shader &shader, const std::string &name) override {
            LightShadow::setInShader(shader, name);
            shader.setVec3(name + ".direction", direction);
        }

        void getLightSpaceMatrix(glm::mat4 &lightSpaceMatrix) override {
            glm::mat4 lightProjection, lightView;
            float orthoSize = 10.0f;
            lightProjection = glm::ortho(-orthoSize, orthoSize, -orthoSize, orthoSize, nearPlane, farPlane);
            lightView = glm::lookAt(-direction * 5.0f, glm::vec3(0.0f), glm::vec3(0.0f, 1.0f, 0.0f));
            lightSpaceMatrix = lightProjection * lightView;
        }
    
    protected:
        const float nearPlane = 1.0f;
        const float farPlane = 7.5f;

        void initShadowMap() override {
            glGenFramebuffers(1, &depthMapFBO);

            glGenTextures(1, &depthMap);
            glBindTexture(GL_TEXTURE_2D, depthMap);
            glTexImage2D(GL_TEXTURE_2D, 0, GL_DEPTH_COMPONENT, SHADOW_WIDTH, SHADOW_HEIGHT, 0, GL_DEPTH_COMPONENT, GL_FLOAT, NULL);
            glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
            glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
            glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_BORDER);
            glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_BORDER);
            float borderColor[] = { 1.0f, 1.0f, 1.0f, 1.0f };
            glTexParameterfv(GL_TEXTURE_2D, GL_TEXTURE_BORDER_COLOR, borderColor);

            glBindFramebuffer(GL_FRAMEBUFFER, depthMapFBO);
            glFramebufferTexture2D(GL_FRAMEBUFFER, GL_DEPTH_ATTACHMENT, GL_TEXTURE_2D, depthMap, 0);
            glDrawBuffer(GL_NONE);
            glReadBuffer(GL_NONE);
            glBindFramebuffer(GL_FRAMEBUFFER, 0);
        }
};

class PointLight : LightAttenuation {
    public:
        glm::vec3 position;

        PointLight(glm::vec3 ambient, glm::vec3 diffuse, glm::vec3 specular, bool hasShadow, unsigned int shadowMap, float constant, float linear, float quadratic, glm::vec3 position)
            : LightAttenuation(ambient, diffuse, specular, hasShadow, shadowMap, constant, linear, quadratic) {
            this->position = position;
        }

        using LightAttenuation::setInShader;
        void setInShader(Shader &shader, const std::string &name) override {
            LightAttenuation::setInShader(shader, name);
            shader.setVec3(name + ".position", position);
        }
};

class SpotLight : LightAttenuation {
    public:
        glm::vec3 position;
        glm::vec3 direction;

        float cutOff;
        float outerCutOff;

        SpotLight(glm::vec3 ambient, glm::vec3 diffuse, glm::vec3 specular, bool hasShadow, unsigned int shadowMap, float constant, float linear, float quadratic, glm::vec3 position, glm::vec3 direction, float cutOff, float outerCutOff)
            : LightAttenuation(ambient, diffuse, specular, hasShadow, shadowMap, constant, linear, quadratic) {
            this->position = position;
            this->direction = direction;
            this->cutOff = cutOff;
            this->outerCutOff = outerCutOff;
        }

        using LightAttenuation::setInShader;
        void setInShader(Shader &shader, const std::string &name) override {
            LightAttenuation::setInShader(shader, name);
            shader.setVec3(name + ".position", position);
            shader.setVec3(name + ".direction", direction);
            shader.setFloat(name + ".cutOff", cutOff);
            shader.setFloat(name + ".outerCutOff", outerCutOff);
        }
};
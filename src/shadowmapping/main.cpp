#include <glad/gl.h>
#include <GLFW/glfw3.h>

#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtc/type_ptr.hpp>

#include <multiproject/camera.h>
#include <multiproject/shader.h>
#include <multiproject/model.h>
#include <multiproject/light.h>
#include <multiproject/postprocesseffect.h>
#include <multiproject/skybox.h>
#include <multiproject/filesystem.h>

#include <iostream>

void framebuffer_size_callback(GLFWwindow* window, int width, int height);
void mouse_callback(GLFWwindow* window, double xpos, double ypos);
void scroll_callback(GLFWwindow* window, double xoffset, double yoffset);
void processInput(GLFWwindow* window);

//Frame Settings
const unsigned int SCR_WIDTH = 800;
const unsigned int SCR_HEIGHT = 800;
unsigned int CURR_WIDTH = SCR_WIDTH;
unsigned int CURR_HEIGHT = SCR_HEIGHT;

//Camera
Camera camera(glm::vec3(0.0f, 0.0f, 5.0f));
const float farPlane = 1000.0f;
const float nearPlane = 0.1f;

//Time Management
float deltaTime = 0.0f;
float lastFrame = 0.0f;

//Mouse Input Management
float lastX = SCR_WIDTH / 2.0f;
float lastY = SCR_HEIGHT / 2.0f;
bool firstMouse = true;

//Post Processing Framebuffer
PostProcessEffect *postProcessEffect;

//Quad vertices for rendering depth map
float quadVerticesStrip[] = {
    // positions        // texture Coords
    -1.0f,  1.0f, 0.0f, 0.0f, 1.0f,
    -1.0f, -1.0f, 0.0f, 0.0f, 0.0f,
     1.0f,  1.0f, 0.0f, 1.0f, 1.0f,
     1.0f, -1.0f, 0.0f, 1.0f, 0.0f,
};

int main(void)
{
    GLFWwindow* window;

    if (!glfwInit())
        return -1;

    glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 4);
    glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 6);
    glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE);

#ifdef __APPLE__
    glfwWindowHint(GLFW_OPENGL_FORWARD_COMPAT, GL_TRUE);
#endif

    glfwWindowHint(GLFW_SAMPLES, 4);

    window = glfwCreateWindow(SCR_WIDTH, SCR_HEIGHT, "Hello World", NULL, NULL);
    if (window == NULL)
    {
        std::cout << "Failed to create GLFW window" << std::endl;
        glfwTerminate();
        return -1;
    }

    glfwMakeContextCurrent(window);
    glfwSetFramebufferSizeCallback(window, framebuffer_size_callback);
    glfwSetCursorPosCallback(window, mouse_callback);
    glfwSetScrollCallback(window, scroll_callback);

    glfwSetInputMode(window, GLFW_CURSOR, GLFW_CURSOR_DISABLED);

    if (!gladLoadGL((GLADloadfunc)glfwGetProcAddress))
    {
        std::cout << "Failed to initialize GLAD" << std::endl;
        return -1;
    }

    Shader shadowShader("depthmap.vs", "depthmap.fs");
    Shader shadowCubeShader("depthcubemap.vs", "depthcubemap.fs", "depthcubemap.gs");
    Shader litShader("defaultNoUboShadow.vs", "defaultShadow.fs");
    Shader postprocessShader("postprocess.vs", "postprocess.fs");

    Model defaultModel(FileSystem::getPath("resources/objects/backpack/backpack.obj").c_str());

	postProcessEffect = new PostProcessEffect(SCR_WIDTH, SCR_HEIGHT);

    //Light configuration
    const bool blinn = true;
    const unsigned int numDirLights = 1;
    const unsigned int numPointLights = 1;
    const unsigned int numSpotLights = 1;
    const unsigned int numShadows = 2;
    DirectionalLight* dirLights[numDirLights];
    dirLights[0] = new DirectionalLight(
        glm::vec3(0.05f, 0.05f, 0.05f), //ambient
        glm::vec3(0.25f, 0.25f, 0.25f), //diffuse
        glm::vec3(1.0f, 1.0f, 1.0f), //specular
        true, 2, 0,             //hasShadow, shadowMap, shadowIndex
        glm::vec3(-2.0f, -4.0f, -1.0f) //direction
    );
    PointLight* pointLights[numPointLights];
    pointLights[0] = new PointLight(
        glm::vec3(0.05f, 0.05f, 0.05f), //ambient
        glm::vec3(0.25f, 0.25f, 0.25f), //diffuse
        glm::vec3(1.0f, 1.0f, 1.0f), //specular
        true, 3, 0,             //hasShadow, shadowMap, shadowIndex
        1.0f, 0.09f, 0.032f,   //constant, linear, quadratic
        glm::vec3(2.0f, 2.0f, 2.0f) //position
    );
    SpotLight* spotLights[numSpotLights];
    spotLights[0] = new SpotLight(
        glm::vec3(0.0f, 0.0f, 0.0f), //ambient
        glm::vec3(0.35f, 0.35f, 0.35f), //diffuse
        glm::vec3(1.0f, 1.0f, 1.0f), //specular
        false, 4, 1,            //hasShadow, shadowMap, shadowIndex
        1.0f, 0.09f, 0.032f,   //constant, linear, quadratic
        camera.Position,        //position
        camera.Front,           //direction
        glm::cos(glm::radians(12.5f)), //cutOff
        glm::cos(glm::radians(15.0f))   //outerCutOff
    );
    litShader.Activate();
    litShader.setInt("numDirLights", numDirLights);
    litShader.setInt("numPointLights", numPointLights);
    litShader.setInt("numSpotLights", numSpotLights);
    litShader.setBool("blinn", blinn);
    litShader.setFloat("material.shininess", 32.0f);

	// configure global opengl state
    glEnable(GL_DEPTH_TEST);
    glEnable(GL_CULL_FACE);
    glCullFace(GL_BACK);
    glFrontFace(GL_CCW);
    glEnable(GL_FRAMEBUFFER_SRGB);

    while (!glfwWindowShouldClose(window))
    {
        float currentFrame = static_cast<float>(glfwGetTime());
        deltaTime = currentFrame - lastFrame;
        lastFrame = currentFrame;
		std::string fpsCount = std::to_string(1.0f / deltaTime);
		std::string title = "FPS: " + fpsCount;
		glfwSetWindowTitle(window, title.c_str());

        processInput(window);
        glm::mat4 projection = glm::perspective(glm::radians(camera.Zoom), (float)CURR_WIDTH / (float)CURR_HEIGHT, nearPlane, farPlane);
        glm::mat4 view = camera.GetViewMatrix();

        glm::mat4 model = glm::mat4(1.0f);
        model = glm::translate(model, glm::vec3(0.0f, 0.0f, 1.5f));
        model = glm::scale(model, glm::vec3(1.0f));

        //render shadows
        for(const auto& dirLight : dirLights) {
            glm::mat4 lightSpaceMatrix;
            dirLight->getLightSpaceMatrix(lightSpaceMatrix);
            shadowShader.Activate();
            shadowShader.setMat4("lightSpaceMatrix", lightSpaceMatrix);
            shadowShader.setMat4("model", model);
            dirLight->renderDepthMap([&]() {
                defaultModel.Draw(shadowShader);
            });
        }
        for(const auto& pointLight : pointLights) {
            std::vector<glm::mat4> shadowTransform = pointLight->getShadowTransformations();
            shadowCubeShader.Activate();
            for(unsigned int i = 0; i < 6; i++) {
                shadowCubeShader.setMat4("shadowTransforms[" + std::to_string(i) + "]", shadowTransform[i]);
            }
            shadowCubeShader.setFloat("farPlane", pointLight->farPlane);
            shadowCubeShader.setVec3("lightPos", pointLight->position);
            pointLight->renderDepthMap([&]() {
                defaultModel.Draw(shadowShader);
            });
        }
        for(const auto& spotLight : spotLights) {
            glm::mat4 lightSpaceMatrix;
            spotLight->getLightSpaceMatrix(lightSpaceMatrix);
            shadowShader.Activate();
            shadowShader.setMat4("lightSpaceMatrix", lightSpaceMatrix);
            shadowShader.setMat4("model", model);
            spotLight->renderDepthMap([&]() {
                defaultModel.Draw(shadowShader);
            });
        }

        //render scene
        glViewport(0, 0, CURR_WIDTH, CURR_HEIGHT);
		postProcessEffect->Bind();
        glClearColor(0.2f, 0.3f, 0.3f, 1.0f);
        glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

        litShader.Activate();
        litShader.setVec3("viewPos", camera.Position);
        litShader.setMat4("projection", projection);
        litShader.setMat4("view", view);
        litShader.setMat4("model", model);
        glm::mat4 lightSpaceMatrix;
        litShader.setInt("numShadows", numShadows);
        for(unsigned int i = 0; i < numDirLights; i++) {
            dirLights[i]->bindShadowMap();
            dirLights[i]->setInShader(litShader, "dirLights", i);
            dirLights[i]->getLightSpaceMatrix(lightSpaceMatrix);
            litShader.setMat4("lightSpaceMatrix[" + std::to_string(dirLights[i]->shadowIndex)  + "]", lightSpaceMatrix);
        }
        for(unsigned int i = 0; i < numPointLights; i++) {
            pointLights[i]->bindShadowMap();
            pointLights[i]->setInShader(litShader, "pointLights", i);
        }
        for(unsigned int i = 0; i < numSpotLights; i++) {
            spotLights[i]->bindShadowMap();
            spotLights[i]->setInShader(litShader, "spotLights", i);
            spotLights[i]->getLightSpaceMatrix(lightSpaceMatrix);
            litShader.setMat4("lightSpaceMatrix[" + std::to_string(spotLights[i]->shadowIndex)  + "]", lightSpaceMatrix);
        }
        defaultModel.Draw(litShader);

        postProcessEffect->Blit();
		postProcessEffect->Unbind();

        //render framebuffer
        glClearColor(1.0f, 1.0f, 1.0f, 1.0f);
        glClear(GL_COLOR_BUFFER_BIT);
        glDisable(GL_DEPTH_TEST);
        postProcessEffect->Render(postprocessShader);
        glEnable(GL_DEPTH_TEST);

        glfwSwapBuffers(window);
        glfwPollEvents();
    }

    glfwTerminate();
    return 0;
}

void processInput(GLFWwindow *window)
{
    if(glfwGetKey(window, GLFW_KEY_ESCAPE) == GLFW_PRESS)
        glfwSetWindowShouldClose(window, true);

    if(glfwGetKey(window, GLFW_KEY_LEFT_SHIFT) == GLFW_PRESS)
        camera.ProccesKeyboardSpeed(true);
    else
        camera.ProccesKeyboardSpeed(false);

    if (glfwGetKey(window, GLFW_KEY_W) == GLFW_PRESS)
        camera.ProcessKeyboardMovement(FORWARD, deltaTime);
    if (glfwGetKey(window, GLFW_KEY_S) == GLFW_PRESS)
        camera.ProcessKeyboardMovement(BACKWARD, deltaTime);
    if (glfwGetKey(window, GLFW_KEY_A) == GLFW_PRESS)
        camera.ProcessKeyboardMovement(LEFT, deltaTime);
    if (glfwGetKey(window, GLFW_KEY_D) == GLFW_PRESS)
        camera.ProcessKeyboardMovement(RIGHT, deltaTime);
    if (glfwGetKey(window, GLFW_KEY_Q) == GLFW_PRESS)
        camera.ProcessKeyboardMovement(UP, deltaTime);
    if (glfwGetKey(window, GLFW_KEY_E) == GLFW_PRESS)
        camera.ProcessKeyboardMovement(DOWN, deltaTime);
}

// glfw: whenever the window size changed (by OS or user resize) this callback function executes
// ---------------------------------------------------------------------------------------------
void framebuffer_size_callback(GLFWwindow* window, int width, int height)
{
    // make sure the viewport matches the new window dimensions; note that width and
    // height will be significantly larger than specified on retina displays.
    CURR_WIDTH = width;
    CURR_HEIGHT = height;
    glViewport(0, 0, width, height);
    postProcessEffect->Resize(width, height);
}

// glfw: whenever the mouse moves, this callback is called
// -------------------------------------------------------
void mouse_callback(GLFWwindow* window, double xposIn, double yposIn)
{
    float xpos = static_cast<float>(xposIn);
    float ypos = static_cast<float>(yposIn);

    if (firstMouse)
    {
        lastX = xpos;
        lastY = ypos;
        firstMouse = false;
    }

    float xoffset = xpos - lastX;
    float yoffset = lastY - ypos; // reversed since y-coordinates go from bottom to top

    lastX = xpos;
    lastY = ypos;

    camera.ProcessMouseMovement(xoffset, yoffset);
}

// glfw: whenever the mouse scroll wheel scrolls, this callback is called
// ----------------------------------------------------------------------
void scroll_callback(GLFWwindow* window, double xoffset, double yoffset)
{
    camera.ProcessMouseScroll(static_cast<float>(yoffset));
}
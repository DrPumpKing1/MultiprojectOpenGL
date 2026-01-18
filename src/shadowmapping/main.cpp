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

//Shadow Settings
const unsigned int SHADOW_WIDTH = 1024;
const unsigned int SHADOW_HEIGHT = 1024;

//Camera
Camera camera(glm::vec3(0.0f, 0.0f, 150.0f));
const float farPlane = 7.5f;
const float nearPlane = 1.0f;

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
    Shader litShader("defaultNoUboShadow.vs", "defaultShadow.fs");
    Shader postprocessShader("postprocess.vs", "postprocess.fs");

    Model defaultModel(FileSystem::getPath("resources/objects/backpack/backpack.obj").c_str());

	postProcessEffect = new PostProcessEffect(SCR_WIDTH, SCR_HEIGHT);

    //Light configuration
    const bool blinn = true;
    const unsigned int numDirLights = 1;
    const unsigned int numPointLights = 0;
    const unsigned int numSpotLights = 0;
    DirectionalLight* dirLights[numDirLights];
    dirLights[0] = new DirectionalLight(
        glm::vec3(0.2f, 0.2f, 0.2f), //ambient
        glm::vec3(0.5f, 0.5f, 0.5f), //diffuse
        glm::vec3(1.0f, 1.0f, 1.0f), //specular
        true,
        0,
        glm::vec3(-2.0f, -4.0f, -1.0f) //direction
    );
    PointLight* pointLights[1];
    SpotLight* spotLights[1];
    litShader.Activate();
    litShader.setInt("numDirLights", numDirLights);
    litShader.setInt("numPointLights", numPointLights);
    litShader.setInt("numSpotLights", numSpotLights);
    litShader.setBool("blinn", blinn);

	// configure global opengl state
    glEnable(GL_DEPTH_TEST);
    glEnable(GL_CULL_FACE);
    glCullFace(GL_BACK);
    glFrontFace(GL_CCW);

    while (!glfwWindowShouldClose(window))
    {
        float currentFrame = static_cast<float>(glfwGetTime());
        deltaTime = currentFrame - lastFrame;
        lastFrame = currentFrame;
		std::string fpsCount = std::to_string(1.0f / deltaTime);
		std::string title = "FPS: " + fpsCount;
		glfwSetWindowTitle(window, title.c_str());

        processInput(window);

        glClearColor(0.2f, 0.3f, 0.3f, 1.0f);
        glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

        //render shadows
        for(const auto& dirLight : dirLights) {
            glm::mat4 lightSpaceMatrix;
            dirLight->getLightSpaceMatrix(lightSpaceMatrix);
            shadowShader.Activate();
            shadowShader.setMat4("lightSpaceMatrix", lightSpaceMatrix);
            dirLight->renderDepthMap([&]() {
                glm::mat4 model = glm::mat4(1.0f);
                model = glm::translate(model, glm::vec3(0.0f, 1.5f, 0.0f));
                model = glm::scale(model, glm::vec3(1.5f));
                shadowShader.setMat4("model", model);
                defaultModel.Draw(shadowShader);
            });
        }

        //render depth map to quad
        glViewport(0, 0, CURR_WIDTH, CURR_HEIGHT);
		postProcessEffect->Bind();
        glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

        litShader.Activate();
        for(unsigned int i = 0; i < numDirLights; i++) {
            dirLights[i]->bindShadowMap();
            dirLights[i]->setInShader(litShader, "dirLight", i);
        }
        glm::mat4 model = glm::mat4(1.0f);
        model = glm::translate(model, glm::vec3(0.0f, 1.5f, 0.0f));
        model = glm::scale(model, glm::vec3(1.5f));
        litShader.setMat4("model", model);
        defaultModel.Draw(litShader);
		postProcessEffect->Unbind();

        //render framebuffer
        glClear(GL_COLOR_BUFFER_BIT);
		postProcessEffect->Render(postprocessShader);

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
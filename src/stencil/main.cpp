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

//settings
const unsigned int SCR_WIDTH = 800;
const unsigned int SCR_HEIGHT = 800;
unsigned int CURR_WIDTH = SCR_WIDTH;
unsigned int CURR_HEIGHT = SCR_HEIGHT;

// camera
const float farPlane = 1000.0f;
const float nearPlane = 0.1f;
Camera camera(glm::vec3(0.0f, 0.0f, 0.0f));
float lastX = SCR_WIDTH / 2.0f;
float lastY = SCR_HEIGHT / 2.0f;
bool firstMouse = true;

// timing
float deltaTime = 0.0f;
float lastFrame = 0.0f;

const float border = .025f;

PostProcessEffect *postProcessEffect;

int main(void)
{
    GLFWwindow* window;

    /* Initialize the library */
    if (!glfwInit())
        return -1;

    glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 4);
    glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 6);
    glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE);

#ifdef __APPLE__
    glfwWindowHint(GLFW_OPENGL_FORWARD_COMPAT, GL_TRUE);
#endif

    //glfw Window creation
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

    // glad: load all OpenGL function pointers
    // ---------------------------------------
    if (!gladLoadGL((GLADloadfunc)glfwGetProcAddress))
    {
        std::cout << "Failed to initialize GLAD" << std::endl;
        return -1;
    }

    //Building and compiling our shader program
    Shader modelShader("defaultNoUbo.vs", "default.fs");
    Shader shaderSingleColor("defaultNoUbo.vs", "singleColor.fs");
    Shader postprocessShader("postprocess.vs", "postprocess.fs");
	Shader skyboxShader("skybox.vs", "skybox.fs");

    //Loading models
    Model defaultModel(FileSystem::getPath("resources/objects/backpack/backpack.obj").c_str());

	//Loading skybox
    Skybox skybox(
        FileSystem::getPath("resources/cubemaps/Storforsen3/posx.jpg").c_str(),
        FileSystem::getPath("resources/cubemaps/Storforsen3/negx.jpg").c_str(),
        FileSystem::getPath("resources/cubemaps/Storforsen3/posy.jpg").c_str(),
        FileSystem::getPath("resources/cubemaps/Storforsen3/negy.jpg").c_str(),
        FileSystem::getPath("resources/cubemaps/Storforsen3/posz.jpg").c_str(),
        FileSystem::getPath("resources/cubemaps/Storforsen3/negz.jpg").c_str(),
        4
    );

    //Setting Lighting
    int numDirLights = 1;
    DirectionalLightData dirLightData{
        glm::vec3(-0.2f, -1.0f, -0.3f), //direction

        glm::vec3(0.5f, 0.5f, 0.5f), //ambient
        glm::vec3(0.4f, 0.4f, 0.4f),    //diffuse
        glm::vec3(0.5f, 0.5f, 0.5f)     //specular
    };
    DirectionalLight dirLight(dirLightData);

    int numPointLights = 0;
    PointLightData pointLightData{
        glm::vec3(-2.0f, 1.0f, 0.0f), //position

        1.0f,   //constant
        0.09f,   //linear
        0.032f,   //quadratic

        glm::vec3(0.0f, 0.0f, 0.0f), //ambient
        glm::vec3(0.8f, 0.8f, 0.8f),    //diffuse
        glm::vec3(1.0f, 1.0f, 1.0f)     //specular
    };
    int numSpotLights = 0;
    PointLight pointLight(pointLightData);

    SpotLightData spotLightData{
        camera.Position,        //position
        camera.Front,           //direction
        glm::cos(glm::radians(12.5f)), //cutOff
        glm::cos(glm::radians(15.0f)), //outerCutOff

        1.0f,   //constant
        0.09f,  //linear
        0.032f, //quadratic

        glm::vec3(0.0f, 0.0f, 0.0f), //ambient
        glm::vec3(0.8f, 0.8f, 0.8f), //diffuse
        glm::vec3(1.0f, 1.0f, 1.0f)  //specular
    };
    SpotLight spotLight(spotLightData);

	postProcessEffect = new PostProcessEffect(SCR_WIDTH, SCR_HEIGHT);

	// configure global opengl state
    glEnable(GL_DEPTH_TEST);

    glEnable(GL_STENCIL_TEST);
    glStencilOp(GL_KEEP, GL_KEEP, GL_REPLACE);

    glEnable(GL_CULL_FACE);
    glCullFace(GL_BACK);
    glFrontFace(GL_CCW);

    // render loop
    // -----------
    while (!glfwWindowShouldClose(window))
    {
        // per-frame time logic
        // --------------------
        float currentFrame = static_cast<float>(glfwGetTime());
        deltaTime = currentFrame - lastFrame;
        lastFrame = currentFrame;
		std::string fpsCount = std::to_string(1.0f / deltaTime);
		std::string title = "FPS: " + fpsCount;
		glfwSetWindowTitle(window, title.c_str());

        //input
        //-----
        processInput(window);

        //render
        //------
		postProcessEffect->Bind();
		glEnable(GL_DEPTH_TEST); // enable depth testing (is disabled for rendering screen-space quad)
        glEnable(GL_STENCIL_TEST);
		glEnable(GL_CULL_FACE);

        glClearColor(0.2f, 0.3f, 0.3f, 1.0f);
        glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT | GL_STENCIL_BUFFER_BIT);

        glStencilFunc(GL_ALWAYS, 1, 0xFF);
        glStencilMask(0xFF);

        glm::mat4 view = camera.GetViewMatrix();
	    glm::mat4 projection = glm::perspective(glm::radians(camera.Zoom), (float)CURR_WIDTH / (float)CURR_HEIGHT, nearPlane, farPlane);

        modelShader.Activate();
        modelShader.setMat4("projection", projection);
        modelShader.setMat4("view", view);

        //setting lighting uniforms
        modelShader.setVec3("viewPos", camera.Position);
        modelShader.setFloat("material.shininess", 16.0f);
        modelShader.setBool("blinn", true);
        modelShader.setInt("numDirLights", numDirLights);
        modelShader.setInt("numPointLights", numPointLights);
        modelShader.setInt("numSpotLights", numSpotLights);
        dirLight.setInShader(modelShader, "dirLights[0]");
        pointLight.setInShader(modelShader, "pointLights[0]");
        spotLight.setInShader(modelShader, "spotLights[0]");

        //render the loaded model
        glm::mat4 model = glm::mat4(1.0f);
        model = glm::translate(model, glm::vec3(0.0f, 0.0f, -10.0f));
        model = glm::scale(model, glm::vec3(1.0f, 1.0f, 1.0f));
        modelShader.setMat4("model", model);
        defaultModel.Draw(modelShader);

        glStencilFunc(GL_NOTEQUAL, 1, 0xFF);
        glStencilMask(0x00);
        glDepthFunc(GL_ALWAYS);

        shaderSingleColor.Activate();
        shaderSingleColor.setMat4("projection", projection);
        shaderSingleColor.setMat4("view", view);
        model = glm::mat4(1.0f);
        model = glm::translate(model, glm::vec3(0.0f, 0.0f, -10.0f));
        model = glm::scale(model, glm::vec3(1.0, 1.0f, 1.0f) * (1.0f + border));
        shaderSingleColor.setMat4("model", model);
        defaultModel.Draw(shaderSingleColor);

        //render the skybox
		glDisable(GL_CULL_FACE);

        view = glm::mat4(glm::mat3(camera.GetViewMatrix()));
		skyboxShader.Activate();
		skyboxShader.setMat4("view", view);
		skyboxShader.setMat4("projection", projection);
		skybox.Draw(skyboxShader);

		postProcessEffect->Unbind();
        glDisable(GL_DEPTH_TEST);
        glDisable(GL_STENCIL_TEST);

		glClearColor(1.0f, 1.0f, 1.0f, 1.0f);
        glClear(GL_COLOR_BUFFER_BIT);

		postProcessEffect->Render(postprocessShader);

        glStencilMask(0xFF);

        // glfw: swap buffers and poll IO events (keys pressed/released, mouse moved etc.)
        // -------------------------------------------------------------------------------
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
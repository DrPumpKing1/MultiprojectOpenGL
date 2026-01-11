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
Camera camera(glm::vec3(0.0f, 0.0f, 150.0f));
float lastX = SCR_WIDTH / 2.0f;
float lastY = SCR_HEIGHT / 2.0f;
bool firstMouse = true;

// timing
float deltaTime = 0.0f;
float lastFrame = 0.0f;

float transparentVertices[] = {
    // positions         // texture Coords (swapped y coordinates because texture is flipped upside down)
    0.0f,  0.5f,  0.0f,  0.0f,  0.0f,
    0.0f, -0.5f,  0.0f,  0.0f,  1.0f,
    1.0f, -0.5f,  0.0f,  1.0f,  1.0f,

    0.0f,  0.5f,  0.0f,  0.0f,  0.0f,
    1.0f, -0.5f,  0.0f,  1.0f,  1.0f,
    1.0f,  0.5f,  0.0f,  1.0f,  0.0f
};

std::vector<glm::vec3> windows
{
    glm::vec3(-1.5f, 0.0f, -5.48f),
    glm::vec3(1.5f, 0.0f, -5.51f),
    glm::vec3(0.0f, 0.0f, -5.7f),
    glm::vec3(-0.3f, 0.0f, -7.3f),
    glm::vec3(0.5f, 0.0f, -6.6f)
};

unsigned int instances = 100000;
std::vector<glm::mat4> instancesModelMatrices;
float radius = 150.0;
float offset = 25.0f;

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
    Shader modelShader("default.vs", "default.fs");
    Shader postprocessShader("postprocess.vs", "edgedetection.fs");
	Shader transparentShader("transparent.vs", "transparent.fs");
	Shader skyboxShader("skybox.vs", "skybox.fs");
    Shader normalVisualizationShader("normalvisualization.vs", "normalvisualization.fs", "normalvisualization.gs");
	Shader asteroidShader("instancing.vs", "default.fs");

    //Loading models
    Model defaultModel(FileSystem::getPath("resources/objects/planet/planet.obj").c_str());

	//Loading textures
	Texture transparentTexture(FileSystem::getPath("resources/textures/window.png").c_str(), "diffuse", 0, true);

	//Loading skybox
    /*
    Skybox skybox(
        FileSystem::getPath("resources/cubemaps/Storforsen3/posx.jpg").c_str(),
        FileSystem::getPath("resources/cubemaps/Storforsen3/negx.jpg").c_str(),
        FileSystem::getPath("resources/cubemaps/Storforsen3/posy.jpg").c_str(),
        FileSystem::getPath("resources/cubemaps/Storforsen3/negy.jpg").c_str(),
        FileSystem::getPath("resources/cubemaps/Storforsen3/posz.jpg").c_str(),
        FileSystem::getPath("resources/cubemaps/Storforsen3/negz.jpg").c_str(),
        4
    );
    */

    Skybox skybox(
        FileSystem::getPath("resources/cubemaps/Galaxy/right.png").c_str(),
        FileSystem::getPath("resources/cubemaps/Galaxy/left.png").c_str(),
        FileSystem::getPath("resources/cubemaps/Galaxy/top.png").c_str(),
        FileSystem::getPath("resources/cubemaps/Galaxy/bottom.png").c_str(),
        FileSystem::getPath("resources/cubemaps/Galaxy/front.png").c_str(),
        FileSystem::getPath("resources/cubemaps/Galaxy/back.png").c_str(),
        4
    );

    srand(static_cast<unsigned int>(glfwGetTime())); // initialize random seed
    for (unsigned int i = 0; i < instances; i++)
    {
        glm::mat4 model = glm::mat4(1.0f);
        // 1. translation: displace along circle with 'radius' in range [-offset, offset]
        float angle = (float)i / (float)instances * 360.0f;
        float displacement = (rand() % (int)(2 * offset * 100)) / 100.0f - offset;
        float x = sin(angle) * radius + displacement;
        displacement = (rand() % (int)(2 * offset * 100)) / 100.0f - offset;
        float y = displacement * 0.4f; // keep height of asteroid field smaller compared to width of x and z
        displacement = (rand() % (int)(2 * offset * 100)) / 100.0f - offset;
        float z = cos(angle) * radius + displacement;
        model = glm::translate(model, glm::vec3(x, y, z));

        // 2. scale: Scale between 0.05 and 0.25f
        float scale = static_cast<float>((rand() % 20) / 100.0 + 0.05);
        model = glm::scale(model, glm::vec3(scale));

        // 3. rotation: add random rotation around a (semi)randomly picked rotation axis vector
        float rotAngle = static_cast<float>((rand() % 360));
        model = glm::rotate(model, rotAngle, glm::vec3(0.4f, 0.6f, 0.8f));

        // 4. now add to list of matrices
        instancesModelMatrices.push_back(model);
    }

    Model asteroids(FileSystem::getPath("resources/objects/rocks/rock_001.obj").c_str(), instances, instancesModelMatrices);

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

    //Transparent plane
	unsigned int transparentVAO, transparentVBO;
	glGenVertexArrays(1, &transparentVAO);
	glGenBuffers(1, &transparentVBO);
	glBindVertexArray(transparentVAO);
    glBindBuffer(GL_ARRAY_BUFFER, transparentVBO);
	glBufferData(GL_ARRAY_BUFFER, sizeof(transparentVertices), transparentVertices, GL_STATIC_DRAW);
	glEnableVertexAttribArray(0);
    glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 5 * sizeof(float), (void*)0);
	glEnableVertexAttribArray(1);
    glVertexAttribPointer(1, 2, GL_FLOAT, GL_FALSE, 5 * sizeof(float), (void*)(3 * sizeof(float)));
	glBindVertexArray(0);

    //Uniform Block Index
	unsigned int uniformBlockIndexModel = glGetUniformBlockIndex(modelShader.ID, "Matrices");
	glUniformBlockBinding(modelShader.ID, uniformBlockIndexModel, 0);
	unsigned int uniformBlockIndexTransparent = glGetUniformBlockIndex(transparentShader.ID, "Matrices");
	glUniformBlockBinding(transparentShader.ID, uniformBlockIndexTransparent, 0);
	unsigned int uniformBlockIndexSkybox = glGetUniformBlockIndex(skyboxShader.ID, "Matrices");
	glUniformBlockBinding(skyboxShader.ID, uniformBlockIndexSkybox, 0);
	unsigned int uniformBlockIndexNormalVisualization = glGetUniformBlockIndex(normalVisualizationShader.ID, "Matrices");
	glUniformBlockBinding(normalVisualizationShader.ID, uniformBlockIndexNormalVisualization, 0);
	unsigned int uniformBlockIndexAsteroids = glGetUniformBlockIndex(asteroidShader.ID, "Matrices");
	glUniformBlockBinding(asteroidShader.ID, uniformBlockIndexAsteroids, 0);

    unsigned int uboMatrices;
	glGenBuffers(1, &uboMatrices);
	glBindBuffer(GL_UNIFORM_BUFFER, uboMatrices);
	glBufferData(GL_UNIFORM_BUFFER, 2 * sizeof(glm::mat4), NULL, GL_STATIC_DRAW);
	glBindBuffer(GL_UNIFORM_BUFFER, 0);
	glBindBufferRange(GL_UNIFORM_BUFFER, 0, uboMatrices, 0, 2 * sizeof(glm::mat4));

	glm::mat4 projection = glm::perspective(glm::radians(camera.Zoom), (float)CURR_WIDTH / (float)CURR_HEIGHT, 0.1f, 1000.0f);
	glBindBuffer(GL_UNIFORM_BUFFER, uboMatrices);
	glBufferSubData(GL_UNIFORM_BUFFER, 0, sizeof(glm::mat4), glm::value_ptr(projection));
	glBindBuffer(GL_UNIFORM_BUFFER, 0);

	// configure global opengl state
    glEnable(GL_DEPTH_TEST);
    glEnable(GL_CULL_FACE);
    glCullFace(GL_BACK);
    glFrontFace(GL_CCW);
	//glPolygonMode(GL_FRONT_AND_BACK, GL_LINE);
    glEnable(GL_BLEND);
    glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);

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

        //depth sorting transparent objects
        std::map<float, glm::vec3> sorted;
        for (unsigned int i = 0; i < windows.size(); i++)
        {
            float distance = glm::length(camera.Position - windows[i]);
            sorted[distance] = windows[i];
        }

        //render
        //------
		postProcessEffect->Bind();
		glEnable(GL_DEPTH_TEST); // enable depth testing (is disabled for rendering screen-space quad)
		glEnable(GL_CULL_FACE);

        glClearColor(0.2f, 0.3f, 0.3f, 1.0f);
        glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

        glm::mat4 view = camera.GetViewMatrix();
	    projection = glm::perspective(glm::radians(camera.Zoom), (float)CURR_WIDTH / (float)CURR_HEIGHT, 0.1f, 1000.0f);
		glBindBuffer(GL_UNIFORM_BUFFER, uboMatrices);
		glBufferSubData(GL_UNIFORM_BUFFER, 0, sizeof(glm::mat4), glm::value_ptr(projection));
		glBufferSubData(GL_UNIFORM_BUFFER, sizeof(glm::mat4), sizeof(glm::mat4), glm::value_ptr(view));
		glBindBuffer(GL_UNIFORM_BUFFER, 0);


        modelShader.Activate();
        //setting lighting uniforms
        modelShader.setVec3("viewPos", camera.Position);
        /*
		skybox.cubemap.Bind();
		skybox.cubemap.SetShaderUniform(modelShader, "skybox");
        */
        modelShader.setFloat("material.shininess", 16.0f);
        modelShader.setBool("blinn", true);
        modelShader.setInt("numDirLights", numDirLights);
        modelShader.setInt("numPointLights", numPointLights);
        modelShader.setInt("numSpotLights", numSpotLights);
        dirLight.setInShader(modelShader, "dirLights[0]");
        pointLight.setInShader(modelShader, "pointLights[0]");
        spotLight.setInShader(modelShader, "spotLights[0]");

		//modelShader.setFloat("time", currentFrame);

        //render the loaded model
        glm::mat4 model = glm::mat4(1.0f);
        model = glm::translate(model, glm::vec3(0.0f, -3.0f, 0.0f));
        model = glm::scale(model, glm::vec3(4.0f, 4.0f, 4.0f));
        modelShader.setMat4("model", model);
        defaultModel.Draw(modelShader);

        /*
		normalVisualizationShader.Activate();
        normalVisualizationShader.setMat4("model", model);
        defaultModel.Draw(normalVisualizationShader);
        */

		//render asteroids
        asteroidShader.Activate();
        asteroidShader.setVec3("viewPos", camera.Position);
        asteroidShader.setFloat("material.shininess", 16.0f);
        asteroidShader.setBool("blinn", true);
        asteroidShader.setInt("numDirLights", numDirLights);
        asteroidShader.setInt("numPointLights", numPointLights);
        asteroidShader.setInt("numSpotLights", numSpotLights);
        dirLight.setInShader(asteroidShader, "dirLights[0]");
        pointLight.setInShader(asteroidShader, "pointLights[0]");
        spotLight.setInShader(asteroidShader, "spotLights[0]");
		asteroids.Draw(asteroidShader);

		glDisable(GL_CULL_FACE);

        //render the skybox
        view = glm::mat4(glm::mat3(camera.GetViewMatrix()));
		skyboxShader.Activate();
		skyboxShader.setMat4("view", view);
		skyboxShader.setMat4("projection", projection);
		skybox.Draw(skyboxShader);

        //render transparents objects
        /*
		transparentShader.Activate();
        glBindVertexArray(transparentVAO);
        glActiveTexture(GL_TEXTURE0);
		transparentTexture.Bind();
		transparentTexture.SetShaderUniform(transparentShader, "texture1");
        for (std::map<float, glm::vec3>::reverse_iterator it = sorted.rbegin(); it != sorted.rend(); ++it)
        {
            model = glm::mat4(1.0f);
            model = glm::translate(model, it->second);
            transparentShader.setMat4("model", model);
            glDrawArrays(GL_TRIANGLES, 0, 6);
        }
        */
		glBindVertexArray(0);

		postProcessEffect->Unbind();
        glDisable(GL_DEPTH_TEST);

		glClearColor(1.0f, 1.0f, 1.0f, 1.0f);
        glClear(GL_COLOR_BUFFER_BIT);

		postProcessEffect->Render(postprocessShader);

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
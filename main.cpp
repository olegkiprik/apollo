#include "shader.h"
#include "model.h"
#include "mesh.h"
#include "camera.h"
#include <glad/glad.h>
#include <GLFW/glfw3.h>

#include <iostream>
#include <fstream>
#include <sstream>

extern const float skyboxVertices[108];

template<class T>
T quick_power(T x, int y);

int binomialCoeff(int i, int n);
float Bfunction(float t, int n, int i);
float Zfunction(float x, float y, const float *points);
glm::vec3 Pfunc(float u, float v, const float *points);
glm::vec3 PuFunc(float u, float v, const float *points);
glm::vec3 PvFunc(float u, float v, const float *points);

// not normalized
glm::vec3 NVec(float u, float v, const float *points);

struct GlobalAttributes;
void updateTriangles(GlobalAttributes& attr);

void framebuffer_size_callback(GLFWwindow *window, int width, int height);
void mouse_callback(GLFWwindow *window, double xpos, double ypos);
void scroll_callback(GLFWwindow *window, double xoffset, double yoffset);
void key_callback(GLFWwindow *window, int key, int scancode, int action, int mods);
void processInput(GLFWwindow *window);

unsigned int loadCubemap(const std::string* faces,
                         bool flipVertBefore = false,
                         bool flipVertAfter = true);

glm::mat3 normMatrix(const glm::mat4& mx) {
    return glm::mat3(glm::transpose(glm::inverse(mx)));
}

enum class CameraMode {
    Explore,
    ShuttleFPP,
    Static,
    LookingAt
};

enum class Shading {
    Flat,
    Gouraud,
    Phong
};

const int BEZIER_N = 3;
const int BEZIER_M = 3;

const int GL_MAJOR = 3;
const int GL_MINOR = 3;

const unsigned int SCR_WIDTH = 1920;
const unsigned int SCR_HEIGHT = 1080;

const int u_nr_points = 50;
const int v_nr_points = 50;

const float moonShininess = 4;
const float cityShininess = 4;
const float shuttleShininess = 6;

const glm::vec3 bezierColor(0.4f, 0.3f, 0.6f);

struct GlobalAttributes {
    
    GlobalAttributes() : camera(glm::vec3(0.f, 0.f, 3.f)) {}

    unsigned int trVBO, trVAO;

    float controlPointZs[(BEZIER_N+1) * (BEZIER_M+1)];
    float controlPointPhases[(BEZIER_N+1) * (BEZIER_M+1)];
    float controlPointAmplitudes[(BEZIER_N+1) * (BEZIER_M+1)];
    
    std::vector<float> triangles;

    Camera camera;

    bool firstMouse = true;

    float deltaTime = 0.f;
    float lastFrame = 0.f;
    float gamma_val = 2.2f;

    bool shuttleMoving = true;
    bool reflectorsUp = false;
    bool reflectorsDown = false;

    float lastX = SCR_WIDTH / 2.f;
    float lastY = SCR_HEIGHT / 2.f;

    bool day = true;

    float fogDensityDay = 0.001f;
    float fogDensityNight = 0.0001f;

    CameraMode cameraMode = CameraMode::Explore;

    Shading shading = Shading::Phong;
};

GlobalAttributes* callback_attributes = NULL;

int main()
{
    GlobalAttributes attr;
    callback_attributes = &attr;

    for (int i = 0; i < (BEZIER_M+1) * (BEZIER_N+1); ++i) {
        attr.controlPointPhases[i]     = (float)std::rand() * 6 / RAND_MAX;
        attr.controlPointAmplitudes[i] = (float)std::rand() * 3 / RAND_MAX;
        attr.controlPointZs[i]         = (float)std::rand() * 1 / RAND_MAX;
    }

    const int NR_SQR_VERTICES = 6;
    const int NR_VX_ATTR = 9;

    attr.triangles.resize((u_nr_points - 1) * (v_nr_points - 1) * NR_SQR_VERTICES * NR_VX_ATTR);

    const std::vector<std::string> faces{
        "skybox/right.jpg",
        "skybox/left.jpg",
        "skybox/top.jpg",
        "skybox/bottom.jpg",
        "skybox/front.jpg",
        "skybox/back.jpg"
    };

    glfwInit();
    glfwWindowHint(GLFW_SAMPLES, 4); // antialiasing
    glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, GL_MAJOR);
    glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, GL_MINOR);
    glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE); // enable core profile

#ifdef __APPLE__
    glfwWindowHint(GLFW_OPENGL_FORWARD_COMPAT, GL_TRUE);
#endif

    GLFWwindow *window = glfwCreateWindow(SCR_WIDTH, SCR_HEIGHT,
        "Apollo", glfwGetPrimaryMonitor(), NULL); // fullscreen

    if (!window)
    {
        std::cout << "Failed to create GLFW window" << std::endl;
        glfwTerminate();
        return EXIT_FAILURE;
    }

    glfwMakeContextCurrent(window);
    glfwSetFramebufferSizeCallback(window, framebuffer_size_callback);
    glfwSetCursorPosCallback(window, mouse_callback);
    glfwSetScrollCallback(window, scroll_callback);
    glfwSetKeyCallback(window, key_callback);
    glfwSetInputMode(window, GLFW_CURSOR, GLFW_CURSOR_DISABLED); // cursor is invisible

    if (!gladLoadGLLoader((GLADloadproc)glfwGetProcAddress))
    {
        std::cout << "Failed to initialize GLAD\n";
        glfwTerminate();
        return EXIT_FAILURE;
    }

    stbi_set_flip_vertically_on_load(true);

    // enable

    glEnable(GL_DEPTH_TEST);
    glEnable(GL_CULL_FACE);
    glEnable(GL_MULTISAMPLE);
    
    // Bezier init

    glGenVertexArrays(1, &attr.trVAO);
    glGenBuffers(1, &attr.trVBO);

    glBindBuffer(GL_ARRAY_BUFFER, attr.trVBO);
    glBufferData(GL_ARRAY_BUFFER, attr.triangles.size() * sizeof(float), NULL, GL_STREAM_DRAW);
    glBindVertexArray(attr.trVAO);
    glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 9 * sizeof(float), (void *)0);
    glEnableVertexAttribArray(0);
    glVertexAttribPointer(1, 3, GL_FLOAT, GL_FALSE, 9 * sizeof(float), (void *)(3 * sizeof(float)));
    glEnableVertexAttribArray(1);
    glVertexAttribPointer(2, 3, GL_FLOAT, GL_FALSE, 9 * sizeof(float), (void *)(6 * sizeof(float)));
    glEnableVertexAttribArray(2);

    // skybox init

    unsigned int skyboxVAO, skyboxVBO;
    glGenVertexArrays(1, &skyboxVAO);
    glGenBuffers(1, &skyboxVBO);
    glBindVertexArray(skyboxVAO);
    glBindBuffer(GL_ARRAY_BUFFER, skyboxVBO);
    glBufferData(GL_ARRAY_BUFFER, sizeof(skyboxVertices), &skyboxVertices, GL_STATIC_DRAW);
    glEnableVertexAttribArray(0);
    glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 3 * sizeof(float), (void *)0);

    unsigned int cubemapTexture = loadCubemap(faces.data());

    // shader init

    Shader skyboxShader("skybox_shader_vert.glsl", "skybox_shader_frag.glsl");
    Shader lightShader("light_shader_vert.glsl", "light_shader_frag.glsl");

    Shader flatShader("flat_shader_vert.glsl",
                      "flat_shader_frag.glsl",
                      "flat_shader_geom.glsl");

    Shader gouraudShader("gouraud_shader_vert.glsl", "gouraud_shader_frag.glsl");
    Shader phongShader("phong_shader_vert.glsl", "phong_shader_frag.glsl");

    skyboxShader.use();
    skyboxShader.setInt("skybox", 0);

    // gamma correction enabled

    Model cityModel_meshes("city/FabConvert.com_city.obj", true);
    //Model moonModel_meshes("moon/FabConvert.com_nasa_cgi_moon_kit.obj", true);
    Model shuttleModel_meshes("shuttle/FabConvert.com_orbiter_space_shuttle_ov-103_discovery.obj", true);

    float angle = 0;          // shuttle flying around
    float angleOffset = 0.3f; // rotating reflectors

    while (!glfwWindowShouldClose(window))
    {
        float currentFrame = glfwGetTime();
        attr.deltaTime = currentFrame - attr.lastFrame;
        attr.lastFrame = currentFrame;

        // adjusting

        if (attr.shuttleMoving) {
            angle += attr.deltaTime * 0.6f;
        }

        if (attr.reflectorsUp) {
            angleOffset += attr.deltaTime * 0.8f;
        }

        if (attr.reflectorsDown) {
            angleOffset -= attr.deltaTime * 0.8f;
        }

        processInput(window);

        glClearColor(0.f, 0.f, 0.f, 1.f);
        glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

        // projection

        glm::mat4 projection = glm::perspective(glm::radians(attr.camera.Zoom), // zoom enabled
            (float)SCR_WIDTH / (float)SCR_HEIGHT, 0.1f, 10000.f);

        // model

        glm::mat4 shuttleMovingMx = glm::mat4(1.f);
        shuttleMovingMx = glm::translate(shuttleMovingMx, glm::vec3(-1350.f, 1500.f, 0.f));
        shuttleMovingMx = glm::rotate(shuttleMovingMx, glm::radians(-45.f), glm::vec3(0.f, 0.f, 1.f));
        shuttleMovingMx = glm::rotate(shuttleMovingMx, angle, glm::vec3(0.f, 1.f, 0.f));
        shuttleMovingMx = glm::translate(shuttleMovingMx, glm::vec3(-2000.f, 0.f, 0.f));

        glm::mat4 shuttleModel = glm::mat4(1.f);
        shuttleModel = glm::scale(shuttleModel, glm::vec3(1.f, 1.f, 1.f));
        shuttleModel = glm::translate(shuttleModel, glm::vec3(0.f, -5.f, 0.f));
        shuttleModel = shuttleMovingMx * shuttleModel;

        glm::mat4 moonModel = glm::mat4(1.f);
        moonModel = glm::translate(moonModel, glm::vec3(-3000.f, 3000.f, 0.f));
        moonModel = glm::scale(moonModel, glm::vec3(20.f, 20.f, 20.f));
        moonModel = glm::rotate(moonModel, glm::radians(300.f), glm::vec3(0.f, 0.f, 1.f));

        glm::mat4 sunModel = glm::mat4(1.f);
        sunModel = glm::translate(sunModel, glm::vec3(-2500.f, 2500.f, -4000.f));
        sunModel = glm::scale(sunModel, glm::vec3(30.f, 30.f, 30.f));

        glm::mat4 refl1Model = glm::mat4(1.f);
        refl1Model = glm::translate(refl1Model, glm::vec3(1.f, -7.f, 14.f));
        refl1Model = glm::scale(refl1Model, glm::vec3(0.03f, 0.03f, 0.03f));
        refl1Model = shuttleMovingMx * refl1Model;

        glm::mat4 refl2Model = glm::mat4(1.f);
        refl2Model = glm::translate(refl2Model, glm::vec3(-1.f, -7.f, 14.f));
        refl2Model = glm::scale(refl2Model, glm::vec3(0.03f, 0.03f, 0.03f));
        refl2Model = shuttleMovingMx * refl2Model;

        glm::mat4 cityModel = glm::mat4(1.f);
        cityModel = glm::translate(cityModel, glm::vec3(0.f, 0.f, 0.f));
        cityModel = glm::scale(cityModel, glm::vec3(1.f, 1.f, 1.f));

        glm::mat4 bezierModel = glm::mat4(1.f);
        bezierModel = glm::translate(bezierModel, glm::vec3(30.f, 20.f, -30.f));
        bezierModel = glm::scale(bezierModel, glm::vec3(200.f/3, 100.f/3, 100.f/3));

        // view

        glm::mat4 view;

        if (attr.cameraMode == CameraMode::Explore) {
            view = attr.camera.GetViewMatrix();
        }
        else if (attr.cameraMode == CameraMode::ShuttleFPP) {
            view = glm::inverse(glm::scale(shuttleMovingMx, glm::vec3(-1.f, 1.f, -1.f)));
        }
        else if (attr.cameraMode == CameraMode::Static) {
            view = glm::transpose(glm::mat4(0.372964f,  2.6077e-06f, -0.927846f,   -7.06786f,
                                            0.397981f,  0.903338f,    0.159978f, -106.224f,
                                            0.838158f, -0.428931f,    0.336911f, -292.188f,
                                            0.f,        0.f,          0.f,          1.f));
        }
        else if (attr.cameraMode == CameraMode::LookingAt) {
            glm::vec3 shuttlePosition = glm::vec3(shuttleMovingMx * glm::vec4(0.f, 0.f, 0.f, 1.f));
            view = glm::lookAt(glm::vec3(-200.f, 5.f, 0.f),
                               shuttlePosition,
                               glm::vec3(0.f, 1.f, 0.f));
        }

        // reflectors' direction

        glm::vec3 refl1Direction = glm::vec3(0.3f, 0.f, 0.f);
        glm::vec3 refl2Direction = glm::vec3(-0.3f, 0.f, 0.f);

        glm::vec3 shuttleDirectionView1 = glm::normalize(glm::vec3(normMatrix(view * shuttleMovingMx) *
                                          glm::normalize(glm::vec3(0.f, angleOffset, -1.f) + refl1Direction)));

        glm::vec3 shuttleDirectionView2 = glm::normalize(glm::vec3(normMatrix(view * shuttleMovingMx) *
                                          glm::normalize(glm::vec3(0.f, angleOffset, -1.f) + refl2Direction)));

        // main objects

        Shader &mainShader = (attr.shading == Shading::Flat ? flatShader :
                             (attr.shading == Shading::Gouraud ? gouraudShader : phongShader));
        
        mainShader.use();

        mainShader.setFloat("gamma", attr.gamma_val);
        mainShader.setFloat("lights[0].constant", 1.f); // sun
        mainShader.setFloat("lights[1].constant", 1.f); // moon
        mainShader.setFloat("lights[2].constant", 1.f); // refl 1
        mainShader.setFloat("lights[3].constant", 1.f); // refl 2

        mainShader.setFloat("lights[0].linear", 0.0001f);
        mainShader.setFloat("lights[1].linear", 0.001f);
        mainShader.setFloat("lights[2].linear", 0.01f);
        mainShader.setFloat("lights[3].linear", 0.01f);
        
        mainShader.setFloat("lights[0].quadratic", 0.00f); // gamma correction enabled
        mainShader.setFloat("lights[1].quadratic", 0.00f);
        mainShader.setFloat("lights[2].quadratic", 0.00f);
        mainShader.setFloat("lights[3].quadratic", 0.00f);
        
        mainShader.setFloat("spotlightComponents[0].cutOff", glm::cos(glm::radians(12.5f)));
        mainShader.setFloat("spotlightComponents[1].cutOff", glm::cos(glm::radians(12.5f)));

        mainShader.setFloat("spotlightComponents[0].outerCutoff", glm::cos(glm::radians(17.5f)));
        mainShader.setFloat("spotlightComponents[1].outerCutoff", glm::cos(glm::radians(17.5f)));

        if (attr.day)
        {
            mainShader.setVec3("lights[0].ambientLightColor", glm::vec3(0.08f, 0.08f, 0.08f));
            mainShader.setVec3("lights[1].ambientLightColor", glm::vec3(0.f, 0.f, 0.f));
            mainShader.setVec3("lights[2].ambientLightColor", glm::vec3(0.f, 0.f, 0.f));
            mainShader.setVec3("lights[3].ambientLightColor", glm::vec3(0.f, 0.f, 0.f));

            mainShader.setVec3("lights[0].diffuseLightColor", glm::vec3(1.f, 1.f, 1.f));
            mainShader.setVec3("lights[1].diffuseLightColor", glm::vec3(0.1f, 0.25f, 0.25f)); // moon is slightly lighting
            mainShader.setVec3("lights[2].diffuseLightColor", glm::vec3(1.f, 1.f, 1.f));
            mainShader.setVec3("lights[3].diffuseLightColor", glm::vec3(1.f, 1.f, 1.f));

            mainShader.setVec3("lights[0].specularLightColor", glm::vec3(0.5f, 0.5f, 0.5f));
            mainShader.setVec3("lights[1].specularLightColor", glm::vec3(0.1f, 0.35f, 0.35f));
            mainShader.setVec3("lights[2].specularLightColor", glm::vec3(0.6f, 0.6f, 0.6f));
            mainShader.setVec3("lights[3].specularLightColor", glm::vec3(0.6f, 0.6f, 0.6f));
        }
        else
        {
            mainShader.setVec3("lights[0].ambientLightColor", glm::vec3(0.028f, 0.028f, 0.028f));
            mainShader.setVec3("lights[1].ambientLightColor", glm::vec3(0.f, 0.f, 0.f));
            mainShader.setVec3("lights[2].ambientLightColor", glm::vec3(0.f, 0.f, 0.f));
            mainShader.setVec3("lights[3].ambientLightColor", glm::vec3(0.f, 0.f, 0.f));

            mainShader.setVec3("lights[0].diffuseLightColor", glm::vec3(0.f, 0.f, 0.f));
            mainShader.setVec3("lights[1].diffuseLightColor", glm::vec3(0.27f, 0.12f, 0.08f));
            mainShader.setVec3("lights[2].diffuseLightColor", glm::vec3(1.f, 1.f, 1.f));
            mainShader.setVec3("lights[3].diffuseLightColor", glm::vec3(1.f, 1.f, 1.f));

            mainShader.setVec3("lights[0].specularLightColor", glm::vec3(0.f, 0.f, 0.f));
            mainShader.setVec3("lights[1].specularLightColor", glm::vec3(0.9f, 0.4f, 0.2f));
            mainShader.setVec3("lights[2].specularLightColor", glm::vec3(0.6f, 0.6f, 0.6f));
            mainShader.setVec3("lights[3].specularLightColor", glm::vec3(0.6f, 0.6f, 0.6f));
        }

        mainShader.setVec3("spotlightComponents[0].spotDirectionView", shuttleDirectionView1);
        mainShader.setVec3("spotlightComponents[1].spotDirectionView", shuttleDirectionView2);

        mainShader.setMat4("view", view);
        mainShader.setMat4("projection", projection);

        // city

        mainShader.setMat4("model", cityModel);
        mainShader.setMat3("normViewModelMatrix", normMatrix(view * cityModel));

        mainShader.setVec3("lights[0].lightPosView", view * sunModel * glm::vec4(0.f, 0.f, 0.f, 1.f)); // sun
        mainShader.setVec3("lights[1].lightPosView", view * moonModel * glm::vec4(0.f, 0.f, 0.f, 1.f)); // moon
        mainShader.setVec3("lights[2].lightPosView", view * refl1Model * glm::vec4(0.f, 0.f, 0.f, 1.f)); // refl 1
        mainShader.setVec3("lights[3].lightPosView", view * refl2Model * glm::vec4(0.f, 0.f, 0.f, 1.f)); // refl 2
        
        mainShader.setFloat("shininess", cityShininess);

        if (attr.day) {
            mainShader.setFloat("fogDensity", attr.fogDensityDay);
            mainShader.setVec3("fogColor", glm::vec3(0.2f, 0.2f, 0.2f));
        } else {
            mainShader.setFloat("fogDensity", attr.fogDensityNight);
            mainShader.setVec3("fogColor", glm::vec3(0.1f, 0.02f, 0.f));
        }

        cityModel_meshes.Draw(mainShader);

        // moon

        mainShader.setFloat("fogDensity", 0.f);

        mainShader.setMat4("model", moonModel);
        mainShader.setMat3("normViewModelMatrix", normMatrix(view * moonModel));
        mainShader.setFloat("shininess", moonShininess);

        if (attr.day) {
            mainShader.setVec3("lights[0].diffuseLightColor", glm::vec3(1.f, 1.f, 1.f));
        } else {
            mainShader.setVec3("lights[0].diffuseLightColor", glm::vec3(0.9f, 0.4f, 0.2f));
        }

        //moonModel_meshes.Draw(mainShader);

        // shuttle

        if (attr.day) {
            mainShader.setVec3("lights[0].diffuseLightColor", glm::vec3(1.f, 1.f, 1.f));
        } else {
            mainShader.setVec3("lights[0].diffuseLightColor", glm::vec3(0.f, 0.f, 0.f));
        }

        mainShader.setMat4("model", shuttleModel);
        mainShader.setMat3("normViewModelMatrix", normMatrix(view * shuttleModel));
        mainShader.setFloat("shininess", shuttleShininess);
        shuttleModel_meshes.Draw(mainShader);

        // Bezier surface

        glBindVertexArray(attr.trVAO);
        mainShader.setMat4("model", bezierModel);
        mainShader.setMat3("normViewModelMatrix", normMatrix(view * bezierModel));
        mainShader.setFloat("shininess", shuttleShininess);

        updateTriangles(attr);

        if (attr.day) {
            mainShader.setFloat("fogDensity", attr.fogDensityDay);
            mainShader.setVec3("fogColor", glm::vec3(0.2f, 0.2f, 0.2f));
        } else {
            mainShader.setFloat("fogDensity", attr.fogDensityNight);
            mainShader.setVec3("fogColor", glm::vec3(0.1f, 0.02f, 0.f));
        }

        glDisable(GL_CULL_FACE); // disable face culling to draw both sides
        glDrawArrays(GL_TRIANGLES, 0, attr.triangles.size());
        glEnable(GL_CULL_FACE);

        mainShader.setFloat("fogDensity", 0.f);

        // sun

        lightShader.use();
        lightShader.setMat4("view", view);
        lightShader.setMat4("projection", projection);
        lightShader.setMat4("model", sunModel);
        lightShader.setMat3("normViewModelMatrix", normMatrix(view * sunModel));

        //if (attr.day)
            //moonModel_meshes.Draw(lightShader);

        // reflectors

        lightShader.setMat4("model", refl1Model);
        lightShader.setMat3("normViewModelMatrix", normMatrix(view * refl1Model));
        //moonModel_meshes.Draw(lightShader);

        lightShader.setMat4("model", refl2Model);
        lightShader.setMat3("normViewModelMatrix", normMatrix(view * refl2Model));
        //moonModel_meshes.Draw(lightShader);

        // skybox

        glDepthFunc(GL_LEQUAL);
        skyboxShader.use();
        view = glm::mat4(glm::mat3(view)); // view without translation
        skyboxShader.setMat4("view", view);
        skyboxShader.setMat4("projection", projection);

        glBindVertexArray(skyboxVAO);
        glActiveTexture(GL_TEXTURE0);
        glBindTexture(GL_TEXTURE_CUBE_MAP, cubemapTexture);

        if (attr.day)
            glDrawArrays(GL_TRIANGLES, 0, 36);

        glBindVertexArray(0);
        glDepthFunc(GL_LESS);

        glfwSwapBuffers(window);
        glfwPollEvents();
    }

    glfwTerminate();
    return EXIT_SUCCESS;
}

void processInput(GLFWwindow *window)
{
    if (!callback_attributes)
        return;

    GlobalAttributes& attr = *callback_attributes;

    if (glfwGetKey(window, GLFW_KEY_ESCAPE) == GLFW_PRESS)
        glfwSetWindowShouldClose(window, true);

    if (glfwGetKey(window, GLFW_KEY_W) == GLFW_PRESS)
        attr.camera.ProcessKeyboard(Camera_Movement::FORWARD, attr.deltaTime);
    if (glfwGetKey(window, GLFW_KEY_S) == GLFW_PRESS)
        attr.camera.ProcessKeyboard(Camera_Movement::BACKWARD, attr.deltaTime);
    if (glfwGetKey(window, GLFW_KEY_A) == GLFW_PRESS)
        attr.camera.ProcessKeyboard(Camera_Movement::LEFT, attr.deltaTime);
    if (glfwGetKey(window, GLFW_KEY_D) == GLFW_PRESS)
        attr.camera.ProcessKeyboard(Camera_Movement::RIGHT, attr.deltaTime);

    if (glfwGetKey(window, GLFW_KEY_UP) == GLFW_PRESS)
        attr.camera.ProcessKeyboard(Camera_Movement::FORWARD, attr.deltaTime*15);
    if (glfwGetKey(window, GLFW_KEY_DOWN) == GLFW_PRESS)
        attr.camera.ProcessKeyboard(Camera_Movement::BACKWARD, attr.deltaTime*15);
    if (glfwGetKey(window, GLFW_KEY_RIGHT) == GLFW_PRESS)
        attr.camera.ProcessKeyboard(Camera_Movement::RIGHT, attr.deltaTime*15);
    if (glfwGetKey(window, GLFW_KEY_LEFT) == GLFW_PRESS)
        attr.camera.ProcessKeyboard(Camera_Movement::LEFT, attr.deltaTime*15);

    if (glfwGetKey(window, GLFW_KEY_LEFT_BRACKET) == GLFW_PRESS) {
        attr.fogDensityDay += attr.deltaTime * 0.001f;
    }
    if (glfwGetKey(window, GLFW_KEY_RIGHT_BRACKET) == GLFW_PRESS) {
        attr.fogDensityDay -= attr.deltaTime * 0.001f;
        if (attr.fogDensityDay < 0) {
            attr.fogDensityDay = 0;
        }
    }
    if (glfwGetKey(window, GLFW_KEY_PERIOD) == GLFW_PRESS) {
        attr.gamma_val += attr.deltaTime * 0.1f;
    }
    if (glfwGetKey(window, GLFW_KEY_SLASH) == GLFW_PRESS) {
        attr.gamma_val -= attr.deltaTime * 0.1f;
        if (attr.gamma_val < 1) {
            attr.gamma_val = 1;
        }
    }
}

void key_callback(GLFWwindow *window, int key, int scancode, int action, int mods)
{
    if (!callback_attributes)
        return;

    GlobalAttributes& attr = *callback_attributes;

    if (key == GLFW_KEY_N && action == GLFW_PRESS)
    {
        attr.day = !attr.day;
    }
    if (key == GLFW_KEY_1 && action == GLFW_PRESS)
    {
        attr.cameraMode = CameraMode::Explore;
    }
    if (key == GLFW_KEY_2 && action == GLFW_PRESS)
    {
        attr.cameraMode = CameraMode::ShuttleFPP;
    }
    if (key == GLFW_KEY_3 && action == GLFW_PRESS)
    {
        attr.cameraMode = CameraMode::LookingAt;
    }
    if (key == GLFW_KEY_4 && action == GLFW_PRESS)
    {
        attr.cameraMode = CameraMode::Static;
    }
    if (key == GLFW_KEY_F && action == GLFW_PRESS)
    {
        attr.shading = Shading::Flat;
    }
    if (key == GLFW_KEY_G && action == GLFW_PRESS)
    {
        attr.shading = Shading::Gouraud;
    }
    if (key == GLFW_KEY_P && action == GLFW_PRESS)
    {
        attr.shading = Shading::Phong;
    }
    if (key == GLFW_KEY_E && action == GLFW_PRESS)
    {
        attr.shuttleMoving = !attr.shuttleMoving;
    }
    if (key == GLFW_KEY_Z && action == GLFW_PRESS) {
        attr.reflectorsUp = true;
    }
    if (key == GLFW_KEY_X && action == GLFW_PRESS) {
        attr.reflectorsDown = true;
    }
    if ((key == GLFW_KEY_Z || key == GLFW_KEY_X) && action == GLFW_RELEASE) {
        attr.reflectorsDown = attr.reflectorsUp = false;
    }
}

void mouse_callback(GLFWwindow *window, double xpos, double ypos)
{
    if (!callback_attributes)
        return;

    GlobalAttributes& attr = *callback_attributes;

    if (attr.firstMouse)
    {
        attr.lastX = xpos;
        attr.lastY = ypos;
        attr.firstMouse = false;
    }

    float xoffset = xpos - attr.lastX;
    float yoffset = attr.lastY - ypos;

    attr.lastX = xpos;
    attr.lastY = ypos;

    attr.camera.ProcessMouseMovement(xoffset, yoffset);
}

void scroll_callback(GLFWwindow *window, double xoffset, double yoffset)
{
    if (!callback_attributes)
        return;

    GlobalAttributes& attr = *callback_attributes;

    attr.camera.ProcessMouseScroll(yoffset);
}

void framebuffer_size_callback(GLFWwindow *window, int width, int height)
{
    glViewport(0, 0, width, height);
}

unsigned int loadCubemap(const std::string* faces,
                         bool flipVertBefore,
                         bool flipVertAfter)
{
    unsigned int textureID;
    glGenTextures(1, &textureID);
    glBindTexture(GL_TEXTURE_CUBE_MAP, textureID);

    stbi_set_flip_vertically_on_load(flipVertBefore);

    int width, height, nrChannels;
    for (unsigned int i = 0; i < 6; i++)
    {
        unsigned char *data = stbi_load(faces[i].c_str(), &width, &height, &nrChannels, 0);
        if (data)
        {
            glTexImage2D(GL_TEXTURE_CUBE_MAP_POSITIVE_X + i, 0, GL_RGB, width, height,
                         0, GL_RGB, GL_UNSIGNED_BYTE, data);
        }
        else
        {
            std::cout << "Cubemap texture failed to load at path: " << faces[i] << '\n';
        }
        stbi_image_free(data);
    }

    stbi_set_flip_vertically_on_load(flipVertAfter);

    glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
    glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
    glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_WRAP_R, GL_CLAMP_TO_EDGE);

    return textureID;
}

void updateTriangles(GlobalAttributes& attr)
{
    const int NR_CTRL_PT = (BEZIER_M+1) * (BEZIER_N+1);
    float controlPointCurrZs[NR_CTRL_PT];

    const float BEZIER_ANIMATION_STRENGTH = 0.1f;

    for (int i = 0; i < NR_CTRL_PT; ++i) {
        controlPointCurrZs[i] = attr.controlPointZs[i] +
                                BEZIER_ANIMATION_STRENGTH * attr.controlPointAmplitudes[i] *
                                      std::sin(glfwGetTime() + attr.controlPointPhases[i]);
    }

    std::vector<glm::vec3> points;
    std::vector<glm::vec3> n_vecs;

    points.resize(u_nr_points * v_nr_points);
    n_vecs.resize(u_nr_points * v_nr_points);

    // Bezier update

    for (int i = 0; i < u_nr_points; ++i)
    {
        for (int j = 0; j < v_nr_points; ++j)
        {
            float u = (float)i / (u_nr_points - 1);
            float v = (float)j / (v_nr_points - 1);

            points[j * u_nr_points + i] = Pfunc(u, v, controlPointCurrZs);
            n_vecs[j * u_nr_points + i] = glm::normalize(NVec(u, v, controlPointCurrZs));
        }
    }

    const int NR_SQR_VERTICES = 6;
    const int NR_VX_ATTR = 9;

    attr.triangles.clear();
    attr.triangles.reserve((u_nr_points - 1) * (v_nr_points - 1) * NR_SQR_VERTICES * NR_VX_ATTR);

    const float r = bezierColor.r;
    const float g = bezierColor.g;
    const float b = bezierColor.b;

    const int j_indices[] = {0, 0, 1, 1, 0, 1};
    const int i_indices[] = {0, 1, 0, 0, 1, 1};

    for (int i = 0; i < u_nr_points - 1; ++i) {
        for (int j = 0; j < v_nr_points - 1; ++j) {
            for (int k = 0; k < NR_SQR_VERTICES; ++k) {

                attr.triangles.push_back(points[(j+j_indices[k]) * u_nr_points + (i+i_indices[k])].x);
                attr.triangles.push_back(points[(j+j_indices[k]) * u_nr_points + (i+i_indices[k])].y);
                attr.triangles.push_back(points[(j+j_indices[k]) * u_nr_points + (i+i_indices[k])].z);

                attr.triangles.push_back(n_vecs[(j+j_indices[k]) * u_nr_points + (i+i_indices[k])].x);
                attr.triangles.push_back(n_vecs[(j+j_indices[k]) * u_nr_points + (i+i_indices[k])].y);
                attr.triangles.push_back(n_vecs[(j+j_indices[k]) * u_nr_points + (i+i_indices[k])].z);

                attr.triangles.push_back(r);
                attr.triangles.push_back(g);
                attr.triangles.push_back(b);
            }
        }
    }

    glBindBuffer(GL_ARRAY_BUFFER, attr.trVBO);
    glBufferSubData(GL_ARRAY_BUFFER, 0, attr.triangles.size() * sizeof(float), attr.triangles.data());
}

// BEZIER SURFACE functions

template<class T>
T quick_power(T x, int y) {
    if (y == 0) { return (T)1; }
    if (y == 1) { return x; }

    if (y & 1) {
        return quick_power(x, y-1) * x;
    }

    T val = quick_power(x, y >> 1);
    return val * val;
}

int binomialCoeff(int i, int n)
{
    long long x = 1, y = 1;
    for (int k = 1; k <= i; ++k)
    {
        x *= k + n - i;
        y *= k;
    }
    return x / y;
}

float Bfunction(float t, int n, int i)
{
    return binomialCoeff(i, n) * quick_power(t, i) * quick_power(1.f - t, n - i);
}

float Zfunction(float x, float y, const float *points)
{
    float result = 0;
    for (int i = 0; i <= BEZIER_N; ++i)
    {
        for (int j = 0; j <= BEZIER_M; ++j)
        {
            float pt = points[j * (BEZIER_N+1) + i];
            result += pt * Bfunction(x, BEZIER_N, i) * Bfunction(y, BEZIER_M, j);
        }
    }
    return result;
}

glm::vec3 Pfunc(float u, float v, const float *points)
{
    return glm::vec3(u, v, Zfunction(u, v, points));
}

glm::vec3 PuFunc(float u, float v, const float *points)
{
    glm::vec3 result(0.f, 0.f, 0.f);
    for (int i = 0; i <= BEZIER_N - 1; ++i)
    {
        for (int j = 0; j <= BEZIER_M; ++j)
        {
            glm::vec3 Vi1j0{(float)(i + 1) / BEZIER_N, 0, points[(j + 0) * (BEZIER_N+1) + i + 1]};
            glm::vec3 Vi0j0{(float)(i)     / BEZIER_N, 0, points[(j + 0) * (BEZIER_N+1) + i + 0]};
            result += (Vi1j0 - Vi0j0) * Bfunction(u, BEZIER_N - 1, i) * Bfunction(v, BEZIER_M, j);
        }
    }
    return (float)BEZIER_N * result;
}

glm::vec3 PvFunc(float u, float v, const float *points)
{
    glm::vec3 result(0.f, 0.f, 0.f);
    for (int i = 0; i <= BEZIER_N; ++i)
    {
        for (int j = 0; j <= BEZIER_M - 1; ++j)
        {
            glm::vec3 Vi0j1{0, (float)(j + 1) / BEZIER_M, points[(j + 1) * (BEZIER_N+1) + i + 0]};
            glm::vec3 Vi0j0{0, (float)(j)     / BEZIER_M, points[(j + 0) * (BEZIER_N+1) + i + 0]};
            result += (Vi0j1 - Vi0j0) * Bfunction(u, BEZIER_N, i) * Bfunction(v, BEZIER_M - 1, j);
        }
    }
    return (float)BEZIER_M * result;
}

glm::vec3 NVec(float u, float v, const float *points)
{
    return glm::cross(PuFunc(u, v, points), PvFunc(u, v, points));
}

const float skyboxVertices[] = {
    -1.f, 1.f, -1.f,
    -1.f, -1.f, -1.f,
    1.f, -1.f, -1.f,
    1.f, -1.f, -1.f,
    1.f, 1.f, -1.f,
    -1.f, 1.f, -1.f,

    -1.f, -1.f, 1.f,
    -1.f, -1.f, -1.f,
    -1.f, 1.f, -1.f,
    -1.f, 1.f, -1.f,
    -1.f, 1.f, 1.f,
    -1.f, -1.f, 1.f,

    1.f, -1.f, -1.f,
    1.f, -1.f, 1.f,
    1.f, 1.f, 1.f,
    1.f, 1.f, 1.f,
    1.f, 1.f, -1.f,
    1.f, -1.f, -1.f,

    -1.f, -1.f, 1.f,
    -1.f, 1.f, 1.f,
    1.f, 1.f, 1.f,
    1.f, 1.f, 1.f,
    1.f, -1.f, 1.f,
    -1.f, -1.f, 1.f,

    -1.f, 1.f, -1.f,
    1.f, 1.f, -1.f,
    1.f, 1.f, 1.f,
    1.f, 1.f, 1.f,
    -1.f, 1.f, 1.f,
    -1.f, 1.f, -1.f,

    -1.f, -1.f, -1.f,
    -1.f, -1.f, 1.f,
    1.f, -1.f, -1.f,
    1.f, -1.f, -1.f,
    -1.f, -1.f, 1.f,
    1.f, -1.f, 1.f
};

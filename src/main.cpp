#include "imgui.h"
#include "imgui_impl_glfw.h"
#include "imgui_impl_opengl3.h"

#include <glad/glad.h>
#include <GLFW/glfw3.h>

#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtc/type_ptr.hpp>

#include <learnopengl/filesystem.h>
#include <learnopengl/shader.h>
#include <learnopengl/camera.h>
#include <learnopengl/model.h>

#include <iostream>

void framebuffer_size_callback(GLFWwindow *window, int width, int height);

void mouse_callback(GLFWwindow *window, double xpos, double ypos);

void scroll_callback(GLFWwindow *window, double xoffset, double yoffset);

void processInput(GLFWwindow *window);
void processInput1(GLFWwindow *window);

void key_callback(GLFWwindow *window, int key, int scancode, int action, int mods);

unsigned int loadCubemap(vector<std::string> faces);

unsigned int loadTexture(char const *path);


bool CircleColision(glm::vec3 c,float r);

void renderQuad();
// funkcija za pomeranje kamena
void moveRock(Camera_Movement direction);

// settings
const unsigned int SCR_WIDTH = 800;
const unsigned int SCR_HEIGHT = 600;
//promena
bool freeMove = true;
bool spotlightOn = true;
bool bg_changen = false;
bool bloom = true;
bool bloomKeyPressed = false;
float exposure = 1.0f;
// camera
float lastX = SCR_WIDTH / 2.0f;
float lastY = SCR_HEIGHT / 2.0f;
bool firstMouse = false;

glm::vec3 lastCamPosition;

// timing
float deltaTime = 0.0f;
float lastFrame = 0.0f;

struct PointLight {
    glm::vec3 position;
    glm::vec3 ambient;
    glm::vec3 diffuse;
    glm::vec3 specular;

    float constant;
    float linear;
    float quadratic;
};

struct ProgramState {
    glm::vec3 clearColor = glm::vec3(0);
    glm::vec3 dirLightDir = glm::vec3(-0.2f, -1.0f, -0.3f);
    glm::vec3 dirLightAmbDiffSpec = glm::vec3(0.3f, 0.3f,0.2f);
    bool ImGuiEnabled = false;
    Camera camera;
    bool CameraMouseMovementUpdateEnabled = true;
    glm::vec3 vecCalibrate = glm::vec3(0.0f); //pomocna promenljiva za podesavanje objekata
    glm::vec3 vecRotate = glm::vec3(0.0f);//pomocna promenljiva za podesavanje objekata
    float fineCalibrate = 0.001f;//pomocna promenljiva za podesavanje objekata
    //test
    vector<std::string> faces;
    unsigned int cubemapTexture;
    //-----
    PointLight pointLight;
    ProgramState()
            : camera(glm::vec3(0.45f, 5.0f, 14.0f)) {}

    void SaveToFile(std::string filename);

    void LoadFromFile(std::string filename);
};

void ProgramState::SaveToFile(std::string filename) { //Ovde se cuva pozicija pre iskljucivanja aplikacije
    std::ofstream out(filename);
    out << clearColor.r << '\n'
        << clearColor.g << '\n'
        << clearColor.b << '\n'
        << ImGuiEnabled << '\n'
        << camera.Position.x << '\n'
        << camera.Position.y << '\n'
        << camera.Position.z << '\n'
        << camera.Front.x << '\n'
        << camera.Front.y << '\n'
        << camera.Front.z;
}

void ProgramState::LoadFromFile(std::string filename) { //Ovde se ucitava pozicija iz prethodnog pokretanja aplikacije
    std::ifstream in(filename);
    if (in) {
        in >> clearColor.r
           >> clearColor.g
           >> clearColor.b
           >> ImGuiEnabled
           >> camera.Position.x
           >> camera.Position.y
           >> camera.Position.z
           >> camera.Front.x
           >> camera.Front.y
           >> camera.Front.z;
    }
}

ProgramState *programState;

void DrawImGui(ProgramState *programState);


glm::vec3 koralPosition(-7.5f, -0.2f, -6.0f);
glm::vec3 pozicijaKamena(5.15f,3.0f,4.0f);

int main() {
    // glfw: initialize and configure
    // ------------------------------
    glfwInit();
    glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 3);
    glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 3);
    glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE);


#ifdef __APPLE__
    glfwWindowHint(GLFW_OPENGL_FORWARD_COMPAT, GL_TRUE);
#endif

    // glfw window creation
    // --------------------
    GLFWwindow *window = glfwCreateWindow(SCR_WIDTH, SCR_HEIGHT, "LearnOpenGL", NULL, NULL);
    if (window == NULL) {
        std::cout << "Failed to create GLFW window" << std::endl;
        glfwTerminate();
        return -1;
    }
    glfwMakeContextCurrent(window);
    glfwSetFramebufferSizeCallback(window, framebuffer_size_callback);
    glfwSetCursorPosCallback(window, mouse_callback);
    glfwSetScrollCallback(window, scroll_callback);
    glfwSetKeyCallback(window, key_callback);

    glfwSetInputMode(window, GLFW_CURSOR, GLFW_CURSOR_DISABLED);

    // glad: load all OpenGL function pointers
    // _____________________________________________________________
    if (!gladLoadGLLoader((GLADloadproc) glfwGetProcAddress)) {
        std::cout << "Failed to initialize GLAD" << std::endl;
        return -1;
    }


    programState = new ProgramState;
    programState->LoadFromFile("resources/program_state.txt");
    if (programState->ImGuiEnabled) {
        glfwSetInputMode(window, GLFW_CURSOR, GLFW_CURSOR_NORMAL);
    }
    // Inicijalizujemo komandni prozor
    IMGUI_CHECKVERSION();
    ImGui::CreateContext();
    ImGuiIO &io = ImGui::GetIO();
    (void) io;

    ImGui_ImplGlfw_InitForOpenGL(window, true);
    ImGui_ImplOpenGL3_Init("#version 330 core");

    // configure global opengl state (ovo je bitno da bi program znao sta je ispred a sta iza)
    // -----------------------------
    glEnable(GL_DEPTH_TEST);

    // build and compile shaders
    // _____________________________________________________________________________________________________
    Shader ourShader("resources/shaders/directionLight.vs", "resources/shaders/directionLight.fs");
    Shader lightSourceShader("resources/shaders/directionLight.vs", "resources/shaders/lightSourceShader.fs");
//bloom
    Shader shaderBlur("resources/shaders/blur.vs", "resources/shaders/blur.fs");
    Shader shaderBloomFinal("resources/shaders/bloom_final.vs", "resources/shaders/bloom_final.fs");

    Shader skyboxShader("resources/shaders/skybox.vs", "resources/shaders/skybox.fs");
    Shader kamen("resources/shaders/kamen.vs", "resources/shaders/kamen.fs");
    Shader blendingShader("resources/shaders/blending.vs", "resources/shaders/blending.fs");


    //postavljanje vertexa

    //kamen
    float kamenVertices[] = {
            0.5f, 0.7f, 0.5f, 0.0f, 1.0f,
            0.5f, 0.7f, -0.5f, 1.0f, 1.0f,
            -0.5f, 0.7f, -0.5f, 1.0f, 0.0f,

            0.5f, 0.7f, 0.5f, 1.0f, 0.0f,
            -0.5f, 0.7f, -0.5f, 0.0f, 1.0f,
            -0.5f, 0.7f, 0.5f, 0.0f, 0.0f,

            0.5f, -0.7f, 0.5f, 0.0f, 1.0f,
            0.5f, -0.7f, -0.5f, 1.0f, 1.0f,
            -0.5f, -0.7f, -0.5f, 1.0f, 0.0f,

            0.5f, -0.7f, 0.5f, 1.0f, 0.0f,
            -0.5f, -0.7f, -0.5f, 0.0f, 1.0f,
            -0.5f, -0.7f, 0.5f, 0.0f, 0.0f,

            1.0f, 0.0f, -1.0f, 0.0f, 1.0f,
            0.5f, 0.7f, -0.5f, 1.0f, 1.0f,
            0.5f, 0.7f, 0.5f, 1.0f, 0.0f,

            1.0f, 0.0f, -1.0f, 1.0f, 0.0f,
            0.5f, 0.7f, 0.5f, 0.0f, 1.0f,
            1.0f, 0.0f, 1.0f, 0.0f, 0.0f,

            1.0f, 0.0f, -1.0f, 0.0f, 1.0f,
            0.5f, -0.7f, -0.5f, 1.0f, 1.0f,
            0.5f, -0.7f, 0.5f, 1.0f, 0.0f,

            1.0f, 0.0f, -1.0f, 1.0f, 0.0f,
            0.5f, -0.7f, 0.5f, 0.0f, 1.0f,
            1.0f, 0.0f, 1.0f, 0.0f, 0.0f,

            1.0f, 0.0f, 1.0f, 0.0f, 1.0f,
            0.5f, 0.7f, 0.5f, 1.0f, 1.0f,
            -0.5f, 0.7f, 0.5f, 1.0f, 0.0f,

            1.0f, 0.0f, 1.0f, 1.0f, 0.0f,
            -0.5f, 0.7f, 0.5f, 0.0f, 1.0f,
            -1.0f, 0.0f, 1.0f, 0.0f, 0.0f,

            1.0f, 0.0f, 1.0f, 0.0f, 1.0f,
            0.5f, -0.7f, 0.5f, 1.0f, 1.0f,
            -0.5f, -0.7f, 0.5f, 1.0f, 0.0f,

            1.0f, 0.0f, 1.0f, 1.0f, 0.0f,
            -0.5f, -0.7f, 0.5f, 0.0f, 1.0f,
            -1.0f, 0.0f, 1.0f, 0.0f, 0.0f,

            -1.0f, 0.0f, 1.0f, 0.0f, 1.0f,
            -0.5f, 0.7f, 0.5f, 1.0f, 1.0f,
            -0.5f, 0.7f, -0.5f, 1.0f, 0.0f,

            -1.0f, 0.0f, 1.0f, 1.0f, 0.0f,
            -0.5f, 0.7f, -0.5f, 0.0f, 1.0f,
            -1.0f, 0.0f, -1.0f, 0.0f, 0.0f,

            -1.0f, 0.0f, 1.0f, 0.0f, 1.0f,
            -0.5f, -0.7f, 0.5f, 1.0f, 1.0f,
            -0.5f, -0.7f, -0.5f, 1.0f, 0.0f,

            -1.0f, 0.0f, 1.0f, 1.0f, 0.0f,
            -0.5f, -0.7f, -0.5f, 0.0f, 1.0f,
            -1.0f, 0.0f, -1.0f, 0.0f, 0.0f,

            -1.0f, 0.0f, -1.0f, 0.0f, 1.0f,
            -0.5f, 0.7f, -0.5f, 1.0f, 1.0f,
            0.5f, 0.7f, -0.5f, 1.0f, 0.0f,

            -1.0f, 0.0f, -1.0f, 1.0f, 0.0f,
            0.5f, 0.7f, -0.5f, 0.0f, 1.0f,
            1.0f, 0.0f, -1.0f, 0.0f, 0.0f,

            -1.0f, 0.0f, -1.0f, 0.0f, 1.0f,
            -0.5f, -0.7f, -0.5f, 1.0f, 1.0f,
            0.5f, -0.7f, -0.5f, 1.0f, 0.0f,

            -1.0f, 0.0f, -1.0f, 1.0f, 0.0f,
            0.5f, -0.7f, -0.5f, 0.0f, 1.0f,
            1.0f, 0.0f, -1.0f, 0.0f, 0.0f,
    };


    //pod
    // floor plain coordinates
    float floorVertices[] = {
            // positions          // normals          // texture coords
            0.5f,  0.5f,  0.0f,  0.0f, 0.0f, -1.0f,  1.0f,  1.0f,  // top right
            0.5f, -0.5f,  0.0f,  0.0f, 0.0f, -1.0f,  1.0f,  0.0f,  // bottom right
            -0.5f, -0.5f,  0.0f,  0.0f, 0.0f, -1.0f,  0.0f,  0.0f,  // bottom left
            -0.5f,  0.5f,  0.0f,  0.0f, 0.0f, -1.0f,  0.0f,  1.0f   // top left
    };

    // floor vertices for use in EBO
    unsigned int floorIndices[] = {
            0, 1, 3,  // first Triangle
            1, 2, 3   // second Triangle
    };


    //Seting skybox vertices
    //____________________________________________________________________________________________
    float skyboxVertices[] = {
            // positions
            -1.0f,  1.0f, -1.0f,
            -1.0f, -1.0f, -1.0f,
            1.0f, -1.0f, -1.0f,
            1.0f, -1.0f, -1.0f,
            1.0f,  1.0f, -1.0f,
            -1.0f,  1.0f, -1.0f,

            -1.0f, -1.0f,  1.0f,
            -1.0f, -1.0f, -1.0f,
            -1.0f,  1.0f, -1.0f,
            -1.0f,  1.0f, -1.0f,
            -1.0f,  1.0f,  1.0f,
            -1.0f, -1.0f,  1.0f,

            1.0f, -1.0f, -1.0f,
            1.0f, -1.0f,  1.0f,
            1.0f,  1.0f,  1.0f,
            1.0f,  1.0f,  1.0f,
            1.0f,  1.0f, -1.0f,
            1.0f, -1.0f, -1.0f,

            -1.0f, -1.0f,  1.0f,
            -1.0f,  1.0f,  1.0f,
            1.0f,  1.0f,  1.0f,
            1.0f,  1.0f,  1.0f,
            1.0f, -1.0f,  1.0f,
            -1.0f, -1.0f,  1.0f,

            -1.0f,  1.0f, -1.0f,
            1.0f,  1.0f, -1.0f,
            1.0f,  1.0f,  1.0f,
            1.0f,  1.0f,  1.0f,
            -1.0f,  1.0f,  1.0f,
            -1.0f,  1.0f, -1.0f,

            -1.0f, -1.0f, -1.0f,
            -1.0f, -1.0f,  1.0f,
            1.0f, -1.0f, -1.0f,
            1.0f, -1.0f, -1.0f,
            -1.0f, -1.0f,  1.0f,
            1.0f, -1.0f,  1.0f
    };

    float transparentVertices[] = {

            0.0f,  0.5f,  0.0f,  0.0f,  0.0f,
            0.0f, -0.5f,  0.0f,  0.0f,  1.0f,
            1.0f, -0.5f,  0.0f,  1.0f,  1.0f,

            0.0f,  0.5f,  0.0f,  0.0f,  0.0f,
            1.0f, -0.5f,  0.0f,  1.0f,  1.0f,
            1.0f,  0.5f,  0.0f,  1.0f,  0.0f
    };

    //Kamen setup
    unsigned int kamenVAO, kamenVBO;
    glGenVertexArrays(1, &kamenVAO);
    glGenBuffers(1, &kamenVBO);

    glBindVertexArray(kamenVAO);
    glBindBuffer(GL_ARRAY_BUFFER, kamenVBO);
    glBufferData(GL_ARRAY_BUFFER, sizeof(kamenVertices), kamenVertices, GL_STATIC_DRAW);

    glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 5 * sizeof(float), (void*)0);
    glEnableVertexAttribArray(0);

    glVertexAttribPointer(1, 2, GL_FLOAT, GL_FALSE, 5 * sizeof(float), (void*)(3 * sizeof(float)));
    glEnableVertexAttribArray(1);

    //POD
    //pod setup
    unsigned int floorVAO, floorVBO, floorEBO;
    glGenVertexArrays(1, &floorVAO);
    glGenBuffers(1, &floorVBO);
    glGenBuffers(1, &floorEBO);

    glBindVertexArray(floorVAO);
    glBindBuffer(GL_ARRAY_BUFFER, floorVBO);
    glBufferData(GL_ARRAY_BUFFER, sizeof(floorVertices), floorVertices, GL_STATIC_DRAW);
    glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, floorEBO);
    glBufferData(GL_ELEMENT_ARRAY_BUFFER, sizeof(floorIndices), floorIndices, GL_STATIC_DRAW);

    glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 8 * sizeof(float), nullptr);
    glEnableVertexAttribArray(0);
    glVertexAttribPointer(1, 3, GL_FLOAT, GL_FALSE, 8 * sizeof(float), (void*)(3*sizeof(float)));
    glEnableVertexAttribArray(1);
    glVertexAttribPointer(2, 2, GL_FLOAT, GL_FALSE, 8 * sizeof(float), (void*)(6*sizeof(float)));
    glEnableVertexAttribArray(2);

    //blending
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



    // skybox VAO
    //_______________________________________________________________________________________________
    unsigned int skyboxVAO, skyboxVBO;
    glGenVertexArrays(1, &skyboxVAO);
    glGenBuffers(1, &skyboxVBO);
    glBindVertexArray(skyboxVAO);
    glBindBuffer(GL_ARRAY_BUFFER, skyboxVBO);
    glBufferData(GL_ARRAY_BUFFER, sizeof(skyboxVertices), &skyboxVertices, GL_STATIC_DRAW);
    glEnableVertexAttribArray(0);
    glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 3 * sizeof(float), (void*)0);

    // Kamen tekstura
    unsigned int kamenTekstura = loadTexture(FileSystem::getPath("resources/textures/TexturesCom_MarbleBase0121_1_seamless_S.png").c_str());
    kamen.use();
    kamen.setInt("tekstura", 0);


    // pod tekstura
    unsigned int floorDiffuseMap = loadTexture(FileSystem::getPath("resources/textures/TexturesCom_Snow0192_6_seamless_S.jpg").c_str());
    unsigned int floorSpecularMap = loadTexture(FileSystem::getPath("resources/textures/TexturesCom_Snow0192_6_seamless_S.jpg").c_str());

    //blending
    unsigned int transparentTexture = loadTexture(FileSystem::getPath("resources/textures/fish.png").c_str());
    blendingShader.use();
    blendingShader.setInt("texture1", 0);

    // load textures for skybox
    programState->faces =
            {
                    FileSystem::getPath("resources/textures/underwater/uw_rt.jpg"),
                    FileSystem::getPath("resources/textures/underwater/uw_lf.jpg"),
                    FileSystem::getPath("resources/textures/underwater/uw_up.jpg"),
                    FileSystem::getPath("resources/textures/underwater/uw_dn.jpg"),
                    FileSystem::getPath("resources/textures/underwater/uw_bk.jpg"),
                    FileSystem::getPath("resources/textures/underwater/uw_ft.jpg")
            };

    programState->cubemapTexture = loadCubemap(programState->faces);

    skyboxShader.use();
    skyboxShader.setInt("skybox", 0);

    //load models

    //hrist
    Model hristModel("resources/objects/christusfigur/source/Jesus_C.obj");
    hristModel.SetShaderTextureNamePrefix("material.");
    //andjeo
    Model engelModel("resources/objects/engel/source/Engel_C.obj");
    engelModel.SetShaderTextureNamePrefix("material.");
//pavilion2--> ODLICAN
//    Model ourModel("resources/objects/pavilion2/MPM34X49LMKPH0U4GJX5J4HM5.obj");
//    ourModel.SetShaderTextureNamePrefix("material.");
    //pavilion
    Model ourModel("resources/objects/pavilion_obj/3UEVSMJPECNQWYGX8TJA3HP6L.obj");
    ourModel.SetShaderTextureNamePrefix("material.");
    //maria immaculata
    Model mariaModel("resources/objects/engel(3)/source/Engel_C.obj");
    mariaModel.SetShaderTextureNamePrefix("material.");
    //postolje za kamen
    Model platformModel("resources/objects/platforma_obj/KT06ZA8NPCLRPYON8BOA8WF0N.obj");
    platformModel.SetShaderTextureNamePrefix("material.");


    //BLOOM
    //Bloom efekat _____________________________________________________________________________________________
    // configure framebuffers
    unsigned int hdrFBO;
    glGenFramebuffers(1, &hdrFBO);
    glBindFramebuffer(GL_FRAMEBUFFER, hdrFBO);
    // create 2 color buffers (1 for normal rendering, other for brightness threshold values)
    unsigned int colorBuffers[2];
    glGenTextures(2, colorBuffers);
    for (unsigned int i = 0; i < 2; i++)
    {
        glBindTexture(GL_TEXTURE_2D, colorBuffers[i]);
        glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA16F, SCR_WIDTH, SCR_HEIGHT, 0, GL_RGBA, GL_FLOAT, NULL);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);  // we clamp to the edge as the blur filter would otherwise sample repeated texture values!
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
        // attach texture to framebuffer
        glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0 + i, GL_TEXTURE_2D, colorBuffers[i], 0);
    }
    // create and attach depth buffer (renderbuffer)
    unsigned int rboDepth;
    glGenRenderbuffers(1, &rboDepth);
    glBindRenderbuffer(GL_RENDERBUFFER, rboDepth);
    glRenderbufferStorage(GL_RENDERBUFFER, GL_DEPTH_COMPONENT, SCR_WIDTH, SCR_HEIGHT);
    glFramebufferRenderbuffer(GL_FRAMEBUFFER, GL_DEPTH_ATTACHMENT, GL_RENDERBUFFER, rboDepth);
    // tell OpenGL which color attachments we'll use (of this framebuffer) for rendering
    glBindFramebuffer(GL_FRAMEBUFFER, hdrFBO);
    unsigned int attachments[2] = { GL_COLOR_ATTACHMENT0, GL_COLOR_ATTACHMENT1 };
    glDrawBuffers(2, attachments);
    glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_TEXTURE_2D, colorBuffers[0], 0);
    glFramebufferRenderbuffer(GL_FRAMEBUFFER, GL_DEPTH_ATTACHMENT, GL_RENDERBUFFER, rboDepth);

    // check if framebuffer is complete
    if (glCheckFramebufferStatus(GL_FRAMEBUFFER) != GL_FRAMEBUFFER_COMPLETE)
        std::cout << "Framebuffer not complete!" << std::endl;
    glBindFramebuffer(GL_FRAMEBUFFER, 0);

    // ping-pong-framebuffer for blurring
    unsigned int pingpongFBO[2];
    unsigned int pingpongColorbuffers[2];
    glGenFramebuffers(2, pingpongFBO);
    glGenTextures(2, pingpongColorbuffers);
    for (unsigned int i = 0; i < 2; i++)
    {
        glBindFramebuffer(GL_FRAMEBUFFER, pingpongFBO[i]);
        glBindTexture(GL_TEXTURE_2D, pingpongColorbuffers[i]);
        glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA16F, SCR_WIDTH, SCR_HEIGHT, 0, GL_RGBA, GL_FLOAT, NULL);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE); // we clamp to the edge as the blur filter would otherwise sample repeated texture values!
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
        glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_TEXTURE_2D, pingpongColorbuffers[i], 0);
        // also check if framebuffers are complete (no need for depth buffer)
        if (glCheckFramebufferStatus(GL_FRAMEBUFFER) != GL_FRAMEBUFFER_COMPLETE)
            std::cout << "Framebuffer not complete!" << std::endl;
    }



    //Point light
    PointLight& pointLight = programState->pointLight;
    pointLight.position = glm::vec3(-5.6f, 5.0f, 1.7f);
    pointLight.ambient = glm::vec3(0.1, 0.1, 0.1);
    pointLight.diffuse = glm::vec3(0.6, 0.6, 0.6);
    pointLight.specular = glm::vec3(1.0, 1.0, 1.0);

    pointLight.constant = 0.3f;
    pointLight.linear = 0.8f;
    pointLight.quadratic = 0.4f;

    // shader configuration
    ourShader.use();
    ourShader.setInt("diffuseTexture", 0);

    //bloom
    shaderBlur.use();
    shaderBlur.setInt("image", 0);
    shaderBloomFinal.use();
    shaderBloomFinal.setInt("scene", 0);
    shaderBloomFinal.setInt("bloomBlur", 1);

    // render loop
    // ____________________________________________________________________________________________________
    while (!glfwWindowShouldClose(window)) {

        float currentFrame = glfwGetTime();
        deltaTime = currentFrame - lastFrame;
        lastFrame = currentFrame;
        float time = currentFrame;

        // input
        // ____________________________________________________________________________________
        if(freeMove) {
            processInput(window);
        }else{
            processInput1(window);
            programState->camera.Position.y = 0.7f;
        }

        // render
        // ____________________________________________________________________________________
        glClearColor(programState->clearColor.r, programState->clearColor.g, programState->clearColor.b, 1.0f);
        glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

        //bloom
        //render scene into floating point framebuffer
        glBindFramebuffer(GL_FRAMEBUFFER, hdrFBO);
        glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

        // enable shader before setting uniforms
        ourShader.use();
        // view/projection transformations
        glm::mat4 projection = glm::perspective(glm::radians(programState->camera.Zoom),
                                                (float) SCR_WIDTH / (float) SCR_HEIGHT, 0.1f, 100.0f);
        glm::mat4 view = programState->camera.GetViewMatrix();
        ourShader.setMat4("projection", projection);
        ourShader.setMat4("view", view);

        ourShader.setVec3("viewPosition", programState->camera.Position);
        ourShader.setFloat("material.shininess", 32.0f);
        // directional light
        ourShader.setVec3("dirLight.direction", programState->dirLightDir);
        ourShader.setVec3("dirLight.ambient", glm::vec3(programState->dirLightAmbDiffSpec.x));
        ourShader.setVec3("dirLight.diffuse", glm::vec3(programState->dirLightAmbDiffSpec.y));
        ourShader.setVec3("dirLight.specular", glm::vec3(programState->dirLightAmbDiffSpec.z));

//        ourShader.setVec3("pointLights[0].position", glm::vec3(-0.8f ,0.05f, 2.7f));
        ourShader.setVec3("pointLights[0].position", pozicijaKamena);
        ourShader.setVec3("pointLights[0].ambient", pointLight.ambient);
        ourShader.setVec3("pointLights[0].diffuse", pointLight.diffuse);
        ourShader.setVec3("pointLights[0].specular", pointLight.specular);
        ourShader.setFloat("pointLights[0].constant", pointLight.constant);
        ourShader.setFloat("pointLights[0].linear", pointLight.linear);
        ourShader.setFloat("pointLights[0].quadratic", pointLight.quadratic);

//        ourShader.setVec3("pointLights[1].position", glm::vec3(-1.2f ,0.3f, -0.05f));
        ourShader.setVec3("pointLights[1].position", pozicijaKamena);
        ourShader.setVec3("pointLights[1].ambient", pointLight.ambient);
        ourShader.setVec3("pointLights[1].diffuse", pointLight.diffuse);
        ourShader.setVec3("pointLights[1].specular", pointLight.specular);
        ourShader.setFloat("pointLights[1].constant", pointLight.constant);
        ourShader.setFloat("pointLights[1].linear", pointLight.linear);
        ourShader.setFloat("pointLights[1].quadratic", pointLight.quadratic);


        // spotLight
        if (spotlightOn) {
            ourShader.setVec3("spotLight.position", programState->camera.Position);
            ourShader.setVec3("spotLight.direction", programState->camera.Front);
            ourShader.setVec3("spotLight.ambient", 0.0f, 0.0f, 0.0f);
            ourShader.setVec3("spotLight.diffuse", 1.0f, 1.0f, 1.0f);
            ourShader.setVec3("spotLight.specular", 1.0f, 1.0f, 1.0f);
            ourShader.setFloat("spotLight.constant", 1.0f);
            ourShader.setFloat("spotLight.linear", 0.09);
            ourShader.setFloat("spotLight.quadratic", 0.032);
            ourShader.setFloat("spotLight.cutOff", glm::cos(glm::radians(12.5f)));
            ourShader.setFloat("spotLight.outerCutOff", glm::cos(glm::radians(15.0f)));
        }else{
            ourShader.setVec3("spotLight.diffuse", 0.0f, 0.0f, 0.0f);
            ourShader.setVec3("spotLight.specular", 0.0f, 0.0f, 0.0f);
        }


        // renderovanje ucitanih modela

//engel
//za pavilion1 translate x-->6.0
        glm::mat4 modelEngel = glm::mat4(1.0f);
        modelEngel = glm::translate(modelEngel,
                               glm::vec3(7.5f, -0.2f, -6.0f)); // translate it down so it's at the center of the scene
        modelEngel = glm::rotate(modelEngel,  glm::radians(180.0f) ,glm::vec3(0.0f, 1.0f, 0.0f));
        modelEngel = glm::scale(modelEngel, glm::vec3(1.0f));    // it's a bit too big for our scene, so scale it down
        ourShader.setMat4("model", modelEngel);
        engelModel.Draw(ourShader);
//hrist
//promena
         glm::mat4 model = glm::mat4(1.0f);
//        model = glm::translate(model,
//                               glm::vec3 (-6.0f, 0.0f, 7.6f)); // translate it down so it's at the center of the scene
        model = glm::translate(model,
                               glm::vec3 (0.0f, 0.45f, -5.0f)); // translate it down so it's at the center of the scene
        model = glm::rotate(model,  glm::radians(180.0f) ,glm::vec3(0.0f, 1.0f, 0.0f));
        model = glm::scale(model, glm::vec3(0.4f));    // it's a bit too big for our scene, so scale it down
        ourShader.setMat4("model", model);
        hristModel.Draw(ourShader);
//maria immaculata
        model = glm::mat4(1.0f);
        model = glm::translate(model,
                               koralPosition); // translate it down so it's at the center of the scene
        model = glm::rotate(model,  glm::radians(180.0f) ,glm::vec3(0.0f, 1.0f, 0.0f));
        model = glm::scale(model, glm::vec3(0.175f));    // it's a bit too big for our scene, so scale it down
        ourShader.setMat4("model", model);
        mariaModel.Draw(ourShader);

        //ZA POSTOLJE
//        model = glm::mat4(1.0f);
//        model = glm::translate(model, glm::vec3(0.0f, 5.5f, -6.0f));
//        //model = glm::rotate(model, glm::radians(ugao), glm::vec3(0.0f, 1.0f, 0.0f));
//        model = glm::scale(model, glm::vec3(5.0f));
//        ourShader.setMat4("model", model);
//        ourModel.Draw(ourShader);
        //postolje2
        model = glm::mat4(1.0f);
        model = glm::translate(model, glm::vec3(0.0f, 5.5f, -6.0f));
        model = glm::rotate(model, glm::radians(90.0f), glm::vec3(0.0f, 1.0f, 0.0f));
        model = glm::scale(model, glm::vec3(5.0f));
        ourShader.setMat4("model", model);
        ourModel.Draw(ourShader);

        //platforma
        glm::mat4 modelPlatform = glm::mat4(1.0f);
        modelPlatform = glm::translate(modelPlatform,
                                       glm::vec3 (5.15f,0.1f,4.0f)); // translate it down so it's at the center of the scene
        modelPlatform = glm::rotate(modelPlatform,  glm::radians(180.0f) ,glm::vec3(0.0f, 1.0f, 0.0f));
        modelPlatform = glm::scale(modelPlatform, glm::vec3(1.0f));    // it's a bit too big for our scene, so scale it down
        ourShader.setMat4("model", modelPlatform);
        platformModel.Draw(ourShader);

  //      glBindFramebuffer(GL_FRAMEBUFFER, 0);


        //world transformation
        model = glm::mat4(1.0f);
        model = glm::translate(model, glm::vec3(0.0f, -0.0f, 0.0f));
        model = glm::rotate(model, glm::radians(90.0f), glm::vec3(1.0f, 0.0f, 0.0f));
        model = glm::scale(model, glm::vec3(25.0f));
        ourShader.setMat4("model", model);

        // bind diffuse map
        glActiveTexture(GL_TEXTURE0);
        glBindTexture(GL_TEXTURE_2D, floorDiffuseMap);

        // bind specular map
        glActiveTexture(GL_TEXTURE1);
        glBindTexture(GL_TEXTURE_2D, floorSpecularMap);

        // render pod
        glBindVertexArray(floorVAO);
        glEnable(GL_CULL_FACE);     // floor won't be visible if looked from bellow
        glCullFace(GL_BACK);
        glDrawElements(GL_TRIANGLES, 6, GL_UNSIGNED_INT, nullptr);
        glDisable(GL_CULL_FACE);


        // render Kamen
        kamen.use();
        kamen.setMat4("projection", projection);
        kamen.setMat4("view", view);
        model = glm::mat4(1.0f);
        model = glm::translate(model, pozicijaKamena);
        model = glm::scale(model, glm::vec3(0.4f)); //OVDE
        kamen.setMat4("model", model);

        glActiveTexture(GL_TEXTURE0);
        glBindTexture(GL_TEXTURE_2D, kamenTekstura);
        glBindVertexArray(kamenVAO);
        glDrawArrays(GL_TRIANGLES, 0, 64);

        //blending RIBICE
        blendingShader.use();
        blendingShader.setMat4("projection", projection);
        blendingShader.setMat4("view", view);

        int n=50;
        for(int i=0;i<=n;i++){
            glBindVertexArray(transparentVAO);
            glActiveTexture(GL_TEXTURE0);
            glBindTexture(GL_TEXTURE_2D, transparentTexture);
            model = glm::mat4(1.0f);
            model = glm::translate(model, glm::vec3((17.5+i*3) * cos(currentFrame *i/ 2)+sin(currentFrame), sin(currentFrame*i/2), (17.5+i*3) * sin(currentFrame / 2)) + cos(i*currentFrame));
            model = glm::scale(model, glm::vec3(0.5f));
            blendingShader.setMat4("model", model);
            glDrawArrays(GL_TRIANGLES, 0, 6);

        }

        int m=30;
        for(int i=0;i<=m;i++){
            glBindVertexArray(transparentVAO);
            glActiveTexture(GL_TEXTURE0);
            glBindTexture(GL_TEXTURE_2D, transparentTexture);
            model = glm::mat4(1.0f);
            model = glm::translate(model, glm::vec3(-(17.5+i*3) * cos(currentFrame *i/ 2)+sin(currentFrame), sin(currentFrame*i/2), -(17.5+i*3) * sin(currentFrame / 2)) + cos(i*currentFrame));
            model = glm::scale(model, glm::vec3(0.5f));
            blendingShader.setMat4("model", model);
            glDrawArrays(GL_TRIANGLES, 0, 6);

        }

        glm::vec3 pozicija2 = glm::vec3(15 * cos(currentFrame / 2), 1.0f, 15 * sin(currentFrame / 2));

        //face culling + blending
        glEnable(GL_CULL_FACE);
        glCullFace(GL_BACK);
        model = glm::mat4(1.0f);
//        model = glm::translate(model, glm::vec3(-3.0f, 2.0f, 4.0f));
        model = glm::translate(model, pozicija2);
        model = glm::rotate(model, glm::radians(180.0f), glm::vec3(0.0f, 1.0f, 0.0f));
        model = glm::scale(model, glm::vec3(0.5f));
        blendingShader.setMat4("model", model);
        glDrawArrays(GL_TRIANGLES, 0, 6);

        glDisable(GL_CULL_FACE);



        // draw skybox
        glDepthFunc(GL_LEQUAL);
        skyboxShader.use();
        view = glm::mat4(glm::mat3(programState->camera.GetViewMatrix()));
        skyboxShader.setMat4("view", view);
        skyboxShader.setMat4("projection", projection);
        // skybox cube
        glBindVertexArray(skyboxVAO);
        glActiveTexture(GL_TEXTURE0);
        glBindTexture(GL_TEXTURE_CUBE_MAP, programState->cubemapTexture);
        glDrawArrays(GL_TRIANGLES, 0, 36);
        glBindVertexArray(0);
        glDepthFunc(GL_LESS); // set depth function back to default

        // blur bright fragments with two-pass Gaussian Blur
        bool horizontal = true, first_iteration = true;
        unsigned int amount = 5;
        shaderBlur.use();
        for (unsigned int i = 0; i < amount; i++)
        {
            glBindFramebuffer(GL_FRAMEBUFFER, pingpongFBO[horizontal]);
            shaderBlur.setInt("horizontal", horizontal);
            glBindTexture(GL_TEXTURE_2D, first_iteration ? colorBuffers[1] : pingpongColorbuffers[!horizontal]);  // bind texture of other framebuffer (or scene if first iteration)
            renderQuad();
            horizontal = !horizontal;
            if (first_iteration)
                first_iteration = false;
        }
        glBindFramebuffer(GL_FRAMEBUFFER, 0);

        // now render floating point color buffer to 2D quad and tonemap HDR colors to default framebuffer's (clamped) color range
        glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
        shaderBloomFinal.use();
        glActiveTexture(GL_TEXTURE0);
        glBindTexture(GL_TEXTURE_2D, colorBuffers[0]);
        glActiveTexture(GL_TEXTURE1);
        glBindTexture(GL_TEXTURE_2D, pingpongColorbuffers[!horizontal]);
        shaderBloomFinal.setInt("bloom", bloom);
        shaderBloomFinal.setFloat("exposure", exposure);
        renderQuad();

        if (programState->ImGuiEnabled)
            DrawImGui(programState);



        // glfw: swap buffers and poll IO events (keys pressed/released, mouse moved etc.)
        // _______________________________________________________________________________________
        glfwSwapBuffers(window);
        glfwPollEvents();
    }

    glDeleteVertexArrays(1, &kamenVAO);
    glDeleteVertexArrays(1, &floorVAO);
    glDeleteVertexArrays(1, &skyboxVAO);
    glDeleteBuffers(1, &kamenVBO);
    glDeleteBuffers(1, &floorVBO);
    glDeleteBuffers(1, &floorEBO);
    glDeleteBuffers(1, &skyboxVBO);

    programState->SaveToFile("resources/program_state.txt");
    delete programState;
    ImGui_ImplOpenGL3_Shutdown();
    ImGui_ImplGlfw_Shutdown();
    ImGui::DestroyContext();

    // glfw: terminate, clearing all previously allocated GLFW resources.
    // _______________________________________________________________________________________
    glfwTerminate();
    return 0;
}

//bloom
// renderQuad() renders a 1x1 XY quad in NDC
unsigned int quadVAO = 0;
unsigned int quadVBO;
void renderQuad()
{
    if (quadVAO == 0)
    {
        float quadVertices[] = {
                // positions        // texture Coords
                -1.0f,  1.0f, 0.0f, 0.0f, 1.0f,
                -1.0f, -1.0f, 0.0f, 0.0f, 0.0f,
                1.0f,  1.0f, 0.0f, 1.0f, 1.0f,
                1.0f, -1.0f, 0.0f, 1.0f, 0.0f,
        };
        // setup plane VAO
        glGenVertexArrays(1, &quadVAO);
        glGenBuffers(1, &quadVBO);
        glBindVertexArray(quadVAO);
        glBindBuffer(GL_ARRAY_BUFFER, quadVBO);
        glBufferData(GL_ARRAY_BUFFER, sizeof(quadVertices), &quadVertices, GL_STATIC_DRAW);
        glEnableVertexAttribArray(0);
        glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 5 * sizeof(float), (void*)0);
        glEnableVertexAttribArray(1);
        glVertexAttribPointer(1, 2, GL_FLOAT, GL_FALSE, 5 * sizeof(float), (void*)(3 * sizeof(float)));
    }
    glBindVertexArray(quadVAO);
    glDrawArrays(GL_TRIANGLE_STRIP, 0, 4);
    glBindVertexArray(0);
}



bool circleColision(glm::vec3 c,glm::vec3 x,float r){
    float res;
    res = pow(x.x-c.x,2.0f)+pow(x.z-c.z,2.0f) - pow(r,2.0f);
    if(res >= 0)
        return true;
    else return false;
}

// process all input
// _______________________________________________________________________________________________
void processInput(GLFWwindow *window) {

    float velocity = 2.5f * deltaTime;
    glm::vec3 yLock(1.0f, 0.0f, 1.0f);
    glm::vec3 yMove(0.0f, 1.0f, 0.0f);

    if (glfwGetKey(window, GLFW_KEY_ESCAPE) == GLFW_PRESS)
        glfwSetWindowShouldClose(window, true);
    if (glfwGetKey(window, GLFW_KEY_W) == GLFW_PRESS)
        programState->camera.ProcessKeyboard(FORWARD, deltaTime);
    if (glfwGetKey(window, GLFW_KEY_S) == GLFW_PRESS)
        programState->camera.ProcessKeyboard(BACKWARD, deltaTime);
    if (glfwGetKey(window, GLFW_KEY_A) == GLFW_PRESS)
        programState->camera.ProcessKeyboard(LEFT, deltaTime);
    if (glfwGetKey(window, GLFW_KEY_D) == GLFW_PRESS)
        programState->camera.ProcessKeyboard(RIGHT, deltaTime);
//pomeranje kamena
    if (glfwGetKey(window, GLFW_KEY_UP) == GLFW_PRESS)
        moveRock(FORWARD);
    if (glfwGetKey(window, GLFW_KEY_DOWN) == GLFW_PRESS)
        moveRock(BACKWARD);
    if (glfwGetKey(window, GLFW_KEY_LEFT) == GLFW_PRESS)
        moveRock(LEFT);
    if (glfwGetKey(window, GLFW_KEY_RIGHT) == GLFW_PRESS)
        moveRock(RIGHT);
    if (glfwGetKey(window, GLFW_KEY_M) == GLFW_PRESS)
        pozicijaKamena += velocity * yMove;
//        moveRock(UP);

    if (glfwGetKey(window, GLFW_KEY_N) == GLFW_PRESS)
        pozicijaKamena -= velocity * yMove;
//        moveRock(DOWN);
}
void processInput1(GLFWwindow *window) {
    if (glfwGetKey(window, GLFW_KEY_ESCAPE) == GLFW_PRESS)
        glfwSetWindowShouldClose(window, true);


    //Colision - this don't allow us to go beyond the border of the land
    if (!(circleColision(glm::vec3(3.34f,0.0f,1.8f),programState->camera.Position,12.9f))) {
        lastCamPosition = programState->camera.Position;
        if (glfwGetKey(window, GLFW_KEY_W) == GLFW_PRESS)
            programState->camera.ProcessKeyboard(FORWARD, deltaTime);
        if (glfwGetKey(window, GLFW_KEY_S) == GLFW_PRESS)
            programState->camera.ProcessKeyboard(BACKWARD, deltaTime);
        if (glfwGetKey(window, GLFW_KEY_A) == GLFW_PRESS)
            programState->camera.ProcessKeyboard(LEFT, deltaTime);
        if (glfwGetKey(window, GLFW_KEY_D) == GLFW_PRESS)
            programState->camera.ProcessKeyboard(RIGHT, deltaTime);
    }else {
        programState->camera.Position = lastCamPosition;
    }


}

// glfw: whenever the window size changed (by OS or user resize) this callback function executes
// _______________________________________________________________________________________________
void framebuffer_size_callback(GLFWwindow *window, int width, int height) {
    glViewport(0, 0, width, height);
}

// glfw: whenever the mouse moves, this callback is called
// _____________________________________________________________________________________________
void mouse_callback(GLFWwindow *window, double xpos, double ypos) {
    if (firstMouse) {
        lastX = xpos;
        lastY = ypos;
        firstMouse = false;
    }

    float xoffset = xpos - lastX;
    float yoffset = lastY - ypos; // reversed since y-coordinates go from bottom to top

    lastX = xpos;
    lastY = ypos;

    if (programState->CameraMouseMovementUpdateEnabled)
        programState->camera.ProcessMouseMovement(xoffset, yoffset);
}

// glfw: whenever the mouse scroll wheel scrolls, this callback is called
void scroll_callback(GLFWwindow *window, double xoffset, double yoffset) {
    programState->camera.ProcessMouseScroll(yoffset);
}

//This is the comand window
void DrawImGui(ProgramState *programState) {
    ImGui_ImplOpenGL3_NewFrame();
    ImGui_ImplGlfw_NewFrame();
    ImGui::NewFrame();
    {
        static float f = 0.0f;
        ImGui::Begin("Settings");
        ImGui::Text("Point light settings:");

        ImGui::DragFloat("pointLight.constant", &programState->pointLight.constant, 0.005, 0.0001, 1.0);
        ImGui::DragFloat("pointLight.linear", &programState->pointLight.linear, 0.005, 0.0001, 1.0);
        ImGui::DragFloat("pointLight.quadratic", &programState->pointLight.quadratic, 0.005, 0.0001, 1.0);

        ImGui::Text("Fine translate");
        ImGui::DragFloat3("translate", (float*)&programState->vecCalibrate, 0.05, -300.0, 300.0);
        ImGui::Text("Fine rotate");
        ImGui::DragFloat3("Rotate", (float*)&programState->vecRotate, 0.05, -181.0, 181.0);
        ImGui::DragFloat("scale", &programState->fineCalibrate, 0.05, 0.00000001, 100.0);

        ImGui::Text("DirLight settings");
        ImGui::DragFloat3("Direction light direction", (float*)&programState->dirLightDir, 0.05, -20.0, 20.0);

        ImGui::Text("Ambient    Diffuse    Specular");
        ImGui::DragFloat3("Direction light settings", (float*)&programState->dirLightAmbDiffSpec, 0.05, 0.001, 1.0);

        ImGui::End();
    }

    {
        ImGui::Begin("Camera info");
        const Camera& c = programState->camera;
        ImGui::Text("Camera position: (%f, %f, %f)", c.Position.x, c.Position.y, c.Position.z);
        ImGui::Text("(Yaw, Pitch): (%f, %f)", c.Yaw, c.Pitch);
        ImGui::Text("Camera front: (%f, %f, %f)", c.Front.x, c.Front.y, c.Front.z);
        ImGui::Checkbox("Camera mouse update", &programState->CameraMouseMovementUpdateEnabled);
        ImGui::End();
    }

    ImGui::Render();
    ImGui_ImplOpenGL3_RenderDrawData(ImGui::GetDrawData());
}

void key_callback(GLFWwindow *window, int key, int scancode, int action, int mods) {
    if (key == GLFW_KEY_F1 && action == GLFW_PRESS) {
        programState->ImGuiEnabled = !programState->ImGuiEnabled;
        if (programState->ImGuiEnabled) {
            programState->CameraMouseMovementUpdateEnabled = false;
            glfwSetInputMode(window, GLFW_CURSOR, GLFW_CURSOR_NORMAL);
        } else {
            programState->CameraMouseMovementUpdateEnabled = true;
            glfwSetInputMode(window, GLFW_CURSOR, GLFW_CURSOR_DISABLED);
        }
    }
    if (key == GLFW_KEY_F && action == GLFW_PRESS){
        if(spotlightOn){
            spotlightOn = false;
        }else{
            spotlightOn = true;
        }
    }
    if (key == GLFW_KEY_F2 && action == GLFW_PRESS){
        if(freeMove){
            freeMove = false;
        }else{
            freeMove = true;
        }
    }
    if (glfwGetKey(window, GLFW_KEY_B) == GLFW_PRESS && !bloomKeyPressed)
    {
        bloom = !bloom;
        bloomKeyPressed = true;
    }
    if (glfwGetKey(window, GLFW_KEY_B) == GLFW_RELEASE)
    {
        bloomKeyPressed = false;
    }
//    if (glfwGetKey(window, GLFW_KEY_Q) == GLFW_PRESS)
//    {
//        if (exposure > 0.0f)
//            exposure -= 0.001f;
//        else
//            exposure = 0.0f;
//    }
//    else if (glfwGetKey(window, GLFW_KEY_E) == GLFW_PRESS)
//    {
//        exposure += 0.001f;
//    }

}
unsigned int loadCubemap(vector<std::string> faces)
{
    unsigned int textureID;
    glGenTextures(1, &textureID);
    glBindTexture(GL_TEXTURE_CUBE_MAP, textureID);

    int width, height, nrComponents;
    for (unsigned int i = 0; i < faces.size(); i++)
    {
        unsigned char *data = stbi_load(faces[i].c_str(), &width, &height, &nrComponents, 0);
        if (data)
        {
            glTexImage2D(GL_TEXTURE_CUBE_MAP_POSITIVE_X + i, 0, GL_RGB, width, height, 0, GL_RGB, GL_UNSIGNED_BYTE, data);
            stbi_image_free(data);
        }
        else
        {
            std::cout << "Cubemap texture failed to load at path: " << faces[i] << std::endl;
            stbi_image_free(data);
        }
    }
    glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
    glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
    glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_WRAP_R, GL_CLAMP_TO_EDGE);

    return textureID;
}
unsigned int loadTexture(char const *path)
{
    unsigned int textureID;
    glGenTextures(1, &textureID);

    int width, height, nrComponents;
    unsigned char *data = stbi_load(path, &width, &height, &nrComponents, 0);
    if (data)
    {
        GLenum format;
        if (nrComponents == 1)
            format = GL_RED;
        else if (nrComponents == 3)
            format = GL_RGB;
        else if (nrComponents == 4)
            format = GL_RGBA;

        glBindTexture(GL_TEXTURE_2D, textureID);
        glTexImage2D(GL_TEXTURE_2D, 0, format, width, height, 0, format, GL_UNSIGNED_BYTE, data);
        glGenerateMipmap(GL_TEXTURE_2D);

        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_REPEAT);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_REPEAT);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR_MIPMAP_LINEAR);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);

        stbi_image_free(data);
    }
    else
    {
        std::cout << "Texture failed to load at path: " << path << std::endl;
        stbi_image_free(data);
    }

    return textureID;
}


////POMERANJE KAMENA
void moveRock(Camera_Movement direction)
{
    float velocity = 2.5f * deltaTime;
    glm::vec3 yLock(1.0f, 0.0f, 1.0f);
    glm::vec3 yMove(0.0f, 1.0f, 0.0f);

    if (direction == FORWARD)
        pozicijaKamena += programState->camera.Front * velocity * yLock;
    if (direction == BACKWARD)
        pozicijaKamena -= programState->camera.Front * velocity * yLock;
    if (direction == LEFT)
        pozicijaKamena -= programState->camera.Right * velocity * yLock;
    if (direction == RIGHT)
        pozicijaKamena += programState->camera.Right * velocity * yLock;

//    if (direction == UP)
//        pozicijaKamena += velocity * yMove;
//    if (direction == DOWN)
//        pozicijaKamena -= velocity * yMove;

    if (pozicijaKamena.y < 0.0f)
        pozicijaKamena.y = 0.0f;
    else if (pozicijaKamena.y > 3.0f)
        pozicijaKamena.y = 3.0f;

}

#define STB_IMAGE_IMPLEMENTATION

#include <glad/glad.h>
#include <GLFW/glfw3.h>

#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtc/type_ptr.hpp>

#include "imgui.h"
#include "imgui_impl_glfw.h"
#include "imgui_impl_opengl3.h"
#include "imfilebrowser.h"

#include "Camera.h"
#include "Model.h"

#include <iostream>

enum progState {
    MENU,
    ACTIVE
};

struct modelInfo {
    glm::vec3 pos;
    glm::vec3 scale;
};

string convertPath(const std::string& str);

void framebuffer_size_callback(GLFWwindow* window, int width, int height);
void mouse_callback(GLFWwindow* window, double xpos, double ypos);
void scroll_callback(GLFWwindow* window, double xoffset, double yoffset);
void processInput(GLFWwindow* window);
void SetnDrawModel(Shader& shader, Model& modelObj, bool moveable, glm::vec3 scale, glm::vec3 pos);
void MenuDraw();
pair<Model, modelInfo> LoadModel(string pathToModel,bool moveable, float pos[3], float scale);

// settings
const unsigned int SCR_WIDTH = 1280;
const unsigned int SCR_HEIGHT = 720;

// camera
Camera camera(glm::vec3(0.0f, 0.0f, 3.0f));
float lastX = SCR_WIDTH / 2.0f;
float lastY = SCR_HEIGHT / 2.0f;
bool firstMouse = true;

// timing
float deltaTime = 0.0f;
float lastFrame = 0.0f;

// model transfrom
float rotAngle = 0.0f;
glm::vec3 moveVec = glm::vec3(0.0f, 0.0f, 0.0f);
glm::vec3 speedVecZ = glm::vec3(0.0f, 0.0f, 4.0f);

// default vectors
const glm::vec3 zeroVec = glm::vec3(0.0f, 0.0f, 0.0f);
const glm::vec3 singleVec = glm::vec3(1.0f, 1.0f, 1.0f);

// tools, objects
vector<pair<Model, modelInfo>> models;
progState state = MENU;
bool KeysProcessed[1024], Keys[1024];

int main()
{
    glfwInit();
    glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 3);
    glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 3);
    glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE);

    GLFWwindow* window = glfwCreateWindow(SCR_WIDTH, SCR_HEIGHT, "ModelViewer", NULL, NULL);
    if (window == NULL)
    {
        std::cout << "Failed to create GLFW window" << std::endl;
        glfwTerminate();
        return -1;
    }
    glfwMakeContextCurrent(window);

    IMGUI_CHECKVERSION();
    ImGui::CreateContext();
    ImGuiIO& io = ImGui::GetIO(); (void)io;
    io.ConfigFlags |= ImGuiConfigFlags_NavEnableKeyboard;     // Enable Keyboard Controls
    io.ConfigFlags |= ImGuiConfigFlags_NavEnableGamepad;      // Enable Gamepad Controls
    io.ConfigFlags |= ImGuiConfigFlags_DockingEnable;         // Enable Docking
    io.ConfigFlags |= ImGuiConfigFlags_ViewportsEnable;       // Enable Multi-Viewport / Platform Windows

    glfwSetFramebufferSizeCallback(window, framebuffer_size_callback);
    glfwSetCursorPosCallback(window, mouse_callback);
    glfwSetScrollCallback(window, scroll_callback);

    ImGui::StyleColorsDark();

    // When viewports are enabled we tweak WindowRounding/WindowBg so platform windows can look identical to regular ones.
    ImGuiStyle& style = ImGui::GetStyle();
    if (io.ConfigFlags & ImGuiConfigFlags_ViewportsEnable)
    {
        style.WindowRounding = 0.0f;
        style.Colors[ImGuiCol_WindowBg].w = 1.0f;
    }

    // Setup Platform/Renderer backends
    ImGui_ImplGlfw_InitForOpenGL(window, true);
    ImGui_ImplOpenGL3_Init("#version 330");

    gladLoadGL();

    // stbi_set_flip_vertically_on_load(true);
    
    glEnable(GL_DEPTH_TEST);

    Shader ourShader("vShader.vx", "fShader.ft");

    // ------------------------- MAIN LOOP STARTED -------------------------

    while (!glfwWindowShouldClose(window)) 
    {
        glClearColor(0.15f, 0.15f, 0.15f, 1.0f);
        glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

        float currentFrame = static_cast<float>(glfwGetTime());
        deltaTime = currentFrame - lastFrame;
        lastFrame = currentFrame;

        // input
        processInput(window);

        // drawing
        for (int i = 0; i < models.size(); ++i) {
            SetnDrawModel(ourShader, models[i].first, models[i].first.IsMoveable(), models[i].second.scale, models[i].second.pos); // first - model object; second - modelInfo struct  
        }

        if (state == MENU) {
            glfwSetInputMode(window, GLFW_CURSOR, GLFW_CURSOR_NORMAL);

            MenuDraw();

            ImGui::Render();
            ImGui_ImplOpenGL3_RenderDrawData(ImGui::GetDrawData());

            if (io.ConfigFlags & ImGuiConfigFlags_ViewportsEnable)
            {
                GLFWwindow* backup_current_context = glfwGetCurrentContext();
                ImGui::UpdatePlatformWindows();
                ImGui::RenderPlatformWindowsDefault();
                glfwMakeContextCurrent(backup_current_context);
            }
        }
        else glfwSetInputMode(window, GLFW_CURSOR, GLFW_CURSOR_DISABLED);
        
        // glfw: swap buffers and poll IO events (keys pressed/released, mouse moved etc.)
        glfwSwapBuffers(window);
        glfwPollEvents();
    }

    // ---------------------------- MAIN LOOP ENDED ------------------------------

    // Cleanup
    ImGui_ImplOpenGL3_Shutdown();
    ImGui_ImplGlfw_Shutdown();
    ImGui::DestroyContext();
    
    glfwDestroyWindow(window);
    glfwTerminate();
    return 0;
}

void processInput(GLFWwindow* window)
{
    if (glfwGetKey(window, GLFW_KEY_ESCAPE) == GLFW_PRESS)
        glfwSetWindowShouldClose(window, true);

    if (glfwGetKey(window, GLFW_KEY_W) == GLFW_PRESS)
        camera.ProcessKeyboard(FORWARD, deltaTime);
    if (glfwGetKey(window, GLFW_KEY_S) == GLFW_PRESS)
        camera.ProcessKeyboard(BACKWARD, deltaTime);
    if (glfwGetKey(window, GLFW_KEY_A) == GLFW_PRESS)
        camera.ProcessKeyboard(LEFT, deltaTime);
    if (glfwGetKey(window, GLFW_KEY_D) == GLFW_PRESS)
        camera.ProcessKeyboard(RIGHT, deltaTime);
    if (glfwGetKey(window, GLFW_KEY_SPACE) == GLFW_PRESS)
        camera.ProcessKeyboard(UP, deltaTime);
    if (glfwGetKey(window, GLFW_KEY_LEFT_CONTROL) == GLFW_PRESS)
        camera.ProcessKeyboard(DOWN, deltaTime);

    if (glfwGetKey(window, GLFW_KEY_RIGHT) == GLFW_PRESS) {
        rotAngle += 0.25f;
        if (rotAngle >= 360) rotAngle = 0.0f;
    }  
    else if (glfwGetKey(window, GLFW_KEY_LEFT) == GLFW_PRESS){
        rotAngle -= 0.25f;
        if (rotAngle <= 0) rotAngle = 360.0f;
    }

    if (glfwGetKey(window, GLFW_KEY_UP) == GLFW_PRESS) moveVec -= speedVecZ * deltaTime;
    else if (glfwGetKey(window, GLFW_KEY_DOWN) == GLFW_PRESS) moveVec += speedVecZ * deltaTime;
    if (glfwGetKey(window, GLFW_KEY_R) == GLFW_PRESS) moveVec = zeroVec;

    if (glfwGetKey(window, GLFW_KEY_M) == GLFW_PRESS)
        Keys[GLFW_KEY_M] = true;
    else if (glfwGetKey(window, GLFW_KEY_M) == GLFW_RELEASE)
    {
        Keys[GLFW_KEY_M] = false;
        KeysProcessed[GLFW_KEY_M] = false;
    }

    if (Keys[GLFW_KEY_M] && !KeysProcessed[GLFW_KEY_M]) {
        if (state == MENU) state = ACTIVE;
        else if (state == ACTIVE) state = MENU; // bool?
        KeysProcessed[GLFW_KEY_M] = true;
    }
}

void SetnDrawModel(Shader& shader, Model& modelObj, bool moveable, glm::vec3 scale, glm::vec3 pos)
{
    shader.use();

    // view/projection transformations
    glm::mat4 projection = glm::perspective(glm::radians(camera.Zoom), (float)SCR_WIDTH / (float)SCR_HEIGHT, 0.1f, 100.0f);
    glm::mat4 view = camera.GetViewMatrix();
    shader.setMat4("projection", projection);
    shader.setMat4("view", view);

    // render the loaded model
    glm::mat4 model = glm::mat4(1.0f);
    if (moveable) {
        model = glm::translate(model, pos + moveVec); // translate it down so it's at the center of the scene
        model = glm::rotate(model, glm::radians(rotAngle), glm::vec3(0.0f, 1.0f, 0.0f));
    } else {
        model = glm::translate(model, pos); // translate it down so it's at the center of the scene
    }
    model = glm::scale(model, scale);	// it's a bit too big for our scene, so scale it down
    shader.setMat4("model", model);
    modelObj.Draw(shader);
}

void MenuDraw()
{
    // variables
    static string pathToModel;

    static bool firstOpen = true;

    static bool checkMove = false, loadWindow = false, pressDelete = false;
    static float position[3], scale = 1.0f;

    // Start the Dear ImGui frame
    ImGui_ImplOpenGL3_NewFrame();
    ImGui_ImplGlfw_NewFrame();
    ImGui::NewFrame();

    // set browser properties
    static ImGui::FileBrowser fileDialog;

    fileDialog.SetTitle("Browse Model");
    fileDialog.SetTypeFilters({ ".obj" });
    // ----------------------------------------------

    if (firstOpen) {
        ImGui::SetNextWindowSize(ImVec2(400, 130));
        firstOpen = false;
    }
    ImGui::Begin("ModelViewer Menu");

    // ----------------------- Model Configure Window --------------------------------

    if (!loadWindow) {
        ImGui::SetCursorPos(ImVec2(136.0f, 45.0f));
        if (ImGui::Button("Open Model File")) loadWindow = true;


        ImGui::SetCursorPos(ImVec2(130.0f, 75.0f));
        if (ImGui::Button("Delete Last Model") && !models.empty()) models.pop_back();
    }
    else {
        // Model info tools
        ImGui::Checkbox("Moveable model", &checkMove);
        ImGui::InputFloat3("Position", position);
        ImGui::InputFloat("Scale", &scale, 0.001f, 0.1f, "\t %.3f (min: 0.001)");

        if (ImGui::Button("Browse File")) fileDialog.Open();
            
        if (!pathToModel.empty()) {
            // loading model in modelVector
            models.push_back( LoadModel(pathToModel, checkMove, position, scale) ); // return pair <Model, modelInfo>

            // clear values for next model
            loadWindow = false, checkMove = false, state = ACTIVE, pathToModel.clear();
            scale = 1.0f;
            for (int i = 0; i < 3; ++i) {
                position[i] = 0.0f;
            }
        }

        // Back button
        ImGui::SetCursorPos(ImVec2(330.0f, 28.0f));
        if (ImGui::Button("Back")) loadWindow = false, pressDelete = false;
    }

    ImGui::End();

    fileDialog.Display();

    if (fileDialog.HasSelected())
    {
        if (pathToModel.empty()) pathToModel = fileDialog.GetSelected().string();
        fileDialog.ClearSelected();
    }
}

pair<Model, modelInfo> LoadModel(string pathToModel, bool moveable, float pos[3], float scale)
{
    Model model(convertPath(pathToModel), moveable);

    modelInfo mInfo;
    mInfo.pos.x = pos[0], mInfo.pos.y = pos[1], mInfo.pos.z = pos[2];
    mInfo.scale.x = scale, mInfo.scale.y = scale, mInfo.scale.z = scale;

    return make_pair(model, mInfo);
}

string convertPath(const std::string& str)
{
    std::string result = str;
    for (auto& c : result) {
        if (c == '\\') {
            c = '/';
        }
    }
    return result;
}

void framebuffer_size_callback(GLFWwindow* window, int width, int height)
{
    glViewport(0, 0, width, height);
}

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

void scroll_callback(GLFWwindow* window, double xoffset, double yoffset)
{
    camera.ProcessMouseScroll(static_cast<float>(yoffset));
}
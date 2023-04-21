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
#include "Animator.h"

#include <iostream>
#include <format>

enum progState {
    MENU,
    ACTIVE
};

// input
void framebuffer_size_callback(GLFWwindow* window, int width, int height);
void mouse_callback(GLFWwindow* window, double xpos, double ypos);
void scroll_callback(GLFWwindow* window, double xoffset, double yoffset);
void processInput(GLFWwindow* window);

// drawing
void ImGuiRender(ImGuiIO& io);
void SetnDrawModel(Shader& shader, Model& modelObj, Animator& animator, glm::vec3 scale, glm::vec3 pos);
void Drawing(GLFWwindow* window, Shader& ourShader);
void MenuDraw();
void HelpMenu();
void DrawCoordinates();

pair<Model*, pair<Animation*, Animator*>> LoadModel(string pathToModel, bool moveable, float pos[3], float scale);

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
string convertPath(const std::string& str);

vector<pair<Model*, pair<Animation*, Animator*>>> models; // first value = Model, second = Animator;
progState state = MENU;

bool KeysProcessed[1024], Keys[1024];
bool helpMenu = true;

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
    glfwSetInputMode(window, GLFW_CURSOR, GLFW_CURSOR_DISABLED);

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
        glClearColor(0.5f, 0.5f, 0.5f, 1.0f);
        glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

        // Timing
        float currentFrame = static_cast<float>(glfwGetTime());
        deltaTime = currentFrame - lastFrame;
        lastFrame = currentFrame;

        processInput(window);

        Drawing(window, ourShader);
  
        // imgui:Render + glfw: swap buffers and poll IO events (keys pressed/released, mouse moved etc.)
        ImGuiRender(io);
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
    if (glfwGetKey(window, GLFW_KEY_F4) == GLFW_PRESS)
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

    if (glfwGetKey(window, GLFW_KEY_LEFT_SHIFT) == GLFW_PRESS) camera.SetSpeed(11.0f);
    else if (glfwGetKey(window, GLFW_KEY_LEFT_SHIFT) == GLFW_RELEASE) camera.SetSpeed(5.5f);

    // MENU Keys

    // ESC key
    if (glfwGetKey(window, GLFW_KEY_ESCAPE) == GLFW_PRESS)
        Keys[GLFW_KEY_ESCAPE] = true;
    else if (glfwGetKey(window, GLFW_KEY_ESCAPE) == GLFW_RELEASE)
    {
        Keys[GLFW_KEY_ESCAPE] = false;
        KeysProcessed[GLFW_KEY_ESCAPE] = false;
    }
    // H key
    if (glfwGetKey(window, GLFW_KEY_H) == GLFW_PRESS)
        Keys[GLFW_KEY_H] = true;
    else if (glfwGetKey(window, GLFW_KEY_H) == GLFW_RELEASE)
    {
        Keys[GLFW_KEY_H] = false;
        KeysProcessed[GLFW_KEY_H] = false;
    }

    // ----                                                  ------
    if (Keys[GLFW_KEY_ESCAPE] && !KeysProcessed[GLFW_KEY_ESCAPE]) {
        if (state == MENU) state = ACTIVE;
        else if (state == ACTIVE) state = MENU;
        
        camera.SwitchCamera();

        KeysProcessed[GLFW_KEY_ESCAPE] = true;
    }
    if (Keys[GLFW_KEY_H] && !KeysProcessed[GLFW_KEY_H]) {
        helpMenu = !helpMenu;

        KeysProcessed[GLFW_KEY_H] = true;
    }
}

void ImGuiRender(ImGuiIO& io)
{
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

void SetnDrawModel(Shader& shader, Model& modelObj, Animator& animator, glm::vec3 scale, glm::vec3 pos)
{
    shader.use();

    // view/projection transformations
    glm::mat4 projection = glm::perspective(glm::radians(45.0f), (float)SCR_WIDTH / (float)SCR_HEIGHT, 0.1f, 8000.0f);
    glm::mat4 view = camera.GetViewMatrix();
    shader.setMat4("projection", projection);
    shader.setMat4("view", view);

    if (modelObj.IsAnimated()) {
        shader.setBool("animated", true);

        auto transforms = animator.GetFinalBoneMatrices();
        for (int i = 0; i < transforms.size(); ++i)
            shader.setMat4("finalBonesMatrices[" + std::to_string(i) + "]", transforms[i]);
    }
    else shader.setBool("animated", false);

    // render the loaded model
    glm::mat4 model = glm::mat4(1.0f);
    if (modelObj.IsMoveable()) {
        model = glm::translate(model, pos + moveVec); // translate it down so it's at the center of the scene
        model = glm::rotate(model, glm::radians(rotAngle), glm::vec3(0.0f, 1.0f, 0.0f));
    } else {
        model = glm::translate(model, pos); // translate it down so it's at the center of the scene
    }
    model = glm::scale(model, scale);	// it's a bit too big for our scene, so scale it down
    shader.setMat4("model", model);
    modelObj.Draw(shader);
}

void Drawing(GLFWwindow* window, Shader& ourShader)
{
    // Start the Dear ImGui frame
    ImGui_ImplOpenGL3_NewFrame();
    ImGui_ImplGlfw_NewFrame();
    ImGui::NewFrame();

    // Objects drawing
    for (int i = 0; i < models.size(); i++)
    {
        if (models[i].first->IsAnimated()) models[i].second.second->UpdateAnimation(deltaTime);

        SetnDrawModel(ourShader, *models[i].first, *models[i].second.second, models[i].first->GetScaleVec(), models[i].first->GetPosVec());
    }

    // Menu/Help drawing
    if (helpMenu) HelpMenu();

    if (state == MENU) {
        glfwSetInputMode(window, GLFW_CURSOR, GLFW_CURSOR_NORMAL);

        MenuDraw();
    }
    else {
        glfwSetInputMode(window, GLFW_CURSOR, GLFW_CURSOR_DISABLED);
    }

    DrawCoordinates();
}

void MenuDraw()
{
    // variables
    static string pathToModel;

    static bool firstOpen = true;

    static bool checkMove = false, loadWindow = false, pressDelete = false;
    static float position[3], scale = 1.0f;

    // set browser properties
    static ImGui::FileBrowser fileDialog;

    fileDialog.SetTitle("Browse Model");
    fileDialog.SetTypeFilters({ ".obj", ".fbx", ".dae" });
    // ----------------------------------------------

    if (firstOpen) {
        ImGui::SetNextWindowSize(ImVec2(380, 130));
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

void HelpMenu()
{
    static bool firstOpen = true;
    static string helpStr;

    if (firstOpen) {
        std::ifstream helpFile;
        std::stringstream helpFileStream;

        try
        {
            helpFile.exceptions(std::ifstream::failbit | std::ifstream::badbit);
            helpFile.open("../readme.md");
            
            helpFileStream << helpFile.rdbuf();
            helpFile.close();

            helpStr = helpFileStream.str();
        }
        catch (const ifstream::failure fail)
        {
            helpFile.clear();
            try
            {
                helpFile.exceptions(std::ifstream::failbit | std::ifstream::badbit);
                helpFile.open("readme.md");

                helpFileStream << helpFile.rdbuf();
                helpFile.close();

                helpStr = helpFileStream.str();
            }
            catch (const ifstream::failure fail)
            {
                helpStr = "Help file not found";
            }
        }
       
        ImGui::SetNextWindowSize(ImVec2(320, 130));
        firstOpen = false;
    }

    // ------------- MENU ----------------
    ImGui::SetNextWindowPos(ImVec2(200.0f, 200.0f), ImGuiCond_Once);
    ImGui::Begin("Help");

    ImGui::Text(helpStr.c_str());

    ImGui::End();
}

void DrawCoordinates()
{
    static bool firstOpen = true;

    if (firstOpen) {
        ImGui::SetNextWindowSize(ImVec2(100, 90));
        ImGui::SetNextWindowPos(ImVec2(200, 800));
        firstOpen = false;
    }
    ImGui::Begin("Position");
    
    ImGui::Text(("X: " + std::to_string(camera.GetCameraPosition().x)).c_str());
    ImGui::Text(("Y: " + std::to_string(camera.GetCameraPosition().y)).c_str());
    ImGui::Text(("Z: " + std::to_string(camera.GetCameraPosition().z)).c_str());

    ImGui::End();
}

pair<Model*, pair<Animation*, Animator*>> LoadModel(string pathToModel, bool moveable, float pos[3], float scale)
{
    Model* model = new Model(convertPath(pathToModel), moveable);
    model->SetPosVec(pos);
    model->SetScaleVec(scale);
    
    camera.MoveToObject(model->GetPosVec(), model->GetSize(), model->GetCenter(), model->GetScaleVec());
    camera.SwitchCamera();

    // if animated 
    try
    {
        Animation* anim = new Animation(convertPath(pathToModel), model);
        Animator* animator = new Animator(anim);
        model->SetAnimated(true);

        return make_pair(model, make_pair(anim, animator));
    }
    catch (const bool ex) {
        model->SetAnimated(false);

        return make_pair(model, make_pair(nullptr, nullptr));
    }
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

    if (camera.IsDisabled()) return;
    camera.ProcessMouseMovement(xoffset, yoffset);
}

void scroll_callback(GLFWwindow* window, double xoffset, double yoffset)
{
    camera.ProcessMouseScroll(static_cast<float>(yoffset));
}
/**
 * @file main.cpp
 * @brief Main application file for OpenGL with ImGui Template.
 *
 * This file contains the main function and initialization routines for an
 * OpenGL application using ImGui for rendering the GUI components.
 *
 * @author Mustafa Alotbah
 * @date 08 Dec 2023
 * @version 0.1
 * @terms_of_use 'Creative Commons Attribution 4.0 International (CC BY 4.0)'
 */

#include <iostream>
#include <string>
#include <vector>
#include <algorithm>

#include <glad/glad.h>
#include <GLFW/glfw3.h>
#include <imgui.h>
#include <backends/imgui_impl_glfw.h>
#include <backends/imgui_impl_opengl3.h>

#define GLFW_EXPOSE_NATIVE_WIN32
#include <GLFW/glfw3native.h>
#include <windows.h>

#include "utility/Layer2D.h"
#include "utility/Logger.h"
#include "utility/SettingsManager.h"

// Global flags
static bool should_exit = false;

// Window visibility flags
static bool show_shader_controls = true;
static bool show_viewport = true;
static bool show_logger = true;

// File to open (set by menu, processed in main loop)
static std::string pending_file_to_open = "";

/**
 * @brief Opens a Windows file dialog to select a shader file
 * @param window The GLFW window (for modal dialog)
 * @return Selected file path, or empty string if cancelled
 */
std::string openFileDialog(GLFWwindow* window) {
    HWND hwnd = glfwGetWin32Window(window);
    
    OPENFILENAMEA ofn;
    char szFile[260] = {0};
    
    ZeroMemory(&ofn, sizeof(ofn));
    ofn.lStructSize = sizeof(ofn);
    ofn.hwndOwner = hwnd;
    ofn.lpstrFile = szFile;
    ofn.nMaxFile = sizeof(szFile);
    ofn.lpstrFilter = "GLSL Shaders\0*.frag;*.vert;*.glsl\0All Files\0*.*\0";
    ofn.nFilterIndex = 1;
    ofn.lpstrFileTitle = nullptr;
    ofn.nMaxFileTitle = 0;
    ofn.lpstrInitialDir = nullptr;
    ofn.Flags = OFN_PATHMUSTEXIST | OFN_FILEMUSTEXIST | OFN_NOCHANGEDIR;
    
    if (GetOpenFileNameA(&ofn) == TRUE) {
        return std::string(ofn.lpstrFile);
    }
    
    return "";
}

/**
 * @brief Creates a GLFW window with specified settings.
 *
 * Initializes the GLFW library and creates a windowed mode window with OpenGL context.
 * The function sets various window hints such as compatibility and debugging flags.
 * If GLFW fails to initialize or window creation fails, it returns a nullptr.
 *
 * @return GLFWwindow* A pointer to the created GLFW window, or nullptr if the window creation fails.
 */
GLFWwindow *createGLFWWindow();

/**
 * @brief Initializes OpenGL settings.
 *
 * Sets up OpenGL context according to the provided arguments. It initializes the GLEW library,
 * sets OpenGL state based on the parameters, and prints the OpenGL version to the console.
 * The function enables or disables features like depth test, face culling, and blending based on the input parameters.
 *
 * @param depthTest Enables depth testing if true.
 * @param cullFace Enables face culling if true.
 * @param blend Enables blending if true.
 *
 * @note Exits the application if GLEW initialization fails.
 */
void initializeOpenGL(bool depthTest, bool cullFace, bool blend);

/**
 * @brief Initializes ImGui with GLFW and OpenGL.
 *
 * Creates an ImGui context and sets up ImGuiIO configuration. The function also loads custom fonts,
 * sets the ImGui style to dark, and initializes ImGui for GLFW and OpenGL.
 * It additionally customizes the ImGui style, adjusting elements such as window padding, rounding, and color.
 *
 * @param window Pointer to the GLFWwindow for which ImGui is being initialized.
 */
void initImGui(GLFWwindow *window);

/**
 * @brief Retrieves the current state of mouse inputs.
 *
 * Queries the ImGui library to check the status of mouse buttons (left, middle, right) and the mouse wheel.
 * It returns an InputState structure containing the status of the mouse buttons and the mouse wheel delta.
 *
 * @return InputState A structure containing the current state of the mouse buttons and the mouse wheel delta.
 */
InputState getState() {

    bool mouseButtonLeft = ImGui::IsMouseDown(ImGuiMouseButton_Left);
    bool mouseButtonMiddle = ImGui::IsMouseDown(ImGuiMouseButton_Middle);
    bool mouseButtonRight = ImGui::IsMouseDown(ImGuiMouseButton_Right);

    Event::MouseButton button = Event::MouseButton::None;
    if (mouseButtonLeft) button = Event::MouseButton::Left;
    else if (mouseButtonRight) button = Event::MouseButton::Right;
    else if (mouseButtonMiddle) button = Event::MouseButton::Middle;

    return {button, ImGui::GetIO().MouseWheel};
}



int main() {

    // Create GLFW window
    GLFWwindow *window = createGLFWWindow();
    if (!window) return -1;

    // Initialize OpenGL
    initializeOpenGL(false, true, true);

    // Initialize ImGui
    initImGui(window);

    // Custom Renderer
    // Layer2D renderer;
    KiwiCore* app = KiwiAppFactory::getInstance().createApp("MyKiwiApp");
    app->onLoad();

    /* Main Loop */
    while (!glfwWindowShouldClose(window) && !should_exit) {
        /* Clear Buffer */
        glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

        // Start new ImGui frame
        ImGui_ImplOpenGL3_NewFrame();
        ImGui_ImplGlfw_NewFrame();
        ImGui::NewFrame();

        // ImGui docking space
        ImGui::DockSpaceOverViewport();

        /* Window Rendering */
        {
            /* Main menu bar */
            if (ImGui::BeginMainMenuBar()) {
                if (ImGui::BeginMenu("File")) {
                    if (ImGui::MenuItem("Open...", "Ctrl+O")) {
                        std::string filePath = openFileDialog(window);
                        if (!filePath.empty()) {
                            pending_file_to_open = filePath;
                        }
                    }
                    
                    auto recentFiles = SettingsManager::getInstance().getRecentFiles();
                    if (ImGui::BeginMenu("Open Recent", !recentFiles.empty())) {
                        for (size_t i = 0; i < recentFiles.size(); ++i) {
                            const auto& file = recentFiles[i];
                            // Extract filename for display
                            size_t lastSlash = file.find_last_of("/\\");
                            std::string displayName = (lastSlash != std::string::npos) 
                                ? file.substr(lastSlash + 1) 
                                : file;
                            
                            if (ImGui::MenuItem(displayName.c_str())) {
                                pending_file_to_open = file;
                            }
                            
                            if (ImGui::IsItemHovered()) {
                                ImGui::SetTooltip("%s", file.c_str());
                            }
                        }
                        
                        ImGui::Separator();
                        if (ImGui::MenuItem("Clear Recent Files")) {
                            SettingsManager::getInstance().clearRecentFiles();
                        }
                        
                        ImGui::EndMenu();
                    }
                    
                    ImGui::Separator();
                    if (ImGui::MenuItem("Exit", "Alt+F4")) should_exit = true;
                    ImGui::EndMenu();
                }
                
                if (ImGui::BeginMenu("View")) {
                    ImGui::MenuItem("Shader Controls", nullptr, &show_shader_controls);
                    ImGui::MenuItem("Viewport", nullptr, &show_viewport);
                    ImGui::MenuItem("Logger", nullptr, &show_logger);
                    ImGui::EndMenu();
                }
                
                ImGui::EndMainMenuBar();
            }
            
            // Process pending file open (from menu)
            if (!pending_file_to_open.empty()) {
                app->loadShaderFromMenu(pending_file_to_open);
                SettingsManager::getInstance().addRecentFile(pending_file_to_open);
                SettingsManager::getInstance().setLastShader(pending_file_to_open);
                pending_file_to_open = "";
            }

            /* Begin: Shader Controls Window */
            if (show_shader_controls) {
                ImGui::Begin("Shader Controls", &show_shader_controls);
                ImGui::Text("Application average %.3f ms/frame (%.1f FPS)",
                            1000.0f / ImGui::GetIO().Framerate,
                            ImGui::GetIO().Framerate);

                // renderer.renderProperties();
                app->onUpdateUI();

                ImGui::End();
            }
            /* End: Shader Controls Window */


            /* Begin: Viewport Window */
            if (show_viewport) {
                ImGui::Begin("Viewport", &show_viewport);
                // Do whatever you want here

                ImVec2 windowSize =  ImGui::GetWindowSize();
                ImVec2 windowPos = ImGui::GetWindowPos(); // Top-left corner of the image
                ImVec2 mousePos = ImGui::GetMousePos();


                windowSize.x -= 12;
                windowSize.y -= 48;
                double time = ImGui::GetTime();
                double delta = 1.0 / ImGui::GetIO().Framerate;

                app->onUpdate(time, delta);
                app->renderFrame(windowSize.x, windowSize.y, time, delta);
                ImGui::Image((ImTextureID) app->getTextureId(), windowSize, ImVec2(0, 1), ImVec2(1, 0));

                app->pollEvents(
                        glm::vec2(windowPos.x + 12-3, windowPos.y + 48 - 10),
                        glm::vec2(mousePos.x, mousePos.y),
                        getState());

                ImGui::End();
            }
            /* End: Viewport Window */

            // Render the Logger window
            if (show_logger) {
                Logger::onDraw();
            }
        }

        /* BEGIN: Render ImGui and handle multiple viewports */
        {

            ImGui::Render();
            int display_w, display_h;
            glfwGetFramebufferSize(window, &display_w, &display_h);
            glViewport(0, 0, display_w, display_h);
            glClear(GL_COLOR_BUFFER_BIT);

            ImGui_ImplOpenGL3_RenderDrawData(ImGui::GetDrawData());

            // Handling multiple viewports
            if (ImGui::GetIO().ConfigFlags & ImGuiConfigFlags_ViewportsEnable) {
                GLFWwindow *backup_current_context = glfwGetCurrentContext();
                ImGui::UpdatePlatformWindows();
                ImGui::RenderPlatformWindowsDefault(); // Crucial for multi-viewport
                glfwMakeContextCurrent(backup_current_context);
            }
        }
        /* END: Render ImGui and handle multiple viewports */


        /* Swap buffers and poll IO events */
        glfwSwapBuffers(window);
        glfwPollEvents();
    }

    // Cleanup
    ImGui_ImplOpenGL3_Shutdown();
    ImGui_ImplGlfw_Shutdown();
    ImGui::DestroyContext();

    glfwDestroyWindow(window);
    glfwTerminate();

    return 0;
}


// Function to create a GLFW window
GLFWwindow *createGLFWWindow() {

    /* Initialize the library */
    if (!glfwInit()) return nullptr;
    glfwWindowHint(GLFW_OPENGL_FORWARD_COMPAT, GL_FALSE);
    glfwWindowHint(GLFW_OPENGL_DEBUG_CONTEXT, GL_TRUE);

    /* Create a windowed mode window and its OpenGL context */
    GLFWwindow *window = glfwCreateWindow(1480, 960, PROJECT_NAME, nullptr, nullptr);
    if (!window) {
        glfwTerminate();
        return nullptr;
    }

    /* Make the window's context current */
    glfwMakeContextCurrent(window);

    return window;
}

// Function to initialize OpenGL settings
void initializeOpenGL(bool depthTest, bool cullFace, bool blend) {

    /* Initialize openGL */
    if (!gladLoadGL()) {
        std::cout << "[Error] Init of GLEW failed (" << __LINE__ << ") " << std::endl;
        exit(-1);
    } else std::cout << "GLEW OK" << std::endl;
    std::cout << "GL Version : " << glGetString(GL_VERSION) << std::endl;

    // Set OpenGL state
    if (depthTest) {
        glEnable(GL_DEPTH_TEST);
        glDepthFunc(GL_LESS);
    }

    if (cullFace) {
        glEnable(GL_CULL_FACE);
        glFrontFace(GL_BACK);
        glCullFace(GL_CW);
    }

    if (blend) {
        glEnable(GL_BLEND);
        glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
    }
}

// Function to initialize ImGui
void initImGui(GLFWwindow *window) {

    // Create ImGui context
    ImGui::CreateContext();
    ImGuiIO &io = ImGui::GetIO();
    (void) io;

    // Enable ImGui features
    io.ConfigFlags |= ImGuiConfigFlags_DockingEnable;
    io.ConfigFlags |= ImGuiConfigFlags_ViewportsEnable;

    // Load custom font
    io.FontDefault = io.Fonts->AddFontFromFileTTF(
            (std::string(ASSETS_PATH) + "/fonts/OpenSans-Bold.ttf").c_str(),
            22.0f
    );
    io.FontDefault = io.Fonts->AddFontFromFileTTF(
            (std::string(ASSETS_PATH) + "/fonts/OpenSans-Regular.ttf").c_str(),
            22.0f
    );
    
    // Load monospace font for Logger (Cascadia Code, Fira Code, etc.)
    Logger::loadMonospaceFont();

    // Set ImGui style
    ImGui::StyleColorsDark();

    // Initialize ImGui for GLFW and OpenGL
    ImGui_ImplGlfw_InitForOpenGL(window, true);
    ImGui_ImplOpenGL3_Init("#version 410");


    // Edit ImGui Style
    ImGuiStyle& style = ImGui::GetStyle();

    style.WindowMenuButtonPosition = ImGuiDir_None;
    style.IndentSpacing = 10;
    style.WindowPadding = ImVec2(10.0f, 8.0f);
    style.WindowRounding = 10.0f;
    style.FramePadding = ImVec2(5.0f, 4.0f);
    style.FrameRounding = 5.0f;

    style.Colors[ImGuiCol_Text] = ImVec4(1.0f, 1.0f, 1.0f, 1.0f);       // White text
    style.Colors[ImGuiCol_WindowBg] = ImVec4(0.1f, 0.1f, 0.1f, 1.0f);    // White text
    style.Colors[ImGuiCol_TitleBgActive] = ImVec4(0.2f, 0.2f, 0.2f, 1.0f);   // Dark window background

}




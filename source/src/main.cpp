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
#include "utility/FullscreenQuad.h"
#include "utility/StatusBar.h"

// Global flags
static bool should_exit = false;

// Window visibility flags
static bool show_shader_controls = true;
static bool show_viewport = true;
static bool show_logger = true;

// Fullscreen state
static bool is_fullscreen = false;
static int windowed_x = 100, windowed_y = 100;
static int windowed_width = 1480, windowed_height = 960;
static FullscreenQuad* fullscreenQuad = nullptr;

// File to open (set by menu, processed in main loop)
static std::string pending_file_to_open = "";

/**
 * @brief Toggle fullscreen mode using GLFW
 */
void toggleFullscreen(GLFWwindow* window) {
    if (is_fullscreen) {
        // Exit fullscreen - restore windowed mode
        glfwSetWindowMonitor(window, nullptr, windowed_x, windowed_y, windowed_width, windowed_height, GLFW_DONT_CARE);
        is_fullscreen = false;
    } else {
        // Enter fullscreen - save window state first
        glfwGetWindowPos(window, &windowed_x, &windowed_y);
        glfwGetWindowSize(window, &windowed_width, &windowed_height);
        
        // Get primary monitor
        GLFWmonitor* monitor = glfwGetPrimaryMonitor();
        const GLFWvidmode* mode = glfwGetVideoMode(monitor);
        
        // Go fullscreen
        glfwSetWindowMonitor(window, monitor, 0, 0, mode->width, mode->height, mode->refreshRate);
        is_fullscreen = true;
    }
}

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
    
    // Initialize fullscreen quad renderer
    fullscreenQuad = new FullscreenQuad();
    if (!fullscreenQuad->initialize()) {
        std::cerr << "Failed to initialize fullscreen quad renderer" << std::endl;
    }

    /* Main Loop */
    while (!glfwWindowShouldClose(window) && !should_exit) {
        
        // Handle key input for fullscreen toggle (using GLFW directly for reliability)
        static bool f11_was_pressed = false;
        bool f11_pressed = glfwGetKey(window, GLFW_KEY_F11) == GLFW_PRESS;
        if (f11_pressed && !f11_was_pressed) {
            toggleFullscreen(window);
        }
        f11_was_pressed = f11_pressed;
        
        static bool esc_was_pressed = false;
        bool esc_pressed = glfwGetKey(window, GLFW_KEY_ESCAPE) == GLFW_PRESS;
        if (is_fullscreen && esc_pressed && !esc_was_pressed) {
            toggleFullscreen(window);
        }
        esc_was_pressed = esc_pressed;
        
        // Get current framebuffer size
        int display_w, display_h;
        glfwGetFramebufferSize(window, &display_w, &display_h);
        
        // =====================================================================
        // FULLSCREEN MODE: Render framebuffer directly, skip ImGui
        // =====================================================================
        if (is_fullscreen) {
            // Update and render the shader at fullscreen resolution
            double time = glfwGetTime();
            static double lastTime = time;
            double delta = time - lastTime;
            lastTime = time;
            
            app->onUpdate(time, delta);
            app->renderFrame(display_w, display_h, time, delta);
            
            // Render the framebuffer to fullscreen using our quad renderer
            fullscreenQuad->render((GLuint)(uintptr_t)app->getTextureId(), display_w, display_h);
            
            // Render minimal ImGui overlay for hint text
            ImGui_ImplOpenGL3_NewFrame();
            ImGui_ImplGlfw_NewFrame();
            ImGui::NewFrame();
            
            // Transparent fullscreen overlay for hint
            ImGuiViewport* viewport = ImGui::GetMainViewport();
            ImGui::SetNextWindowPos(ImVec2(10, 10));
            ImGui::SetNextWindowBgAlpha(0.3f);
            ImGui::Begin("##fullscreen_hint", nullptr, 
                ImGuiWindowFlags_NoDecoration | 
                ImGuiWindowFlags_AlwaysAutoResize |
                ImGuiWindowFlags_NoSavedSettings |
                ImGuiWindowFlags_NoFocusOnAppearing |
                ImGuiWindowFlags_NoNav |
                ImGuiWindowFlags_NoMove);
            ImGui::TextColored(ImVec4(1.0f, 1.0f, 1.0f, 0.8f), "ESC to exit fullscreen | F11 to toggle");
            ImGui::End();
            
            ImGui::Render();
            ImGui_ImplOpenGL3_RenderDrawData(ImGui::GetDrawData());
        }
        // =====================================================================
        // WINDOWED MODE: Normal ImGui rendering
        // =====================================================================
        else {
            /* Clear Buffer */
            glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

            // Start new ImGui frame
            ImGui_ImplOpenGL3_NewFrame();
            ImGui_ImplGlfw_NewFrame();
            ImGui::NewFrame();

            // ImGui docking space with reserved space for status bar at bottom
            ImGuiViewport* viewport = ImGui::GetMainViewport();
            ImGuiDockNodeFlags dockspace_flags = ImGuiDockNodeFlags_PassthruCentralNode;
            
            // Reserve space at the bottom for status bar
            ImVec2 workPos = viewport->WorkPos;
            ImVec2 workSize = viewport->WorkSize;
            workSize.y -= StatusBar::getHeight(); // Reduce dockspace height to leave room for status bar
            
            ImGui::SetNextWindowPos(workPos);
            ImGui::SetNextWindowSize(workSize);
            ImGui::SetNextWindowViewport(viewport->ID);
            
            ImGuiWindowFlags window_flags = ImGuiWindowFlags_NoTitleBar | ImGuiWindowFlags_NoCollapse |
                                           ImGuiWindowFlags_NoResize | ImGuiWindowFlags_NoMove |
                                           ImGuiWindowFlags_NoBringToFrontOnFocus | ImGuiWindowFlags_NoNavFocus |
                                           ImGuiWindowFlags_NoBackground;
            
            ImGui::PushStyleVar(ImGuiStyleVar_WindowPadding, ImVec2(0.0f, 0.0f));
            ImGui::Begin("DockSpace", nullptr, window_flags);
            ImGui::PopStyleVar();
            
            ImGuiID dockspace_id = ImGui::GetID("MainDockSpace");
            ImGui::DockSpace(dockspace_id, ImVec2(0.0f, 0.0f), dockspace_flags);
            ImGui::End();

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
                        ImGui::Separator();
                        if (ImGui::MenuItem("Fullscreen", "F11")) {
                            toggleFullscreen(window);
                        }
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
                    
                    app->onUpdateUI();

                    ImGui::End();
                }
                /* End: Shader Controls Window */


                /* Begin: Viewport Window */
                if (show_viewport) {
                    ImGui::Begin("Viewport", &show_viewport);

                    ImVec2 windowSize = ImGui::GetContentRegionAvail();
                    ImVec2 windowPos = ImGui::GetWindowPos();
                    ImVec2 mousePos = ImGui::GetMousePos();

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
                
                // Render the Status Bar (always at the bottom)
                StatusBar::getInstance().render();
            }

            /* BEGIN: Render ImGui and handle multiple viewports */
            {
                ImGui::Render();
                glViewport(0, 0, display_w, display_h);
                glClear(GL_COLOR_BUFFER_BIT);

                ImGui_ImplOpenGL3_RenderDrawData(ImGui::GetDrawData());

                // Handling multiple viewports
                if (ImGui::GetIO().ConfigFlags & ImGuiConfigFlags_ViewportsEnable) {
                    GLFWwindow *backup_current_context = glfwGetCurrentContext();
                    ImGui::UpdatePlatformWindows();
                    ImGui::RenderPlatformWindowsDefault();
                    glfwMakeContextCurrent(backup_current_context);
                }
            }
            /* END: Render ImGui and handle multiple viewports */
        }

        /* Swap buffers and poll IO events */
        glfwSwapBuffers(window);
        glfwPollEvents();
    }

    // Cleanup
    delete fullscreenQuad;
    fullscreenQuad = nullptr;
    
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
    GLFWwindow *window = glfwCreateWindow(1480, 960, "Kiwi Shader", nullptr, nullptr);
    if (!window) {
        glfwTerminate();
        return nullptr;
    }

    /* Make the window's context current */
    glfwMakeContextCurrent(window);
    
    /* Enable vsync (swap interval = 1) */
    glfwSwapInterval(1);

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




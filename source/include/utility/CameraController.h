/**
 * @file CameraController.h
 * @brief Interactive 3D camera controller for shader exploration
 * 
 * Features:
 * - FPS-style camera controls (WASD, arrow keys for movement)
 * - Mouse look (right button drag for rotation)
 * - Mouse pan (left button drag for translation)
 * - Mouse wheel for zoom/speed
 * - Provides standard camera uniforms for shaders
 * 
 * Standard Uniforms Provided:
 * - vec3 uCameraPosition
 * - vec3 uCameraTarget
 * - vec3 uCameraUp
 * - mat4 uViewMatrix
 * - mat4 uProjectionMatrix
 * - mat4 uViewProjectionMatrix
 * - vec3 uCameraForward
 * - vec3 uCameraRight
 */

#pragma once

#include <glad/glad.h>
#include <GLFW/glfw3.h>
#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>

/**
 * @brief Camera projection type
 */
enum class CameraProjection {
    Perspective,
    Orthographic
};

/**
 * @brief 3D camera state and transformation
 */
struct Camera3DState {
    // Position & orientation (default: looking at origin from above-front)
    glm::vec3 position{0.0f, 3.0f, 8.0f};
    float pitch = -15.0f;   // Rotation around right axis (looking slightly down)
    float yaw = -90.0f;     // Rotation around up axis (looking towards -Z)
    float roll = 0.0f;      // Rotation around forward axis (tilt)
    
    // Derived vectors
    glm::vec3 forward{0.0f, 0.0f, -1.0f};
    glm::vec3 right{1.0f, 0.0f, 0.0f};
    glm::vec3 up{0.0f, 1.0f, 0.0f};
    
    // Projection settings
    CameraProjection projectionType = CameraProjection::Perspective;
    float fov = 45.0f;          // Field of view (perspective)
    float aspectRatio = 16.0f/9.0f;
    float nearPlane = 0.1f;
    float farPlane = 1000.0f;
    float orthoSize = 10.0f;    // Size for orthographic projection
    
    // Movement settings
    float moveSpeed = 2.5f;     // Units per second
    float sprintMultiplier = 2.0f;
    float mouseSensitivity = 0.1f;
    float panSensitivity = 0.01f;
    
    /**
     * @brief Update derived vectors based on pitch/yaw/roll
     */
    void updateVectors();
    
    /**
     * @brief Get view matrix (camera to world transform)
     */
    glm::mat4 getViewMatrix() const;
    
    /**
     * @brief Get projection matrix
     */
    glm::mat4 getProjectionMatrix() const;
    
    /**
     * @brief Get combined view-projection matrix
     */
    glm::mat4 getViewProjectionMatrix() const;
};

/**
 * @brief Interactive camera controller with mouse and keyboard input
 */
class CameraController {
public:
    CameraController();
    
    /**
     * @brief Update camera state based on input
     * @param window GLFW window for input polling
     * @param deltaTime Time since last frame (seconds)
     */
    void update(GLFWwindow* window, double deltaTime);
    
    /**
     * @brief Handle mouse button events
     * @param button GLFW mouse button
     * @param action GLFW_PRESS or GLFW_RELEASE
     * @param mouseX Current mouse X position
     * @param mouseY Current mouse Y position
     * @param window GLFW window (for cursor control, can be nullptr)
     */
    void onMouseButton(int button, int action, double mouseX, double mouseY, GLFWwindow* window = nullptr);
    
    /**
     * @brief Handle mouse movement
     * @param mouseX Current mouse X position
     * @param mouseY Current mouse Y position
     */
    void onMouseMove(double mouseX, double mouseY);
    
    /**
     * @brief Handle mouse scroll (zoom or speed adjustment)
     * @param yOffset Scroll amount
     */
    void onMouseScroll(double yOffset);
    
    /**
     * @brief Set camera uniforms in a shader program
     * @param programId OpenGL shader program ID
     */
    void setShaderUniforms(unsigned int programId) const;
    
    /**
     * @brief Check which camera uniforms are supported by a shader
     * @param programId OpenGL shader program ID
     * @return Bitmask of supported uniforms
     */
    struct CameraUniformSupport {
        bool position = false;
        bool forward = false;
        bool right = false;
        bool up = false;
        bool target = false;
        bool viewMatrix = false;
        bool projectionMatrix = false;
        bool viewProjectionMatrix = false;
        bool fov = false;
        bool nearPlane = false;
        bool farPlane = false;
        
        bool hasAnySupport() const {
            return position || forward || right || up || target || 
                   viewMatrix || projectionMatrix || viewProjectionMatrix ||
                   fov || nearPlane || farPlane;
        }
        
        int countSupported() const {
            return (position ? 1 : 0) + (forward ? 1 : 0) + (right ? 1 : 0) + 
                   (up ? 1 : 0) + (target ? 1 : 0) + (viewMatrix ? 1 : 0) + 
                   (projectionMatrix ? 1 : 0) + (viewProjectionMatrix ? 1 : 0) +
                   (fov ? 1 : 0) + (nearPlane ? 1 : 0) + (farPlane ? 1 : 0);
        }
    };
    
    static CameraUniformSupport checkShaderSupport(unsigned int programId);
    
    /**
     * @brief Get camera state (read-only)
     */
    const Camera3DState& getState() const { return state_; }
    
    /**
     * @brief Get camera state (mutable, for direct manipulation)
     */
    Camera3DState& getState() { return state_; }
    
    /**
     * @brief Reset camera to default position
     */
    void reset();
    
    /**
     * @brief Enable/disable camera controls
     */
    void setEnabled(bool enabled) { enabled_ = enabled; }
    bool isEnabled() const { return enabled_; }
    
    /**
     * @brief Set aspect ratio (call on window resize)
     */
    void setAspectRatio(float aspectRatio);

private:
    void handleKeyboardInput(GLFWwindow* window, double deltaTime);
    void handleMouseLook(double deltaX, double deltaY);
    void handleMousePan(double deltaX, double deltaY);
    
    Camera3DState state_;
    
    // Mouse state
    bool leftButtonDown_ = false;
    bool rightButtonDown_ = false;
    double lastMouseX_ = 0.0;
    double lastMouseY_ = 0.0;
    bool firstMouse_ = true;
    
    // Control state
    bool enabled_ = true;
};


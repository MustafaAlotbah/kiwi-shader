/**
 * @file CameraController.cpp
 * @brief Implementation of interactive 3D camera controller
 */

#include "utility/CameraController.h"
#include "utility/Logger.h"
#include <glm/gtc/type_ptr.hpp>
#include <algorithm>

// ============================================================================
// Camera3DState Implementation
// ============================================================================

void Camera3DState::updateVectors() {
    // Calculate forward vector from pitch and yaw
    glm::vec3 newForward;
    newForward.x = cos(glm::radians(yaw)) * cos(glm::radians(pitch));
    newForward.y = sin(glm::radians(pitch));
    newForward.z = sin(glm::radians(yaw)) * cos(glm::radians(pitch));
    forward = glm::normalize(newForward);
    
    // Calculate right and up vectors
    right = glm::normalize(glm::cross(forward, glm::vec3(0.0f, 1.0f, 0.0f)));
    up = glm::normalize(glm::cross(right, forward));
}

glm::mat4 Camera3DState::getViewMatrix() const {
    return glm::lookAt(position, position + forward, up);
}

glm::mat4 Camera3DState::getProjectionMatrix() const {
    if (projectionType == CameraProjection::Perspective) {
        return glm::perspective(glm::radians(fov), aspectRatio, nearPlane, farPlane);
    } else {
        float halfWidth = orthoSize * aspectRatio * 0.5f;
        float halfHeight = orthoSize * 0.5f;
        return glm::ortho(-halfWidth, halfWidth, -halfHeight, halfHeight, nearPlane, farPlane);
    }
}

glm::mat4 Camera3DState::getViewProjectionMatrix() const {
    return getProjectionMatrix() * getViewMatrix();
}

// ============================================================================
// CameraController Implementation
// ============================================================================

CameraController::CameraController() {
    state_.updateVectors();
    Logger::Info("CameraController", "Initialized - Use Arrow Keys/WASD to move, Right Mouse to look, Left Mouse to pan", {"camera", "init"});
}

void CameraController::update(GLFWwindow* window, double deltaTime) {
    if (!enabled_) return;
    
    handleKeyboardInput(window, deltaTime);
}

void CameraController::handleKeyboardInput(GLFWwindow* window, double deltaTime) {
    float velocity = state_.moveSpeed * static_cast<float>(deltaTime);
    
    // Sprint modifier (Shift key)
    if (glfwGetKey(window, GLFW_KEY_LEFT_SHIFT) == GLFW_PRESS ||
        glfwGetKey(window, GLFW_KEY_RIGHT_SHIFT) == GLFW_PRESS) {
        velocity *= state_.sprintMultiplier;
    }
    
    // Forward/Backward: W/S or Up/Down arrows
    if (glfwGetKey(window, GLFW_KEY_W) == GLFW_PRESS || 
        glfwGetKey(window, GLFW_KEY_UP) == GLFW_PRESS) {
        state_.position += state_.forward * velocity;
    }
    if (glfwGetKey(window, GLFW_KEY_S) == GLFW_PRESS || 
        glfwGetKey(window, GLFW_KEY_DOWN) == GLFW_PRESS) {
        state_.position -= state_.forward * velocity;
    }
    
    // Left/Right: A/D or Left/Right arrows
    if (glfwGetKey(window, GLFW_KEY_A) == GLFW_PRESS || 
        glfwGetKey(window, GLFW_KEY_LEFT) == GLFW_PRESS) {
        state_.position -= state_.right * velocity;
    }
    if (glfwGetKey(window, GLFW_KEY_D) == GLFW_PRESS || 
        glfwGetKey(window, GLFW_KEY_RIGHT) == GLFW_PRESS) {
        state_.position += state_.right * velocity;
    }
    
    // Up/Down: Q/E or Space/Ctrl
    if (glfwGetKey(window, GLFW_KEY_Q) == GLFW_PRESS || 
        glfwGetKey(window, GLFW_KEY_SPACE) == GLFW_PRESS) {
        state_.position += state_.up * velocity;
    }
    if (glfwGetKey(window, GLFW_KEY_E) == GLFW_PRESS || 
        glfwGetKey(window, GLFW_KEY_LEFT_CONTROL) == GLFW_PRESS ||
        glfwGetKey(window, GLFW_KEY_RIGHT_CONTROL) == GLFW_PRESS) {
        state_.position -= state_.up * velocity;
    }
}

void CameraController::onMouseButton(int button, int action, double mouseX, double mouseY) {
    if (!enabled_) return;
    
    if (button == GLFW_MOUSE_BUTTON_RIGHT) {
        if (action == GLFW_PRESS) {
            rightButtonDown_ = true;
            lastMouseX_ = mouseX;
            lastMouseY_ = mouseY;
            firstMouse_ = true;
        } else if (action == GLFW_RELEASE) {
            rightButtonDown_ = false;
        }
    }
    
    if (button == GLFW_MOUSE_BUTTON_LEFT) {
        if (action == GLFW_PRESS) {
            leftButtonDown_ = true;
            lastMouseX_ = mouseX;
            lastMouseY_ = mouseY;
            firstMouse_ = true;
        } else if (action == GLFW_RELEASE) {
            leftButtonDown_ = false;
        }
    }
}

void CameraController::onMouseMove(double mouseX, double mouseY) {
    if (!enabled_) return;
    
    if (firstMouse_) {
        lastMouseX_ = mouseX;
        lastMouseY_ = mouseY;
        firstMouse_ = false;
        return;
    }
    
    double deltaX = mouseX - lastMouseX_;
    double deltaY = lastMouseY_ - mouseY; // Reversed: y coordinates go from bottom to top
    
    lastMouseX_ = mouseX;
    lastMouseY_ = mouseY;
    
    if (rightButtonDown_) {
        handleMouseLook(deltaX, deltaY);
    }
    
    if (leftButtonDown_) {
        handleMousePan(deltaX, deltaY);
    }
}

void CameraController::handleMouseLook(double deltaX, double deltaY) {
    deltaX *= state_.mouseSensitivity;
    deltaY *= state_.mouseSensitivity;
    
    state_.yaw += static_cast<float>(deltaX);
    state_.pitch += static_cast<float>(deltaY);
    
    // Constrain pitch to avoid gimbal lock
    state_.pitch = std::clamp(state_.pitch, -89.0f, 89.0f);
    
    // Update camera vectors
    state_.updateVectors();
}

void CameraController::handleMousePan(double deltaX, double deltaY) {
    float panAmount = static_cast<float>(state_.panSensitivity);
    
    // Pan in camera's right and up directions
    state_.position -= state_.right * static_cast<float>(deltaX) * panAmount;
    state_.position += state_.up * static_cast<float>(deltaY) * panAmount;
}

void CameraController::onMouseScroll(double yOffset) {
    if (!enabled_) return;
    
    // Adjust FOV for perspective, or size for orthographic
    if (state_.projectionType == CameraProjection::Perspective) {
        state_.fov -= static_cast<float>(yOffset) * 2.0f;
        state_.fov = std::clamp(state_.fov, 1.0f, 120.0f);
    } else {
        state_.orthoSize -= static_cast<float>(yOffset) * 0.5f;
        state_.orthoSize = std::max(state_.orthoSize, 0.1f);
    }
}

void CameraController::setShaderUniforms(unsigned int programId) const {
    GLint loc;
    
    // Camera position
    loc = glGetUniformLocation(programId, "uCameraPosition");
    if (loc != -1) glUniform3fv(loc, 1, glm::value_ptr(state_.position));
    
    // Camera forward, right, up vectors
    loc = glGetUniformLocation(programId, "uCameraForward");
    if (loc != -1) glUniform3fv(loc, 1, glm::value_ptr(state_.forward));
    
    loc = glGetUniformLocation(programId, "uCameraRight");
    if (loc != -1) glUniform3fv(loc, 1, glm::value_ptr(state_.right));
    
    loc = glGetUniformLocation(programId, "uCameraUp");
    if (loc != -1) glUniform3fv(loc, 1, glm::value_ptr(state_.up));
    
    // Camera target (position + forward)
    glm::vec3 target = state_.position + state_.forward;
    loc = glGetUniformLocation(programId, "uCameraTarget");
    if (loc != -1) glUniform3fv(loc, 1, glm::value_ptr(target));
    
    // Matrices
    glm::mat4 viewMatrix = state_.getViewMatrix();
    loc = glGetUniformLocation(programId, "uViewMatrix");
    if (loc != -1) glUniformMatrix4fv(loc, 1, GL_FALSE, glm::value_ptr(viewMatrix));
    
    glm::mat4 projMatrix = state_.getProjectionMatrix();
    loc = glGetUniformLocation(programId, "uProjectionMatrix");
    if (loc != -1) glUniformMatrix4fv(loc, 1, GL_FALSE, glm::value_ptr(projMatrix));
    
    glm::mat4 viewProjMatrix = state_.getViewProjectionMatrix();
    loc = glGetUniformLocation(programId, "uViewProjectionMatrix");
    if (loc != -1) glUniformMatrix4fv(loc, 1, GL_FALSE, glm::value_ptr(viewProjMatrix));
    
    // Camera parameters
    loc = glGetUniformLocation(programId, "uCameraFOV");
    if (loc != -1) glUniform1f(loc, state_.fov);
    
    loc = glGetUniformLocation(programId, "uCameraNear");
    if (loc != -1) glUniform1f(loc, state_.nearPlane);
    
    loc = glGetUniformLocation(programId, "uCameraFar");
    if (loc != -1) glUniform1f(loc, state_.farPlane);
}

void CameraController::reset() {
    state_ = Camera3DState();
    state_.updateVectors();
    Logger::Info("CameraController", "Camera reset to default position", {"camera"});
}

void CameraController::setAspectRatio(float aspectRatio) {
    state_.aspectRatio = aspectRatio;
}

CameraController::CameraUniformSupport CameraController::checkShaderSupport(unsigned int programId) {
    CameraUniformSupport support;
    
    support.position = glGetUniformLocation(programId, "uCameraPosition") != -1;
    support.forward = glGetUniformLocation(programId, "uCameraForward") != -1;
    support.right = glGetUniformLocation(programId, "uCameraRight") != -1;
    support.up = glGetUniformLocation(programId, "uCameraUp") != -1;
    support.target = glGetUniformLocation(programId, "uCameraTarget") != -1;
    support.viewMatrix = glGetUniformLocation(programId, "uViewMatrix") != -1;
    support.projectionMatrix = glGetUniformLocation(programId, "uProjectionMatrix") != -1;
    support.viewProjectionMatrix = glGetUniformLocation(programId, "uViewProjectionMatrix") != -1;
    support.fov = glGetUniformLocation(programId, "uCameraFOV") != -1;
    support.nearPlane = glGetUniformLocation(programId, "uCameraNear") != -1;
    support.farPlane = glGetUniformLocation(programId, "uCameraFar") != -1;
    
    return support;
}


#include "Camera.hpp"
#include "glm/ext/matrix_clip_space.hpp"
#include "glm/ext/matrix_transform.hpp"
#include "glm/gtc/constants.hpp"

#include <SDL_keycode.h>
#include <cstdint>
#include <imgui.h>
#include <SDL.h>

Camera::Camera() {}

Camera::~Camera() {}

void Camera::Init(float initialRadius, const glm::vec3& initialTarget) {}

void Camera::HandelMouseWheel(float yoffset) {
    radius -= yoffset * 0.1f * scrollSensitivity;
    if (radius < 0.25f) {
        radius = 0.25f;
    }
    shouldUpdate = true;
}

void Camera::HandelMouseMotion(uint32_t motionSate, float xrel, float yrel) {
    if (xrel == 0 && yrel == 0) {
        return;
    }
    bool hasNoModifiers =
        !(SDL_GetModState() & (KMOD_SHIFT | KMOD_CTRL | KMOD_ALT | KMOD_GUI));
    if (motionSate & SDL_BUTTON(SDL_BUTTON_RIGHT) && hasNoModifiers) {
        rotation.x += yrel * 0.01f * mouseSensitivity;
        rotation.y += xrel * -0.01f * mouseSensitivity;
        if (rotation.x > (glm::half_pi<float>() * 0.999f)) {
            rotation.x = glm::half_pi<float>() * 0.999f;
        } else if (rotation.x < -(glm::half_pi<float>() * 0.999f)) {
            rotation.x = -(glm::half_pi<float>() * 0.999f);
        }
        if (rotation.y > 2 * glm::pi<float>()) {
            rotation.y = 0;
        } else if (rotation.y < -2 * glm::pi<float>()) {
            rotation.y = 0;
        }
        shouldUpdate = true;
    }
    if (motionSate & SDL_BUTTON(SDL_BUTTON_RIGHT) &&
        SDL_GetModState() & KMOD_SHIFT) {
        // Get the camera's right and up vectors from the matrix
        glm::vec3 camR =
            glm::normalize(glm::vec3(camMat[0][0], camMat[1][0], camMat[2][0]));
        glm::vec3 camU =
            glm::normalize(glm::vec3(camMat[0][1], camMat[1][1], camMat[2][1]));
        glm::vec3 mouseDirWorld = camR * -xrel + camU * yrel;
        target += mouseDirWorld * 0.001f * radius;
        shouldUpdate = true;
    }
}

void Camera::RenderDebugWindow(bool* open) {
    if (open != NULL && !*open) {
        return;
    }
    ImGui::Begin("Cam debug info", open);
    ImGui::Text("CamPos: %.1f, %.1f, %.1f", camPos.x, camPos.y, camPos.z);
    ImGui::Text("Rotation: %.3f, %.3f", rotation.x, rotation.y);
    ImGui::End();
}

void Camera::Update(float aspectRatio) {
    if (!shouldUpdate) {
        return;
    }
    shouldUpdate = false;

    float pitch = rotation.x;
    float yaw = rotation.y;

    camPos.x = target.x + radius * cos(pitch) * sin(yaw);
    camPos.y = target.y + radius * sin(pitch);
    camPos.z = target.z + radius * cos(pitch) * cos(yaw);

    camMat = glm::lookAt(camPos, target, glm::vec3(0.0f, 1.0f, 0.0f));
    projMat = glm::perspective(glm::radians(fov), aspectRatio, 0.01f, 100.0f);

    invViewProj = glm::inverse(projMat * camMat);
}

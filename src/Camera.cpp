#include "Camera.hpp"
#include "glm/gtc/constants.hpp"

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
        if (rotation.x > glm::half_pi<float>()) {
            rotation.x = glm::half_pi<float>();
        } else if (rotation.x < -1.57f) {
            rotation.x = -glm::half_pi<float>();
        }
        if (rotation.y > 2 * glm::pi<float>()) {
            rotation.y = 0;
        } else if (rotation.y < -2 * glm::pi<float>()) {
            rotation.y = 0;
        }
        shouldUpdate = true;
    }
    if (motionSate & SDL_BUTTON(SDL_BUTTON_RIGHT) &&
        SDL_GetModState() & KMOD_CTRL) {
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
    ImGui::Text("CamF: %.1f, %.1f, %.1f", camF.x, camF.y, camF.z);
    ImGui::Text("CamR: %.1f, %.1f, %.1f", camR.x, camR.y, camR.z);
    ImGui::Text("CamU: %.1f, %.1f, %.1f", camU.x, camU.y, camU.z);
    ImGui::End();
}

void Camera::Update() {
    if (!shouldUpdate) {
        return;
    }
    shouldUpdate = false;

    float pitch = rotation.x;
    float yaw = rotation.y;

    camPos.x = target.x + radius * cos(pitch) * sin(yaw);
    camPos.y = target.y + radius * sin(pitch);
    camPos.z = target.z + radius * cos(pitch) * cos(yaw);

    camF = glm::normalize(target - camPos);
    camR = glm::normalize(glm::cross(camF, glm::vec3(0.0f, 1.0f, 0.0f)));
    camU = glm::cross(camR, camF);
    camMat = glm::mat3(camR, camU, -camF);
}

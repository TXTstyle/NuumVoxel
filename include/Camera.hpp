#pragma once

#include <glm/glm.hpp>

class Camera {
  private:
    float mouseSensitivity = 1.0f;
    float scrollSensitivity = 5.0f;

    float radius = 5.0f;
    glm::vec2 rotation = {0.0f, 0.0f};
    bool shouldUpdate = true;

    glm::vec3 camPos = {3.0f, 3.0f, 0.0f};
    glm::vec3 target = {0.0f, 1.0f, 0.0f};
    glm::vec3 camF = glm::normalize(target - camPos);
    glm::vec3 camR =
        glm::normalize(glm::cross(camF, glm::vec3(0.0f, 1.0f, 0.0f)));
    glm::vec3 camU = glm::cross(camR, camF);
    glm::mat3 camMat = glm::mat3(camR, camU, -camF);

  public:
    Camera();
    ~Camera();

    void Init(float initialRadius = 5.0f,
              const glm::vec3& initialTarget = {0.0f, 1.0f, 0.0f});
    void HandelMouseWheel(float yoffset);
    void HandelMouseMotion(uint32_t motionSate, float xrel, float yrel);

    void Update();
    void RenderDebugWindow(bool* open = nullptr);

    float& GetMouseSensitivity() { return mouseSensitivity; }
    float& GetScrollSensitivity() { return scrollSensitivity; }

    glm::vec3 GetPosition() const { return camPos; }
    glm::mat3 GetViewMatrix() const { return camMat; }
};

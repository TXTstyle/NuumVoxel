#pragma once

#include <glm/glm.hpp>

class Camera {
  private:
    float mouseSensitivity = 1.0f;
    float scrollSensitivity = 5.0f;

    float fov = 45.0f;
    float radius = 5.0f;
    glm::vec2 rotation = {0.0f, 0.0f};
    bool shouldUpdate = true;

    glm::vec3 camPos = {3.0f, 3.0f, 0.0f};
    glm::vec3 target = {0.0f, 1.0f, 0.0f};
    glm::mat4 camMat = glm::mat4(0.0f);
    glm::mat4 projMat = glm::mat4(0.0f);
    glm::mat4 invViewProj = glm::mat4(0.0f);

  public:
    Camera();
    ~Camera();

    void Init(float initialRadius = 5.0f,
              const glm::vec3& initialTarget = {0.0f, 1.0f, 0.0f});
    void HandelMouseWheel(float yoffset);
    void HandelMouseMotion(uint32_t motionSate, float xrel, float yrel);

    void Update(float aspectRatio);
    void RenderDebugWindow(bool* open = nullptr);
    inline const void SetUpdateState(bool state) { shouldUpdate = state; }

    inline float& GetMouseSensitivity() { return mouseSensitivity; }
    inline float& GetScrollSensitivity() { return scrollSensitivity; }
    inline float& GetFov() { return fov; }

    inline const glm::vec3 GetPosition() const { return camPos; }
    inline const glm::mat4 GetViewMatrix() const { return camMat; }
    inline const glm::mat4 GetProjectionMatrix() const { return projMat; }
    inline const glm::mat4 GetInvViewProj() const { return invViewProj; }
};

// Code from learnopengl
// https://github.com/JoeyDeVries/LearnOpenGL/blob/master/includes/learnopengl/camera.h

#pragma once

#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>

#include <vector>

// Defines several possible options for camera movement. Used as abstraction to
// stay away from window-system specific input methods
enum eCameraMovement {
  FORWARD,
  BACKWARD,
  LEFT,
  RIGHT,
  YAW_LEFT,
  YAW_RIGHT,
  PITCH_UP,
  PITCH_DOWN
};

// Default camera values
const float YAW = -90.0f;
const float PITCH = 0.0f;
const float SPEED = 1.5f;
const float ROTATION_SPEED = 100.0f;
const float HASTE_MULT = 5.0f;
const int DEFAULT_FOV = 60;
const float DEFAULT_NEAR = 0.01f;
const float DEFAULT_FAR = 100.0f;
const float DEFAULT_WIDTH = 800.0f;
const float DEFAULT_HEIGHT = 600.0f;

// An abstract camera class that processes input and calculates the
// corresponding Euler Angles, Vectors and Matrices for use in OpenGL
class Camera {
 public:
  // Camera Attributes
  glm::vec3 Position{glm::vec3(0.0f, 0.0f, 0.0f)};
  glm::vec3 Front;
  glm::vec3 Up{glm::vec3(0.0f, 0.0f, 1.0f)};
  glm::vec3 Right;
  glm::vec3 WorldUp{glm::vec3(0.0f, 0.0f, 1.0f)};
  // Euler Angles
  float Yaw{YAW};
  float Pitch{PITCH};
  // Camera options
  float MovementSpeed{SPEED};
  float Sensitivity{ROTATION_SPEED};

  float NearPlane{DEFAULT_NEAR};
  float FarPlane{DEFAULT_FAR};
  int FOV{DEFAULT_FOV};
  float Width{DEFAULT_WIDTH};
  float Height{DEFAULT_HEIGHT};

  bool IsPerspective{true};

  Camera() { updateCameraVectors(); }

  // Returns the view matrix calculated using Euler Angles and the LookAt Matrix
  glm::mat4 GetViewMatrix() const {
    return glm::lookAt(Position, Position + Front, Up);
  }

  glm::mat4 GetProjMatrix() const {
    if (IsPerspective)
      return glm::perspective(glm::radians(float(FOV)), Width / Height,
                              NearPlane, FarPlane);
    else
      return glm::ortho(-20.0f, 20.0f, -20.0f, 20.0f, NearPlane, FarPlane);
  }

  // Processes input received from any keyboard-like input system. Accepts input
  // parameter in the form of camera defined ENUM (to abstract it from windowing
  // systems)
  void ProcessKeyboard(eCameraMovement direction, float deltaTime,
                       bool isHaste) {
    float velocity = MovementSpeed * deltaTime;
    if (isHaste) velocity *= HASTE_MULT;

    if (direction == FORWARD) Position += Front * velocity;
    if (direction == BACKWARD) Position -= Front * velocity;
    if (direction == LEFT) Position += Right * velocity;
    if (direction == RIGHT) Position -= Right * velocity;
  }

  void ProcessRotation(eCameraMovement dir, float dt,
                       bool constrainPitch = true) {
    float yaw = 0.0f;
    float pitch = 0.0f;
    switch (dir) {
      case YAW_LEFT:
        yaw = dt * ROTATION_SPEED;
        break;
      case YAW_RIGHT:
        yaw = -dt * ROTATION_SPEED;
        break;
      case PITCH_UP:
        pitch = dt * ROTATION_SPEED;
        break;
      case PITCH_DOWN:
        pitch = -dt * ROTATION_SPEED;
        break;
      default:
        assert(0 || "invalid eCameraMovement");
        break;
    }

    Yaw += yaw;
    Pitch += pitch;

    // Make sure that when pitch is out of bounds, screen doesn't get flipped
    if (constrainPitch) {
      if (Pitch > 89.0f) Pitch = 89.0f;
      if (Pitch < -89.0f) Pitch = -89.0f;
    }

    // Update Front, Right and Up Vectors using the updated Euler angles
    updateCameraVectors();
  }

 private:
  // Calculates the front vector from the Camera's (updated) Euler Angles

  void swap() {
    std::swap(Front[1], Front[2]);
    std::swap(Up[1], Up[2]);
    std::swap(Right[1], Right[2]);
    std::swap(WorldUp[1], WorldUp[2]);
  }

  void updateCameraVectors() {
    // Calculate the new Front vector
    swap();
    glm::vec3 front;
    front.x = cos(glm::radians(Yaw)) * cos(glm::radians(Pitch));
    front.y = sin(glm::radians(Pitch));
    front.z = sin(glm::radians(Yaw)) * cos(glm::radians(Pitch));
    Front = glm::normalize(front);
    // Also re-calculate the Right and Up vector
    Right = glm::normalize(glm::cross(
        Front, WorldUp));  // Normalize the vectors, because their length gets
                           // closer to 0 the more you look up or down which
                           // results in slower movement.
    Up = glm::normalize(glm::cross(Right, Front));
    swap();
  }
};

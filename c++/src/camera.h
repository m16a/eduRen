// Code from learnopengl
// https://github.com/JoeyDeVries/LearnOpenGL/blob/master/includes/learnopengl/camera.h

#pragma once

#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>

#include <vector>

// Defines several possible options for camera movement. Used as abstraction to stay away from window-system specific input methods
enum Camera_Movement {
    FORWARD,
    BACKWARD,
    LEFT,
    RIGHT
};

// Default camera values
const float YAW							= -90.0f;
const float PITCH						=  0.0f;
const float SPEED						=  1.5f;
const float SENSITIVITY			=  0.1f;
const float HASTE_MULT			=  5.0f;
const float DEFAULT_FOV			=  60.0f;
const float DEFAULT_NEAR		=  0.01f;
const float DEFAULT_FAR			=  100.0f;
const float DEFAULT_WIDTH		=  800.0f;
const float DEFAULT_HEIGHT	=  600.0f;


// An abstract camera class that processes input and calculates the corresponding Euler Angles, Vectors and Matrices for use in OpenGL
class Camera
{
public:
    // Camera Attributes
    glm::vec3 Position	{glm::vec3(0.0f, 0.0f, 0.0f)};
    glm::vec3 Front;
    glm::vec3 Up				{glm::vec3(0.0f, 0.0f, 1.0f)};
    glm::vec3 Right;
    glm::vec3 WorldUp {glm::vec3(0.0f, 0.0f, 1.0f)};
    // Euler Angles
    float Yaw {YAW};
    float Pitch {PITCH};
    // Camera options
    float MovementSpeed {SPEED};
    float MouseSensitivity {SENSITIVITY};

		float NearPlane {DEFAULT_NEAR};
		float FarPlane {DEFAULT_FAR};
		float FOV {DEFAULT_FOV};
		float Width {DEFAULT_WIDTH};
		float Height {DEFAULT_HEIGHT};

    Camera()
    {
        updateCameraVectors();
    }

    // Returns the view matrix calculated using Euler Angles and the LookAt Matrix
    glm::mat4 GetViewMatrix() const
    {
        return glm::lookAt(Position, Position + Front, Up);
    }

    glm::mat4 GetProjMatrix() const
    {
				return glm::perspective(glm::radians(FOV), Width / Height, NearPlane, FarPlane);
    }

    // Processes input received from any keyboard-like input system. Accepts input parameter in the form of camera defined ENUM (to abstract it from windowing systems)
    void ProcessKeyboard(Camera_Movement direction, float deltaTime, bool isHaste)
    {
        float velocity = MovementSpeed * deltaTime;
				if (isHaste)
					velocity *= HASTE_MULT;

        if (direction == FORWARD)
            Position += Front * velocity;
        if (direction == BACKWARD)
            Position -= Front * velocity;
        if (direction == LEFT)
            Position += Right * velocity;
        if (direction == RIGHT)
            Position -= Right * velocity;
    }

    // Processes input received from a mouse input system. Expects the offset value in both the x and y direction.
    void ProcessMouseMovement(float xoffset, float yoffset, bool constrainPitch = true)
    {
        xoffset *= MouseSensitivity;
        yoffset *= MouseSensitivity;

        Yaw   -= xoffset;
        Pitch += yoffset;

        // Make sure that when pitch is out of bounds, screen doesn't get flipped
        if (constrainPitch)
        {
            if (Pitch > 89.0f)
                Pitch = 89.0f;
            if (Pitch < -89.0f)
                Pitch = -89.0f;
        }

        // Update Front, Right and Up Vectors using the updated Euler angles
        updateCameraVectors();
    }
private:
    // Calculates the front vector from the Camera's (updated) Euler Angles
	
		void swap()
		{
			std::swap(Front[1], Front[2]);
			std::swap(Up[1], Up[2]);
			std::swap(Right[1], Right[2]);
			std::swap(WorldUp[1], WorldUp[2]);
		}

    void updateCameraVectors()
    {
        // Calculate the new Front vector
				swap();
        glm::vec3 front;
        front.x = cos(glm::radians(Yaw)) * cos(glm::radians(Pitch));
        front.y = sin(glm::radians(Pitch));
        front.z = sin(glm::radians(Yaw)) * cos(glm::radians(Pitch));
        Front = glm::normalize(front);
        // Also re-calculate the Right and Up vector
        Right = glm::normalize(glm::cross(Front, WorldUp));  // Normalize the vectors, because their length gets closer to 0 the more you look up or down which results in slower movement.
        Up    = glm::normalize(glm::cross(Right, Front));
				swap();
    }
};


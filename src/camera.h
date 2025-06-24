#pragma once

#include <vk_types.h>
#include <SDL3/SDL_events.h>

class Camera {
public: 
	glm::vec3 velocity;
	glm::vec3 position;

	// Vertical rotation 
	float pitch{ 0.f };

	// Horizontal rotation
	float yaw{ 0.f };

	bool is_mouse_mode_relative = false;

	glm::mat4 getViewMatrix();
	glm::mat4 getRotationMatrix();

	void processSDLEvent(SDL_Event& e);

	void update();
};
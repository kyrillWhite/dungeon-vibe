#pragma once

#include <GLFW/glfw3.h>
#include <string>
#include <iostream>
#include "Display.h"
#include "Camera.h"
#include "Settings.h"
#include "Map.h"

extern Display display;
extern Camera camera;
extern Map dungeon;

// Debug panel state (inline to allow header-only usage)
inline bool debugActive = false;

inline void framebuffer_size_callback(GLFWwindow* window, int width, int height) {
	display.calculateScale(width, height);
}

inline void mouse_callback(GLFWwindow* window, double xpos, double ypos) {
	camera.processMouse(xpos);
}

inline void key_callback(GLFWwindow* window, int key, int scancode, int action, int mods) {
	if (key == GLFW_KEY_ESCAPE && action == GLFW_PRESS) {
		camera.isCursorLocked = !camera.isCursorLocked;
		if (camera.isCursorLocked) {
			glfwSetInputMode(window, GLFW_CURSOR, GLFW_CURSOR_DISABLED);
			camera.firstMouse = true;
		} else {
			glfwSetInputMode(window, GLFW_CURSOR, GLFW_CURSOR_NORMAL);
		}
	}

	#ifdef GAME_DEBUG
	// Toggle debig visibility with grave/backtick accent key
	if (key == GLFW_KEY_GRAVE_ACCENT && action == GLFW_PRESS) {
		debugActive = !debugActive;
		if (debugActive) {
			glfwSetInputMode(window, GLFW_CURSOR, GLFW_CURSOR_NORMAL);
			camera.isCursorLocked = false;
		} else {
			glfwSetInputMode(window, GLFW_CURSOR, camera.isCursorLocked ? GLFW_CURSOR_DISABLED : GLFW_CURSOR_NORMAL);
		}
	}
	#endif

	if (key == GLFW_MOUSE_BUTTON_LEFT && action == GLFW_PRESS) {
		if (!camera.isCursorLocked && !debugActive) {
			camera.isCursorLocked = true;
			glfwSetInputMode(window, GLFW_CURSOR, GLFW_CURSOR_DISABLED);
			camera.firstMouse = true;
		}
	}
}

inline void mouse_button_callback(GLFWwindow* window, int button, int action, int mods) {
	if (button == GLFW_MOUSE_BUTTON_LEFT && action == GLFW_PRESS) {
		if (!camera.isCursorLocked && !debugActive) {
			camera.isCursorLocked = true;
			glfwSetInputMode(window, GLFW_CURSOR, GLFW_CURSOR_DISABLED);
			camera.firstMouse = true;
		}
	}
}

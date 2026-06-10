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

// Console state (inline to allow header-only usage)
inline bool consoleActive = false;
inline std::string consoleInput;

inline void framebuffer_size_callback(GLFWwindow* window, int width, int height) {
	display.calculateScale(width, height);
}

inline void mouse_callback(GLFWwindow* window, double xpos, double ypos) {
	camera.processMouse(xpos);
}

// Character input for console (when console is active)
inline void char_callback(GLFWwindow* window, unsigned int codepoint) {
	if (!consoleActive) return;
	if (codepoint >= 32 && codepoint <= 126) { // printable
		consoleInput.push_back(static_cast<char>(codepoint));
		std::cout << static_cast<char>(codepoint);
		std::cout.flush();
	}
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

	// Toggle console with grave/backtick
	if (key == GLFW_KEY_GRAVE_ACCENT && action == GLFW_PRESS) {
		consoleActive = !consoleActive;
		if (consoleActive) {
			glfwSetInputMode(window, GLFW_CURSOR, GLFW_CURSOR_NORMAL);
			camera.isCursorLocked = false;
			consoleInput.clear();
			std::cout << "\nConsole> " << std::flush;
		} else {
			glfwSetInputMode(window, GLFW_CURSOR, camera.isCursorLocked ? GLFW_CURSOR_DISABLED : GLFW_CURSOR_NORMAL);
			std::cout << "\nConsole closed\n";
		}
	}

	// Backspace handling when console is active
	if (consoleActive && key == GLFW_KEY_BACKSPACE && (action == GLFW_PRESS || action == GLFW_REPEAT)) {
		if (!consoleInput.empty()) {
			consoleInput.pop_back();
			std::cout << "\b \b" << std::flush;
		}
	}

	// Enter to execute command
	if (consoleActive && key == GLFW_KEY_ENTER && action == GLFW_PRESS) {
		std::cout << "\n";
		if (!consoleInput.empty()) {
			std::string res = Settings::execConsoleCommand(consoleInput);
			std::cout << res << "\n";
		}
		consoleInput.clear();
		std::cout << "Console> " << std::flush;
	}

	// Keep legacy left-click lock (if detected via key callback)
	if (key == GLFW_MOUSE_BUTTON_LEFT && action == GLFW_PRESS) {
		if (!camera.isCursorLocked) {
			camera.isCursorLocked = true;
			glfwSetInputMode(window, GLFW_CURSOR, GLFW_CURSOR_DISABLED);
			camera.firstMouse = true;
		}
	}
}

inline void mouse_button_callback(GLFWwindow* window, int button, int action, int mods) {
	if (button == GLFW_MOUSE_BUTTON_LEFT && action == GLFW_PRESS) {
		if (!camera.isCursorLocked) {
			camera.isCursorLocked = true;
			glfwSetInputMode(window, GLFW_CURSOR, GLFW_CURSOR_DISABLED);
			camera.firstMouse = true;
		}
	}
}

#pragma once

#include <GLFW/glfw3.h>
#include <memory>
#include <functional>
#include <vector>

class Input : public std::enable_shared_from_this<Input>
{
public:
	using KeyCallback = std::function<void(int key, int scancode, int action, int mods)>;
	using MouseMoveCallback = std::function<void(double xpos, double ypos)>;
	using MouseButtonCallback = std::function<void(int button, int action, int mods)>;
	using FramebufferSizeCallback = std::function<void(int width, int height)>;

	void init(GLFWwindow *window)
	{
		this->window = window;
		selfRef = shared_from_this();
		glfwSetWindowUserPointer(window, &selfRef);

		glfwSetFramebufferSizeCallback(window, [](GLFWwindow *w, int width, int height)
									   {
			if (auto input = get(w)) {
				for (const auto& cb : input->framebufferSizeSubscribers) cb(width, height);
			} });

		glfwSetCursorPosCallback(window, [](GLFWwindow *w, double x, double y)
								 {
			if (auto input = get(w)) {
				for (const auto& cb : input->mouseMoveSubscribers) cb(x, y);
			} });

		glfwSetKeyCallback(window, [](GLFWwindow *w, int k, int s, int a, int m)
						   {
			if (auto input = get(w)) {
				for (const auto& cb : input->keySubscribers) cb(k, s, a, m);
			} });

		glfwSetMouseButtonCallback(window, [](GLFWwindow *w, int b, int a, int m)
								   {
			if (auto input = get(w)) {
				for (const auto& cb : input->mouseButtonSubscribers) cb(b, a, m);
			} });
	}

	bool isKeyPressed(int key) const
	{
		return window && glfwGetKey(window, key) == GLFW_PRESS;
	}

	void subscribeToKey(KeyCallback cb) { keySubscribers.push_back(cb); }
	void subscribeToMouseMove(MouseMoveCallback cb) { mouseMoveSubscribers.push_back(cb); }
	void subscribeToMouseButton(MouseButtonCallback cb) { mouseButtonSubscribers.push_back(cb); }
	void subscribeToFramebufferSize(FramebufferSizeCallback cb) { framebufferSizeSubscribers.push_back(cb); }

private:
	GLFWwindow *window = nullptr;
	std::shared_ptr<Input> selfRef;

	std::vector<KeyCallback> keySubscribers;
	std::vector<MouseMoveCallback> mouseMoveSubscribers;
	std::vector<MouseButtonCallback> mouseButtonSubscribers;
	std::vector<FramebufferSizeCallback> framebufferSizeSubscribers;

	static std::shared_ptr<Input> get(GLFWwindow *window)
	{
		auto *ptrRef = static_cast<std::shared_ptr<Input> *>(glfwGetWindowUserPointer(window));
		return (ptrRef && *ptrRef) ? *ptrRef : nullptr;
	}
};

#pragma once

#include <GLFW/glfw3.h>
#ifdef GAME_DEBUG
#include <imgui.h>
#include <backends/imgui_impl_glfw.h>
#include <backends/imgui_impl_opengl3.h>
#endif

#include "Camera.h"

class Window
{
public:
    bool debugActive = false;

    int init(int width, int height)
    {
        if (!glfwInit())
        {
            return -1;
        }

        glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 3);
        glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 3);
        glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE);

        window = glfwCreateWindow(width, height, "Dungeon Vibe", NULL, NULL);
        if (!window)
        {
            glfwTerminate();
            return -1;
        }

        glfwMakeContextCurrent(window);
        glfwSetInputMode(window, GLFW_CURSOR, GLFW_CURSOR_DISABLED);

        return 0;
    }

    void initImGui()
    {
#ifdef GAME_DEBUG
        IMGUI_CHECKVERSION();
        ImGui::CreateContext();
        ImGuiIO &io = ImGui::GetIO();
        (void)io;
        ImGui::StyleColorsDark();
        ImGui_ImplGlfw_InitForOpenGL(window, true);
        ImGui_ImplOpenGL3_Init("#version 130");
#endif
    }

    bool isShouldClose()
    {
        return glfwWindowShouldClose(window);
    }

    void swapBuffers()
    {
        glfwSwapBuffers(window);
    }

    GLFWwindow *getWindowPtr()
    {
        return window;
    }

    void handleKey(std::shared_ptr<Camera> camera, int key, int action)
    {
        if (key == GLFW_KEY_ESCAPE && action == GLFW_PRESS)
        {
            camera->isCursorLocked = !camera->isCursorLocked;
            if (camera->isCursorLocked)
            {
                disableCursor();
                camera->firstMouse = true;
            }
            else
            {
                enableCursor();
            }
        }

#ifdef GAME_DEBUG
        // Toggle debug visibility with grave/backtick accent key
        if (key == GLFW_KEY_GRAVE_ACCENT && action == GLFW_PRESS)
        {
            debugActive = !debugActive;
            if (debugActive)
            {
                enableCursor();
                camera->isCursorLocked = false;
            }
            else
            {
                camera->isCursorLocked ? disableCursor() : enableCursor();
            }
        }
#endif
    }

    void handleMouseButton(std::shared_ptr<Camera> camera, int button, int action)
    {
        if (button == GLFW_MOUSE_BUTTON_LEFT && action == GLFW_PRESS)
        {
            if (!camera->isCursorLocked && !debugActive)
            {
                camera->isCursorLocked = true;
                glfwSetInputMode(window, GLFW_CURSOR, GLFW_CURSOR_DISABLED);
                camera->firstMouse = true;
            }
        }
    }

private:
    GLFWwindow *window;

    void enableCursor()
    {
        glfwSetInputMode(window, GLFW_CURSOR, GLFW_CURSOR_NORMAL);
    }

    void disableCursor()
    {
        glfwSetInputMode(window, GLFW_CURSOR, GLFW_CURSOR_DISABLED);
    }
};

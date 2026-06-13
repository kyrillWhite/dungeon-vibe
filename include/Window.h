#pragma once

#include <GLFW/glfw3.h>

class Window
{
public:
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

private:
    GLFWwindow *window;
};

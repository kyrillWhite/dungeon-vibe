#pragma once

#include <GL/glew.h>
#include <GLFW/glfw3.h>
#include <memory>
#include <random>
#include <ctime>
#include <cmath>
#include <algorithm>

#include "Window.h"
#include "Input.h"
#include "Camera.h"
#include "Display.h"
#include "Map.h"
#include "Minimap.h"
#include "TextureManager.h"
#include "Settings.h"
#include "Renderer.h"
#include "Torch.h"

#ifdef GAME_DEBUG
#include <imgui.h>
#include <backends/imgui_impl_glfw.h>
#include <backends/imgui_impl_opengl3.h>
#include "DevPanel.h"
#endif

class Application
{
public:
    Application() {}

    ~Application() {}

    bool init()
    {
        window = std::make_shared<Window>();
        input = std::make_shared<Input>();
        camera = std::make_shared<Camera>();
        display = std::make_shared<Display>();
        dungeon = std::make_shared<Map>();
        minimap = std::make_shared<Minimap>();
        textures = std::make_shared<TextureManager>();
        torch = std::make_unique<Torch>();

        if (window->init(display->winW, display->winH))
        {
            return false;
        }

        glewExperimental = GL_TRUE;
        if (glewInit() != GLEW_OK)
        {
            return false;
        }

        input->init(window->getWindowPtr());
        window->initImGui();

        input->subscribeToMouseMove(std::bind_front(&Camera::processMouse, camera));
        input->subscribeToFramebufferSize(std::bind_front(&Display::calculateScale, display));

        input->subscribeToKey([this](int key, int scancode, int action, int mods)
                              { window->handleKey(camera, key, action); });

        input->subscribeToMouseButton([this](int button, int action, int mods)
                                      { window->handleMouseButton(camera, button, action); });

        dungeon->generate();
        camera->pos = dungeon->spawnPos;
        display->calculateScale(display->winW, display->winH);

        textures->init();
        Settings::loadFromFile();

        pipeline = std::make_unique<Renderer>(display, textures);
        if (!pipeline->init(dungeon))
        {
            return false;
        }

        return true;
    }

    void run()
    {
        float deltaTime = 0.0f;
        float lastFrame = 0.0f;

        glEnable(GL_BLEND);
        glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);

        while (!window->isShouldClose())
        {
            float currentFrame = static_cast<float>(glfwGetTime());
            deltaTime = currentFrame - lastFrame;
            lastFrame = currentFrame;

            camera->processInput(
                input->isKeyPressed(GLFW_KEY_W),
                input->isKeyPressed(GLFW_KEY_S),
                input->isKeyPressed(GLFW_KEY_A),
                input->isKeyPressed(GLFW_KEY_D),
                deltaTime,
                dungeon->grid);

            dungeon->updateVisibility(camera->pos, 6, 120);

            torch->update(deltaTime);

            bool isTabPressed = input->isKeyPressed(GLFW_KEY_TAB);

            pipeline->renderScene(dungeon, camera, minimap, isTabPressed, torch->getFlicker());

#ifdef GAME_DEBUG
            ImGui_ImplOpenGL3_NewFrame();
            ImGui_ImplGlfw_NewFrame();
            ImGui::NewFrame();

            if (window->debugActive)
            {
                DevPanel::get().draw(&window->debugActive);
            }

            ImGui::Render();
            ImGui_ImplOpenGL3_RenderDrawData(ImGui::GetDrawData());
#endif

            window->swapBuffers();
            glfwPollEvents();
        }

        Settings::saveToFile();

        minimap->cleanup();
        textures->cleanup();
        pipeline->cleanup();

        glfwTerminate();
    }

private:
    std::shared_ptr<Window> window;
    std::shared_ptr<Input> input;
    std::shared_ptr<Camera> camera;
    std::shared_ptr<Display> display;
    std::shared_ptr<Map> dungeon;
    std::shared_ptr<Minimap> minimap;
    std::shared_ptr<TextureManager> textures;

    std::unique_ptr<Renderer> pipeline;
    std::unique_ptr<Torch> torch;
};

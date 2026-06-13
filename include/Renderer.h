#pragma once

#include <GL/glew.h>
#include <GLFW/glfw3.h>
#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtc/type_ptr.hpp>
#include <iostream>
#include <memory>
#include <string>
#include <vector>

#include "shaders/FragmentShader.h"
#include "shaders/ScreenFragmentShader.h"
#include "shaders/ScreenVertexShader.h"
#include "shaders/SsaoFragmentShaderSource.h"
#include "shaders/VertexShader.h"

#include "Camera.h"
#include "Display.h"
#include "Map.h"
#include "Minimap.h"
#include "Settings.h"
#include "TextureManager.h"

class Renderer
{
public:
    Renderer(std::shared_ptr<Display> display, std::shared_ptr<TextureManager> textures)
        : display(display), textures(textures)
    {
    }

    ~Renderer()
    {
        cleanup();
    }

    bool init(std::shared_ptr<Map> dungeon)
    {
        shader3D = compileShaderPipeline(vertexShaderSource, fragmentShaderSource);
        shader2D = compileShaderPipeline(screenVertexShaderSource, screenFragmentShaderSource);
        shaderSSAO = compileShaderPipeline(screenVertexShaderSource, ssaoFragmentShaderSource);

        if (shader3D == 0 || shader2D == 0 || shaderSSAO == 0)
        {
            std::cerr << "Failed to compile shader pipelines\n";
            return false;
        }

        initGeometryBuffers(dungeon);
        applyAnisotropicFiltering();
        initFramebuffers();

        return true;
    }

    void renderScene(std::shared_ptr<Map> dungeon, std::shared_ptr<Camera> camera,
                     std::shared_ptr<Minimap> minimap, bool isTabPressed, float torchFlicker)
    {
        // --- STAGE 1: RENDER 3D TO LOW RESOLUTION ---
        glBindFramebuffer(GL_FRAMEBUFFER, framebuffer);
        glViewport(0, 0, GAME_WIDTH, GAME_HEIGHT);
        glEnable(GL_DEPTH_TEST);
        glClearColor(0.0f, 0.0f, 0.0f, 1.0f);
        glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

        glUseProgram(shader3D);
        glm::mat4 model = glm::mat4(1.0f);
        glm::mat4 view = glm::lookAt(camera->pos, camera->pos + camera->front, camera->up);
        float aspect = static_cast<float>(GAME_WIDTH) / static_cast<float>(GAME_HEIGHT);
        glm::mat4 projection = glm::perspective(glm::radians(Settings::FOV_DEGREES), aspect, 0.05f, 100.0f);

        glUniformMatrix4fv(glGetUniformLocation(shader3D, "model"), 1, GL_FALSE, glm::value_ptr(model));
        glUniformMatrix4fv(glGetUniformLocation(shader3D, "view"), 1, GL_FALSE, glm::value_ptr(view));
        glUniformMatrix4fv(glGetUniformLocation(shader3D, "projection"), 1, GL_FALSE, glm::value_ptr(projection));

        glUniform3fv(glGetUniformLocation(shader3D, "cameraPos"), 1, glm::value_ptr(camera->pos));
        glUniform1f(glGetUniformLocation(shader3D, "lightNear"), Settings::LIGHT_RADIUS_NEAR);
        glUniform1f(glGetUniformLocation(shader3D, "lightFar"), Settings::LIGHT_RADIUS_FAR);
        glUniform1f(glGetUniformLocation(shader3D, "ambientLight"), Settings::AMBIENT_LIGHT);
        glUniform1f(glGetUniformLocation(shader3D, "lightSharpness"), Settings::LIGHT_SHARPNESS);
        glUniform1f(glGetUniformLocation(shader3D, "lightTransitionSoftness"), Settings::LIGHT_TRANSITION_SOFTNESS);
        glUniform1f(glGetUniformLocation(shader3D, "aoLightBlend"), Settings::AO_LIGHT_BLEND);
        glUniform1i(glGetUniformLocation(shader3D, "debugWhitebox"), Settings::DEBUG_WHITEBOX_MODE ? 1 : 0);

        glm::vec3 torchColor(1.0f, 0.55f, 0.18f);
        glUniform3fv(glGetUniformLocation(shader3D, "torchColor"), 1, glm::value_ptr(torchColor));
        glUniform1f(glGetUniformLocation(shader3D, "torchFlicker"), torchFlicker);

        glActiveTexture(GL_TEXTURE0);
        glBindTexture(GL_TEXTURE_2D, textures->textures["wall"]);
        glUniform1i(glGetUniformLocation(shader3D, "texSampler"), 0);
        glBindVertexArray(wallVAO);
        glDrawArrays(GL_TRIANGLES, 0, static_cast<GLsizei>(dungeon->wallVertices.size() / 6));

        glBindTexture(GL_TEXTURE_2D, textures->textures["floor"]);
        glUniform1i(glGetUniformLocation(shader3D, "texSampler"), 0);
        glBindVertexArray(floorVAO);
        glDrawArrays(GL_TRIANGLES, 0, static_cast<GLsizei>(dungeon->floorCeilVertices.size() / 6));

        // --- STAGE 2.5: MINIMAP OVERLAY AT LOW RESOLUTION ---
        glDisable(GL_DEPTH_TEST);
        glDepthMask(GL_FALSE);
        glUseProgram(shader2D);
        glUniform1i(glGetUniformLocation(shader2D, "useTexture"), 0);
        float currentAlpha = isTabPressed ? 0.5f : 0.75f;
        glUniform1f(glGetUniformLocation(shader2D, "mapAlpha"), currentAlpha);

        minimap->render(shader2D, dungeon, camera, isTabPressed, currentAlpha, GAME_WIDTH, GAME_HEIGHT);

        glDepthMask(GL_TRUE);
        glEnable(GL_DEPTH_TEST);

        // --- STAGE 2: SSAO / DITHER PASS ---
        glBindFramebuffer(GL_FRAMEBUFFER, ssaoFBO);
        glViewport(0, 0, GAME_WIDTH, GAME_HEIGHT);
        glDisable(GL_DEPTH_TEST);
        glClearColor(0.0f, 0.0f, 0.0f, 1.0f);
        glClear(GL_COLOR_BUFFER_BIT);

        glUseProgram(shaderSSAO);
        glActiveTexture(GL_TEXTURE0);
        glBindTexture(GL_TEXTURE_2D, textureColorBuffer);
        glUniform1i(glGetUniformLocation(shaderSSAO, "colorTex"), 0);

        glActiveTexture(GL_TEXTURE1);
        glBindTexture(GL_TEXTURE_2D, depthTexture);
        glUniform1i(glGetUniformLocation(shaderSSAO, "depthTex"), 1);

        glUniform1f(glGetUniformLocation(shaderSSAO, "projNear"), 0.05f);
        glUniform1f(glGetUniformLocation(shaderSSAO, "projFar"), 100.0f);
        glUniform1f(glGetUniformLocation(shaderSSAO, "ambientLight"), Settings::AMBIENT_LIGHT);
        glUniform1f(glGetUniformLocation(shaderSSAO, "aoLightBlend"), Settings::AO_LIGHT_BLEND);
        glUniform1i(glGetUniformLocation(shaderSSAO, "useDither"), Settings::USE_DITHER);
        glUniform1i(glGetUniformLocation(shaderSSAO, "ditherPalette"), Settings::DITHER_PALETTE);
        glUniform2f(glGetUniformLocation(shaderSSAO, "screenSize"), static_cast<float>(GAME_WIDTH), static_cast<float>(GAME_HEIGHT));

        glBindVertexArray(quadVAO);
        glDrawArrays(GL_TRIANGLES, 0, 6);

        // --- STAGE 3: UPSCALE TO SCREEN ---
        glBindFramebuffer(GL_FRAMEBUFFER, 0);
        glViewport(display->renderX, display->renderY, display->renderW, display->renderH);
        glClearColor(0.0f, 0.0f, 0.0f, 1.0f);
        glClear(GL_COLOR_BUFFER_BIT);

        glUseProgram(shader2D);
        glUniform1i(glGetUniformLocation(shader2D, "useTexture"), 1);
        glUniform1i(glGetUniformLocation(shader2D, "screenTexture"), 0);
        glUniform1f(glGetUniformLocation(shader2D, "screenDistortion"), Settings::SCREEN_DISTORTION);
        float screenAspect = static_cast<float>(display->renderW) / static_cast<float>(display->renderH);
        glUniform1f(glGetUniformLocation(shader2D, "screenAspect"), screenAspect);

        glActiveTexture(GL_TEXTURE0);
        glBindTexture(GL_TEXTURE_2D, aoTexture);
        glBindVertexArray(quadVAO);
        glDrawArrays(GL_TRIANGLES, 0, 6);
    }

    void cleanup()
    {
        if (wallVAO)
        {
            glDeleteVertexArrays(1, &wallVAO);
            wallVAO = 0;
        }
        if (wallVBO)
        {
            glDeleteBuffers(1, &wallVBO);
            wallVBO = 0;
        }
        if (floorVAO)
        {
            glDeleteVertexArrays(1, &floorVAO);
            floorVAO = 0;
        }
        if (floorVBO)
        {
            glDeleteBuffers(1, &floorVBO);
            floorVBO = 0;
        }
        if (quadVAO)
        {
            glDeleteVertexArrays(1, &quadVAO);
            quadVAO = 0;
        }
        if (quadVBO)
        {
            glDeleteBuffers(1, &quadVBO);
            quadVBO = 0;
        }

        if (framebuffer)
        {
            glDeleteFramebuffers(1, &framebuffer);
            framebuffer = 0;
        }
        if (ssaoFBO)
        {
            glDeleteFramebuffers(1, &ssaoFBO);
            ssaoFBO = 0;
        }

        if (textureColorBuffer)
        {
            glDeleteTextures(1, &textureColorBuffer);
            textureColorBuffer = 0;
        }
        if (aoTexture)
        {
            glDeleteTextures(1, &aoTexture);
            aoTexture = 0;
        }
        if (depthTexture)
        {
            glDeleteTextures(1, &depthTexture);
            depthTexture = 0;
        }

        if (shader3D)
        {
            glDeleteProgram(shader3D);
            shader3D = 0;
        }
        if (shader2D)
        {
            glDeleteProgram(shader2D);
            shader2D = 0;
        }
        if (shaderSSAO)
        {
            glDeleteProgram(shaderSSAO);
            shaderSSAO = 0;
        }
    }

private:
    unsigned int compileShaderPipeline(const char *vertexSrc, const char *fragmentSrc)
    {
        unsigned int vertexShader = glCreateShader(GL_VERTEX_SHADER);
        glShaderSource(vertexShader, 1, &vertexSrc, NULL);
        glCompileShader(vertexShader);
        checkCompileErrors(vertexShader, "VERTEX");

        unsigned int fragmentShader = glCreateShader(GL_FRAGMENT_SHADER);
        glShaderSource(fragmentShader, 1, &fragmentSrc, NULL);
        glCompileShader(fragmentShader);
        checkCompileErrors(fragmentShader, "FRAGMENT");

        unsigned int program = glCreateProgram();
        glAttachShader(program, vertexShader);
        glAttachShader(program, fragmentShader);
        glLinkProgram(program);
        checkCompileErrors(program, "PROGRAM");

        glDeleteShader(vertexShader);
        glDeleteShader(fragmentShader);

        return program;
    }

    void checkCompileErrors(unsigned int shader, std::string type)
    {
        int success;
        char infoLog[1024];
        if (type != "PROGRAM")
        {
            glGetShaderiv(shader, GL_COMPILE_STATUS, &success);
            if (!success)
            {
                glGetShaderInfoLog(shader, 1024, NULL, infoLog);
                std::cerr << "ERROR::SHADER_COMPILATION_ERROR of type: " << type << "\n"
                          << infoLog << "\n";
            }
        }
        else
        {
            glGetProgramiv(shader, GL_LINK_STATUS, &success);
            if (!success)
            {
                glGetProgramInfoLog(shader, 1024, NULL, infoLog);
                std::cerr << "ERROR::PROGRAM_LINKING_ERROR of type: " << type << "\n"
                          << infoLog << "\n";
            }
        }
    }

    void initGeometryBuffers(std::shared_ptr<Map> dungeon)
    {
        glGenVertexArrays(1, &wallVAO);
        glGenBuffers(1, &wallVBO);
        glBindVertexArray(wallVAO);
        glBindBuffer(GL_ARRAY_BUFFER, wallVBO);
        glBufferData(GL_ARRAY_BUFFER, dungeon->wallVertices.size() * sizeof(float), dungeon->wallVertices.data(), GL_STATIC_DRAW);
        glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 6 * sizeof(float), (void *)0);
        glEnableVertexAttribArray(0);
        glVertexAttribPointer(1, 2, GL_FLOAT, GL_FALSE, 6 * sizeof(float), (void *)(3 * sizeof(float)));
        glEnableVertexAttribArray(1);
        glVertexAttribPointer(2, 1, GL_FLOAT, GL_FALSE, 6 * sizeof(float), (void *)(5 * sizeof(float)));
        glEnableVertexAttribArray(2);

        glGenVertexArrays(1, &floorVAO);
        glGenBuffers(1, &floorVBO);
        glBindVertexArray(floorVAO);
        glBindBuffer(GL_ARRAY_BUFFER, floorVBO);
        glBufferData(GL_ARRAY_BUFFER, dungeon->floorCeilVertices.size() * sizeof(float), dungeon->floorCeilVertices.data(), GL_STATIC_DRAW);
        glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 6 * sizeof(float), (void *)0);
        glEnableVertexAttribArray(0);
        glVertexAttribPointer(1, 2, GL_FLOAT, GL_FALSE, 6 * sizeof(float), (void *)(3 * sizeof(float)));
        glEnableVertexAttribArray(1);
        glVertexAttribPointer(2, 1, GL_FLOAT, GL_FALSE, 6 * sizeof(float), (void *)(5 * sizeof(float)));
        glEnableVertexAttribArray(2);

        float quadVertices[] = {
            -1.0f, 1.0f, 0.0f, 0.0f, 1.0f,
            -1.0f, -1.0f, 0.0f, 0.0f, 0.0f,
            1.0f, -1.0f, 0.0f, 1.0f, 0.0f,
            -1.0f, 1.0f, 0.0f, 0.0f, 1.0f,
            1.0f, -1.0f, 0.0f, 1.0f, 0.0f,
            1.0f, 1.0f, 0.0f, 1.0f, 1.0f};

        glGenVertexArrays(1, &quadVAO);
        glGenBuffers(1, &quadVBO);
        glBindVertexArray(quadVAO);
        glBindBuffer(GL_ARRAY_BUFFER, quadVBO);
        glBufferData(GL_ARRAY_BUFFER, sizeof(quadVertices), quadVertices, GL_STATIC_DRAW);
        glVertexAttribPointer(0, 2, GL_FLOAT, GL_FALSE, 5 * sizeof(float), (void *)0);
        glEnableVertexAttribArray(0);
        glVertexAttribPointer(1, 3, GL_FLOAT, GL_FALSE, 5 * sizeof(float), (void *)(2 * sizeof(float)));
        glEnableVertexAttribArray(1);
    }

    void applyAnisotropicFiltering()
    {
        GLfloat maxAniso = 0.0f;
        if (glewIsSupported("GL_EXT_texture_filter_anisotropic"))
        {
            glGetFloatv(GL_MAX_TEXTURE_MAX_ANISOTROPY_EXT, &maxAniso);
            float aniso = Settings::ANISOTROPY_LEVEL;
            if (aniso > maxAniso)
                aniso = maxAniso;
            if (aniso < 1.0f)
                aniso = 1.0f;

            glBindTexture(GL_TEXTURE_2D, textures->textures["wall"]);
            glTexParameterf(GL_TEXTURE_2D, GL_TEXTURE_MAX_ANISOTROPY_EXT, aniso);
            glBindTexture(GL_TEXTURE_2D, textures->textures["floor"]);
            glTexParameterf(GL_TEXTURE_2D, GL_TEXTURE_MAX_ANISOTROPY_EXT, aniso);
            glBindTexture(GL_TEXTURE_2D, 0);
        }
    }
    void initFramebuffers()
    {
        glGenFramebuffers(1, &framebuffer);
        glBindFramebuffer(GL_FRAMEBUFFER, framebuffer);

        glGenTextures(1, &textureColorBuffer);
        glBindTexture(GL_TEXTURE_2D, textureColorBuffer);
        glTexImage2D(GL_TEXTURE_2D, 0, GL_RGB, GAME_WIDTH, GAME_HEIGHT, 0, GL_RGB, GL_UNSIGNED_BYTE, NULL);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
        glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_TEXTURE_2D, textureColorBuffer, 0);

        glGenTextures(1, &depthTexture);
        glBindTexture(GL_TEXTURE_2D, depthTexture);
        glTexImage2D(GL_TEXTURE_2D, 0, GL_DEPTH_COMPONENT24, GAME_WIDTH, GAME_HEIGHT, 0, GL_DEPTH_COMPONENT, GL_FLOAT, NULL);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
        glFramebufferTexture2D(GL_FRAMEBUFFER, GL_DEPTH_ATTACHMENT, GL_TEXTURE_2D, depthTexture, 0);

        if (glCheckFramebufferStatus(GL_FRAMEBUFFER) != GL_FRAMEBUFFER_COMPLETE)
        {
            std::cerr << "Low-res FBO not complete\n";
        }

        glGenFramebuffers(1, &ssaoFBO);
        glBindFramebuffer(GL_FRAMEBUFFER, ssaoFBO);

        glGenTextures(1, &aoTexture);
        glBindTexture(GL_TEXTURE_2D, aoTexture);
        glTexImage2D(GL_TEXTURE_2D, 0, GL_RGB, GAME_WIDTH, GAME_HEIGHT, 0, GL_RGB, GL_UNSIGNED_BYTE, NULL);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
        glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_TEXTURE_2D, aoTexture, 0);

        if (glCheckFramebufferStatus(GL_FRAMEBUFFER) != GL_FRAMEBUFFER_COMPLETE)
        {
            std::cerr << "SSAO FBO not complete\n";
        }

        glBindFramebuffer(GL_FRAMEBUFFER, 0);
    }

    std::shared_ptr<Display> display;
    std::shared_ptr<TextureManager> textures;

    unsigned int shader3D = 0;
    unsigned int shader2D = 0;
    unsigned int shaderSSAO = 0;

    unsigned int wallVAO = 0;
    unsigned int wallVBO = 0;
    unsigned int floorVAO = 0;
    unsigned int floorVBO = 0;
    unsigned int quadVAO = 0;
    unsigned int quadVBO = 0;

    unsigned int framebuffer = 0;
    unsigned int textureColorBuffer = 0;
    unsigned int depthTexture = 0;
    unsigned int ssaoFBO = 0;
    unsigned int aoTexture = 0;
};

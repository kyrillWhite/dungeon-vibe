#define DEBUG_LOG

#include <GL/glew.h>
#include <GLFW/glfw3.h>
#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtc/type_ptr.hpp>
#include <iostream>
#include <random>
#include <ctime>

#include "Shaders.h"
#include "Camera.h"
#include "Display.h"
#include "Map.h"
#include "Minimap.h"
#include "TextureManager.h"
#include "Settings.h"
#include "Input.h"
#include "ShaderUtils.h"

Display display;
Camera camera;
Map dungeon;
Minimap minimap;
TextureManager textures;

// Callbacks and shader helpers moved to separate modules (Input.* and ShaderUtils.*)

int main() {
    if (!glfwInit()) return -1;

    glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 3);
    glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 3);
    glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE);

    GLFWwindow* window = glfwCreateWindow(display.winW, display.winH, "Dungeon & Transparent HUD Map", NULL, NULL);
    if (!window) { glfwTerminate(); return -1; }

    glfwMakeContextCurrent(window);
    glfwSetFramebufferSizeCallback(window, framebuffer_size_callback);
    glfwSetCursorPosCallback(window, mouse_callback);
    glfwSetKeyCallback(window, key_callback);
    glfwSetMouseButtonCallback(window, mouse_button_callback);
    glfwSetInputMode(window, GLFW_CURSOR, GLFW_CURSOR_DISABLED);
    glfwSetCharCallback(window, char_callback);

    glewExperimental = GL_TRUE;
    if (glewInit() != GLEW_OK) return -1;

    dungeon.generate();
    camera.pos = dungeon.spawnPos;
    display.calculateScale(display.winW, display.winH);

    unsigned int shader3D = compileShaderPipeline(vertexShaderSource, fragmentShaderSource);
    unsigned int shader2D = compileShaderPipeline(screenVertexShaderSource, screenFragmentShaderSource);
    unsigned int shaderSSAO = compileShaderPipeline(screenVertexShaderSource, ssaoFragmentShaderSource); // SSAO pass

    float quadVertices[] = {
        -1.0f,  1.0f,  0.0f, 0.0f, 1.0f,  
        -1.0f, -1.0f,  0.0f, 0.0f, 0.0f,   
         1.0f, -1.0f,  0.0f, 1.0f, 0.0f,
        -1.0f,  1.0f,  0.0f, 0.0f, 1.0f,   
         1.0f, -1.0f,  0.0f, 1.0f, 0.0f,   
         1.0f,  1.0f,  0.0f, 1.0f, 1.0f
    };

    // Setup VAO/VBO for walls (Stride = 6)
    unsigned int wallVAO, wallVBO;
    glGenVertexArrays(1, &wallVAO); glGenBuffers(1, &wallVBO);
    glBindVertexArray(wallVAO); glBindBuffer(GL_ARRAY_BUFFER, wallVBO);
    glBufferData(GL_ARRAY_BUFFER, dungeon.wallVertices.size() * sizeof(float), dungeon.wallVertices.data(), GL_STATIC_DRAW);
    glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 6 * sizeof(float), (void*)0); glEnableVertexAttribArray(0);
    glVertexAttribPointer(1, 2, GL_FLOAT, GL_FALSE, 6 * sizeof(float), (void*)(3 * sizeof(float))); glEnableVertexAttribArray(1);
    glVertexAttribPointer(2, 1, GL_FLOAT, GL_FALSE, 6 * sizeof(float), (void*)(5 * sizeof(float))); glEnableVertexAttribArray(2); // AO attribute

    // Setup VAO/VBO for floor and ceiling (Stride = 6)
    unsigned int floorVAO, floorVBO;
    glGenVertexArrays(1, &floorVAO); glGenBuffers(1, &floorVBO);
    glBindVertexArray(floorVAO); glBindBuffer(GL_ARRAY_BUFFER, floorVBO);
    glBufferData(GL_ARRAY_BUFFER, dungeon.floorCeilVertices.size() * sizeof(float), dungeon.floorCeilVertices.data(), GL_STATIC_DRAW);
    glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 6 * sizeof(float), (void*)0); glEnableVertexAttribArray(0);
    glVertexAttribPointer(1, 2, GL_FLOAT, GL_FALSE, 6 * sizeof(float), (void*)(3 * sizeof(float))); glEnableVertexAttribArray(1);
    glVertexAttribPointer(2, 1, GL_FLOAT, GL_FALSE, 6 * sizeof(float), (void*)(5 * sizeof(float))); glEnableVertexAttribArray(2); // AO attribute

    // Setup screen quad buffer (for upscaling)
    unsigned int quadVAO, quadVBO;
    glGenVertexArrays(1, &quadVAO); glGenBuffers(1, &quadVBO);
    glBindVertexArray(quadVAO); glBindBuffer(GL_ARRAY_BUFFER, quadVBO);
    glBufferData(GL_ARRAY_BUFFER, sizeof(quadVertices), quadVertices, GL_STATIC_DRAW);
    glVertexAttribPointer(0, 2, GL_FLOAT, GL_FALSE, 5 * sizeof(float), (void*)0); glEnableVertexAttribArray(0);
    // location 1 is a vec3: we store (z, u, v) in the quad vertex layout -> offset = 2*sizeof(float)
    glVertexAttribPointer(1, 3, GL_FLOAT, GL_FALSE, 5 * sizeof(float), (void*)(2 * sizeof(float))); glEnableVertexAttribArray(1);

    // Initialize textures through manager
    textures.init();

    // Load settings from file (if exists)
    Settings::loadFromFile();

    // apply anisotropic filtering to loaded textures (if supported)
    {
        GLfloat maxAniso = 0.0f;
        if (glewIsSupported("GL_EXT_texture_filter_anisotropic")) {
            glGetFloatv(GL_MAX_TEXTURE_MAX_ANISOTROPY_EXT, &maxAniso);
            float aniso = Settings::ANISOTROPY_LEVEL;
            if (aniso > maxAniso) aniso = maxAniso;
            if (aniso < 1.0f) aniso = 1.0f;
            glBindTexture(GL_TEXTURE_2D, textures.textures["wall"]);
            glTexParameterf(GL_TEXTURE_2D, GL_TEXTURE_MAX_ANISOTROPY_EXT, aniso);
            glBindTexture(GL_TEXTURE_2D, textures.textures["floor"]);
            glTexParameterf(GL_TEXTURE_2D, GL_TEXTURE_MAX_ANISOTROPY_EXT, aniso);
            glBindTexture(GL_TEXTURE_2D, 0);
        }
    }

    // Setup low-resolution framebuffer: color + depth as texture
    unsigned int framebuffer, textureColorBuffer, depthTexture;
    glGenFramebuffers(1, &framebuffer); glBindFramebuffer(GL_FRAMEBUFFER, framebuffer);

    // color texture
    glGenTextures(1, &textureColorBuffer); glBindTexture(GL_TEXTURE_2D, textureColorBuffer);
    glTexImage2D(GL_TEXTURE_2D, 0, GL_RGB, GAME_WIDTH, GAME_HEIGHT, 0, GL_RGB, GL_UNSIGNED_BYTE, NULL);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
    glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_TEXTURE_2D, textureColorBuffer, 0);

    // depth texture (so SSAO can sample it)
    glGenTextures(1, &depthTexture); glBindTexture(GL_TEXTURE_2D, depthTexture);
    glTexImage2D(GL_TEXTURE_2D, 0, GL_DEPTH_COMPONENT24, GAME_WIDTH, GAME_HEIGHT, 0, GL_DEPTH_COMPONENT, GL_FLOAT, NULL);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
    glFramebufferTexture2D(GL_FRAMEBUFFER, GL_DEPTH_ATTACHMENT, GL_TEXTURE_2D, depthTexture, 0);

    if (glCheckFramebufferStatus(GL_FRAMEBUFFER) != GL_FRAMEBUFFER_COMPLETE) {
        std::cerr << "Low-res FBO not complete\n";
    }
    glBindFramebuffer(GL_FRAMEBUFFER, 0);

    // SSAO output FBO (writes AO-applied color)
    unsigned int ssaoFBO, aoTexture;
    glGenFramebuffers(1, &ssaoFBO); glBindFramebuffer(GL_FRAMEBUFFER, ssaoFBO);
    glGenTextures(1, &aoTexture); glBindTexture(GL_TEXTURE_2D, aoTexture);
    glTexImage2D(GL_TEXTURE_2D, 0, GL_RGB, GAME_WIDTH, GAME_HEIGHT, 0, GL_RGB, GL_UNSIGNED_BYTE, NULL);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
    glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_TEXTURE_2D, aoTexture, 0);
    if (glCheckFramebufferStatus(GL_FRAMEBUFFER) != GL_FRAMEBUFFER_COMPLETE) {
        std::cerr << "SSAO FBO not complete\n";
    }
    glBindFramebuffer(GL_FRAMEBUFFER, 0);

    glEnable(GL_BLEND);
    glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);

    float deltaTime = 0.0f, lastFrame = 0.0f;

    while (!glfwWindowShouldClose(window)) {
        float currentFrame = static_cast<float>(glfwGetTime());
        deltaTime = currentFrame - lastFrame;
        lastFrame = currentFrame;

        camera.processInput(window, deltaTime, dungeon.grid);

        // --- FOG OF WAR CALCULATION FOR MINIMAP ---
        {
            int pX = static_cast<int>(camera.pos.x);
            int pZ = static_cast<int>(camera.pos.z);
            const int VIEW_DIST = 6; 
            const int NUM_RAYS = 120; 

            if (pX >= 0 && pX < MAP_SIZE && pZ >= 0 && pZ < MAP_SIZE) {
                dungeon.visible[pX][pZ] = 1;
            }

            for (int i = 0; i < NUM_RAYS; ++i) {
                float angle = (static_cast<float>(i) / NUM_RAYS) * 2.0f * 3.1415926f;
                float dirX = std::cos(angle); float dirZ = std::sin(angle);

                for (float dist = 0.5f; dist <= static_cast<float>(VIEW_DIST); dist += 0.3f) {
                    int curX = static_cast<int>(camera.pos.x + dirX * dist);
                    int curZ = static_cast<int>(camera.pos.z + dirZ * dist);

                    if (curX < 0 || curX >= MAP_SIZE || curZ < 0 || curZ >= MAP_SIZE) break;
                    dungeon.visible[curX][curZ] = 1;
                    if (dungeon.grid[curX][curZ] == 1) break; 
                }
            }
        }

        bool isTabPressed = (glfwGetKey(window, GLFW_KEY_TAB) == GLFW_PRESS);
        
        // --- STAGE 1: RENDER 3D TO LOW RESOLUTION ---
        glBindFramebuffer(GL_FRAMEBUFFER, framebuffer);
        glViewport(0, 0, GAME_WIDTH, GAME_HEIGHT);
        glEnable(GL_DEPTH_TEST);
        glClearColor(0.0f, 0.0f, 0.0f, 1.0f);
        glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
        
    
        // --- STAGE 3: SSAO / DITHER PASS ---
        glUseProgram(shader3D);
        glm::mat4 model = glm::mat4(1.0f);
        glm::mat4 view = glm::lookAt(camera.pos, camera.pos + camera.front, camera.up);

        // Use FOV from settings
        float aspect = (float)GAME_WIDTH / (float)GAME_HEIGHT;
        glm::mat4 projection = glm::perspective(glm::radians(Settings::FOV_DEGREES), aspect, 0.05f, 100.0f);

        // Apply projection matrices
        glUniformMatrix4fv(glGetUniformLocation(shader3D, "model"), 1, GL_FALSE, glm::value_ptr(model));
        glUniformMatrix4fv(glGetUniformLocation(shader3D, "view"), 1, GL_FALSE, glm::value_ptr(view));
        glUniformMatrix4fv(glGetUniformLocation(shader3D, "projection"), 1, GL_FALSE, glm::value_ptr(projection));

        // Pass lighting parameters from Settings
        glUniform3fv(glGetUniformLocation(shader3D, "cameraPos"), 1, glm::value_ptr(camera.pos));
        glUniform1f(glGetUniformLocation(shader3D, "lightNear"), Settings::LIGHT_RADIUS_NEAR);
        glUniform1f(glGetUniformLocation(shader3D, "lightFar"), Settings::LIGHT_RADIUS_FAR);
        glUniform1f(glGetUniformLocation(shader3D, "ambientLight"), Settings::AMBIENT_LIGHT);
        glUniform1f(glGetUniformLocation(shader3D, "lightSharpness"), Settings::LIGHT_SHARPNESS);
        glUniform1f(glGetUniformLocation(shader3D, "lightTransitionSoftness"), Settings::LIGHT_TRANSITION_SOFTNESS);
        glUniform1f(glGetUniformLocation(shader3D, "aoLightBlend"), Settings::AO_LIGHT_BLEND);
        glUniform1i(glGetUniformLocation(shader3D, "debugWhitebox"), Settings::DEBUG_WHITEBOX_MODE ? 1 : 0);

        // Torch (player-carried) flickering yellow-orange light
        glm::vec3 torchColor(1.0f, 0.55f, 0.18f);
        // Delta-time independent, slower and more random flicker
        static float torchFlicker = 1.0f;
        static float torchTarget = 1.0f;
        static float torchTimer = 0.0f;
        static float torchNextChange = 0.35f;
        static std::mt19937 torchRng(static_cast<unsigned>(std::time(nullptr)));
        static std::uniform_real_distribution<float> changeDist(0.12f, 0.9f); // seconds between changes
        static std::uniform_real_distribution<float> flickerDist(0.78f, 1.18f);  // target range

        // advance timer using frame deltaTime so flicker rate is independent of fps
        torchTimer += deltaTime;
        if (torchTimer >= torchNextChange) {
            torchTimer = 0.0f;
            torchNextChange = changeDist(torchRng);
            torchTarget = flickerDist(torchRng);
        }

        // exponential smoothing towards target (time-based) -> stable across framerates
        const float smoothingSpeed = 3.5f; // larger = faster response
        float alpha = 1.0f - std::exp(-smoothingSpeed * deltaTime);
        torchFlicker += (torchTarget - torchFlicker) * alpha;

        // keep within safe bounds
        if (torchFlicker < 0.6f) torchFlicker = 0.6f;
        if (torchFlicker > 1.25f) torchFlicker = 1.25f;

        glUniform3fv(glGetUniformLocation(shader3D, "torchColor"), 1, glm::value_ptr(torchColor));
        glUniform1f(glGetUniformLocation(shader3D, "torchFlicker"), torchFlicker);

        // Draw wall geometry
        glActiveTexture(GL_TEXTURE0);
        glBindTexture(GL_TEXTURE_2D, textures.textures["wall"]);
        glUniform1i(glGetUniformLocation(shader3D, "texSampler"), 0);
        glBindVertexArray(wallVAO);
        glDrawArrays(GL_TRIANGLES, 0, dungeon.wallVertices.size() / 6);

        // Draw floor and ceiling
        glActiveTexture(GL_TEXTURE0);
        glBindTexture(GL_TEXTURE_2D, textures.textures["floor"]);
        glUniform1i(glGetUniformLocation(shader3D, "texSampler"), 0);
        glBindVertexArray(floorVAO);
        glDrawArrays(GL_TRIANGLES, 0, dungeon.floorCeilVertices.size() / 6);

        // --- STAGE 2.5: MINIMAP OVERLAY AT LOW RESOLUTION (AFTER 3D, BEFORE SSAO) ---
        // Render minimap into the low-res color buffer so it gets processed by SSAO/dither
        // Do not write to depth buffer to avoid occluding 3D geometry
        glDisable(GL_DEPTH_TEST);
        glDepthMask(GL_FALSE);
        glUseProgram(shader2D);
        glUniform1i(glGetUniformLocation(shader2D, "useTexture"), 0);
        float currentAlpha = isTabPressed ? 0.5f : 0.75f;
        glUniform1f(glGetUniformLocation(shader2D, "mapAlpha"), currentAlpha);
        minimap.render(shader2D, dungeon, camera, isTabPressed, currentAlpha, GAME_WIDTH, GAME_HEIGHT);
        // Restore depth writes and depth test
        glDepthMask(GL_TRUE);
        glEnable(GL_DEPTH_TEST);

        // --- STAGE 2: SSAO / DITHER PASS ---
        // Run SSAO shader reading the low-res color + depth, write into aoTexture
        glBindFramebuffer(GL_FRAMEBUFFER, ssaoFBO);
        glViewport(0, 0, GAME_WIDTH, GAME_HEIGHT);
        glDisable(GL_DEPTH_TEST);
        glClearColor(0.0f, 0.0f, 0.0f, 1.0f);
        glClear(GL_COLOR_BUFFER_BIT);

        glUseProgram(shaderSSAO);
        // Bind input textures
        glActiveTexture(GL_TEXTURE0); glBindTexture(GL_TEXTURE_2D, textureColorBuffer);
        glUniform1i(glGetUniformLocation(shaderSSAO, "colorTex"), 0);
        glActiveTexture(GL_TEXTURE1); glBindTexture(GL_TEXTURE_2D, depthTexture);
        glUniform1i(glGetUniformLocation(shaderSSAO, "depthTex"), 1);
        // Camera projection range (matches projection used for 3D pass)
        glUniform1f(glGetUniformLocation(shaderSSAO, "projNear"), 0.05f);
        glUniform1f(glGetUniformLocation(shaderSSAO, "projFar"), 100.0f);
        glUniform1f(glGetUniformLocation(shaderSSAO, "ambientLight"), Settings::AMBIENT_LIGHT);
        glUniform1f(glGetUniformLocation(shaderSSAO, "aoLightBlend"), Settings::AO_LIGHT_BLEND);
        glUniform1i(glGetUniformLocation(shaderSSAO, "useDither"), Settings::USE_DITHER);
        glUniform1i(glGetUniformLocation(shaderSSAO, "ditherPalette"), Settings::DITHER_PALETTE);
        glUniform2f(glGetUniformLocation(shaderSSAO, "screenSize"), (float)GAME_WIDTH, (float)GAME_HEIGHT);

        glBindVertexArray(quadVAO);
        glDrawArrays(GL_TRIANGLES, 0, 6);

        // --- STAGE 3: UPSCALE TO SCREEN ---
        glBindFramebuffer(GL_FRAMEBUFFER, 0);
        glViewport(display.renderX, display.renderY, display.renderW, display.renderH);
        glClearColor(0.0f, 0.0f, 0.0f, 1.0f);
        glClear(GL_COLOR_BUFFER_BIT);

        // Draw upscaled (SSAO/dithered) texture to screen
        glUseProgram(shader2D);
        glUniform1i(glGetUniformLocation(shader2D, "useTexture"), 1);
        glUniform1i(glGetUniformLocation(shader2D, "screenTexture"), 0);
        glUniform1f(glGetUniformLocation(shader2D, "screenDistortion"), Settings::SCREEN_DISTORTION);
        float screenAspect = (float)display.renderW / (float)display.renderH;
        glUniform1f(glGetUniformLocation(shader2D, "screenAspect"), screenAspect);

        glActiveTexture(GL_TEXTURE0); glBindTexture(GL_TEXTURE_2D, aoTexture);
        glBindVertexArray(quadVAO);
        glDrawArrays(GL_TRIANGLES, 0, 6);

        // minimap already rendered into low-res buffer earlier; no overlay here

        glfwSwapBuffers(window);
        glfwPollEvents();
    }

    // Save settings on exit (just in case)
    Settings::saveToFile();

    // Full memory cleanup before exit
    minimap.cleanup();
    textures.cleanup();
    glDeleteVertexArrays(1, &wallVAO); glDeleteBuffers(1, &wallVBO);
    glDeleteVertexArrays(1, &floorVAO); glDeleteBuffers(1, &floorVBO);
    glDeleteVertexArrays(1, &quadVAO); glDeleteBuffers(1, &quadVBO);

    // delete framebuffers/textures
    glDeleteFramebuffers(1, &framebuffer);
    glDeleteFramebuffers(1, &ssaoFBO);
    glDeleteTextures(1, &textureColorBuffer);
    glDeleteTextures(1, &aoTexture);
    glDeleteTextures(1, &depthTexture);

    glDeleteProgram(shader3D); glDeleteProgram(shader2D); glDeleteProgram(shaderSSAO);
    glfwTerminate();
    return 0;
}

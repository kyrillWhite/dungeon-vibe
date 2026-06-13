#pragma once

#include <GL/glew.h>
#include <vector>
#include <cmath>
#include <algorithm>
#include "Map.h"
#include "Camera.h"
#include "Display.h"

struct Minimap
{
    unsigned int VAO, VBO;
    bool initialized;

    Minimap() : VAO(0), VBO(0), initialized(false) {}

    void init()
    {
        glGenVertexArrays(1, &VAO);
        glGenBuffers(1, &VBO);
        initialized = true;
    }

    void cleanup()
    {
        if (initialized)
        {
            glDeleteVertexArrays(1, &VAO);
            glDeleteBuffers(1, &VBO);
        }
    }

    // Convert pixel coords (in a given target resolution) to NDC
    float pixelToNdcX(float pixelX, float targetW) const
    {
        return (pixelX * 2.0f / targetW) - 1.0f;
    }

    float pixelToNdcY(float pixelY, float targetH) const
    {
        return 1.0f - (pixelY * 2.0f / targetH);
    }

    void render(unsigned int shader2D, const Map &map, const Camera &camera, bool fullScreenMode, float currentAlpha, int targetW, int targetH)
    {
        const bool DEBUG_FORCE_BRIGHT = false;
        if (!initialized)
            init();

        std::vector<float> vertices;

        float centerPixX = 0.0f;
        float centerPixY = 0.0f;
        float TILE_SIZE = 0.0f;
        float pSize = 0.0f;

        float denomW = static_cast<float>(targetW);
        float denomH = static_cast<float>(targetH);

        if (fullScreenMode)
        {
            // --- TAB MODE: Full-screen overlay at center ---
            centerPixX = denomW / 2.0f;
            centerPixY = denomH / 2.0f;
            TILE_SIZE = 4.0f;
            pSize = 4.0f;
        }
        else
        {
            // --- STANDARD HUD MODE: Square minimap in top left corner ---

            // Increase frame size from 40.0f to 60.0f pixels (great for 240x144)
            const float HUD_BOX_SIZE = 60.0f;
            const float HUD_PADDING = 6.0f;

            // scale HUD box to the target render resolution
            float scaleX = denomW / static_cast<float>(GAME_WIDTH);
            float scaleY = denomH / static_cast<float>(GAME_HEIGHT);
            float padX = HUD_PADDING * scaleX;
            float padY = HUD_PADDING * scaleY;
            float boxW = HUD_BOX_SIZE * scaleX;
            float boxH = HUD_BOX_SIZE * scaleY;

            centerPixX = padX + (boxW / 2.0f);
            centerPixY = padY + (boxH / 2.0f);

            // Increase tile scale from 2.0f to 3.0f to make maze larger
            TILE_SIZE = 3.0f;
            pSize = 4.0f; // Player arrow also slightly larger for readability

            // Enable Scissor Test to clip maze within frame
            glEnable(GL_SCISSOR_TEST);
            glScissor(
                static_cast<int>(padX),
                static_cast<int>(denomH - padY - boxH),
                static_cast<int>(boxW),
                static_cast<int>(boxH));

            // --- ADD OPAQUE BACKGROUND ---
            // Save current alpha set in main.cpp
            int alphaLocation = glGetUniformLocation(shader2D, "mapAlpha");
            // Force to 1.0f (100% opaque) for background
            glUniform1f(alphaLocation, 1.0f);

            // Calculate black background quad corners in NDC
            float bx0 = pixelToNdcX(padX, denomW);
            float bx1 = pixelToNdcX(padX + boxW, denomW);
            float by0 = pixelToNdcY(padY, denomH);
            float by1 = pixelToNdcY(padY + boxH, denomH);

            float bg = 0.05f;
            float bgR = bg, bgG = bg, bgB = bg;
            float bgQuad[] = {
                bx0, by0, bgR, bgG, bgB, bx0, by1, bgR, bgG, bgB, bx1, by1, bgR, bgG, bgB,
                bx0, by0, bgR, bgG, bgB, bx1, by1, bgR, bgG, bgB, bx1, by0, bgR, bgG, bgB};

            // Quickly send and render only black background quad
            glBindVertexArray(VAO);
            glBindBuffer(GL_ARRAY_BUFFER, VBO);
            glBufferData(GL_ARRAY_BUFFER, sizeof(bgQuad), bgQuad, GL_STREAM_DRAW);
            glVertexAttribPointer(0, 2, GL_FLOAT, GL_FALSE, 5 * sizeof(float), (void *)0);
            glEnableVertexAttribArray(0);
            glVertexAttribPointer(1, 3, GL_FLOAT, GL_FALSE, 5 * sizeof(float), (void *)(2 * sizeof(float)));
            glEnableVertexAttribArray(1);
            glDrawArrays(GL_TRIANGLES, 0, 6);

            // Restore original alpha for drawing walls and arrow
            glUniform1f(alphaLocation, currentAlpha);
        }

        // Player current coordinates
        float pX = camera.pos.x;
        float pZ = camera.pos.z;

        // 1. Wall geometry (Uses new increased TILE_SIZE)
        for (int x = 0; x < MAP_SIZE; x++)
        {
            for (int z = 0; z < MAP_SIZE; z++)
            {
                if (map.grid[x][z] == 1 && map.visible[x][z] == 1)
                {
                    float offsetX = static_cast<float>(x) - pX;
                    float offsetZ = static_cast<float>(z) - pZ;

                    float pixX0 = centerPixX + offsetX * TILE_SIZE;
                    float pixX1 = pixX0 + TILE_SIZE;
                    float pixY0 = centerPixY + offsetZ * TILE_SIZE;
                    float pixY1 = pixY0 + TILE_SIZE;

                    float x0 = pixelToNdcX(pixX0, denomW);
                    float x1 = pixelToNdcX(pixX1, denomW);
                    float y0 = pixelToNdcY(pixY0, denomH);
                    float y1 = pixelToNdcY(pixY1, denomH);

                    float c = 0.4f;
                    float quad[] = {
                        x0, y0, c, c, c, x0, y1, c, c, c, x1, y1, c, c, c,
                        x0, y0, c, c, c, x1, y1, c, c, c, x1, y0, c, c, c};
                    vertices.insert(vertices.end(), std::begin(quad), std::end(quad));
                }
            }
        }

        // 2. Player geometry
        float angle = std::atan2(-camera.front.z, camera.front.x);

        float pXTop = centerPixX + std::cos(angle) * pSize;
        float pYTop = centerPixY - std::sin(angle) * pSize;
        float pXLeft = centerPixX + std::cos(angle + 2.4f) * (pSize * 0.7f);
        float pYLeft = centerPixY - std::sin(angle + 2.4f) * (pSize * 0.7f);
        float pXRight = centerPixX + std::cos(angle - 2.4f) * (pSize * 0.7f);
        float pYRight = centerPixY - std::sin(angle - 2.4f) * (pSize * 0.7f);

        float xTop = pixelToNdcX(pXTop, denomW);
        float yTop = pixelToNdcY(pYTop, denomH);
        float xLeft = pixelToNdcX(pXLeft, denomW);
        float yLeft = pixelToNdcY(pYLeft, denomH);
        float xRight = pixelToNdcX(pXRight, denomW);
        float yRight = pixelToNdcY(pYRight, denomH);

        float r = 0.0f, g = 1.0f, b = 0.0f;
        float playerTri[] = {
            xTop, yTop, r, g, b,
            xLeft, yLeft, r, g, b,
            xRight, yRight, r, g, b};
        vertices.insert(vertices.end(), std::begin(playerTri), std::end(playerTri));

        // Send and render maze and player over background
        glBindVertexArray(VAO);
        glBindBuffer(GL_ARRAY_BUFFER, VBO);
        glBufferData(GL_ARRAY_BUFFER, vertices.size() * sizeof(float), vertices.data(), GL_STREAM_DRAW);

        size_t verts = vertices.size() / 5; // 5 floats per vertex (x,y,r,g,b)

        glVertexAttribPointer(0, 2, GL_FLOAT, GL_FALSE, 5 * sizeof(float), (void *)0);
        glEnableVertexAttribArray(0);
        glVertexAttribPointer(1, 3, GL_FLOAT, GL_FALSE, 5 * sizeof(float), (void *)(2 * sizeof(float)));
        glEnableVertexAttribArray(1);

        glDrawArrays(GL_TRIANGLES, 0, vertices.size() / 5);

        (void)verts;

        // Disable scissor to avoid breaking screen upscale
        if (!fullScreenMode)
        {
            glDisable(GL_SCISSOR_TEST);
        }
    }
};

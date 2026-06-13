#pragma once

#include <GLFW/glfw3.h>
#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>

#include "Settings.h"
#include "Display.h" // For access to global MAP_SIZE

struct Camera
{
    glm::vec3 pos;
    glm::vec3 front;
    glm::vec3 up;
    float yaw;
    float lastX;
    bool firstMouse;
    bool isCursorLocked;

    Camera()
    {
        // Initial position is overwritten in main.cpp via dungeon.spawnPos
        pos = glm::vec3(0.0f, 0.5f, 0.0f);
        front = glm::vec3(0.0f, 0.0f, -1.0f);
        up = glm::vec3(0.0f, 1.0f, 0.0f);
        yaw = -90.0f;
        lastX = 400.0f;
        firstMouse = true;
        isCursorLocked = true;
    }

    // Pass the actual MAP_SIZE array size instead of static [16][16]
    void processInput(bool _up, bool _down, bool _left, bool _right, float deltaTime, int grid[MAP_SIZE][MAP_SIZE])
    {
        if (!isCursorLocked)
            return;

        float speed = Settings::PLAYER_SPEED * deltaTime;
        glm::vec3 frontXZ = glm::normalize(glm::vec3(front.x, 0.0f, front.z));
        glm::vec3 rightXZ = glm::normalize(glm::cross(frontXZ, up));

        glm::vec3 moveDir(0.0f);

        if (_up)
            moveDir += frontXZ;
        if (_down)
            moveDir -= frontXZ;
        if (_left)
            moveDir -= rightXZ;
        if (_right)
            moveDir += rightXZ;

        if (glm::length(moveDir) <= 0.0f)
            return;

        moveDir = glm::normalize(moveDir);

        float r = 0.2f; // Player collision radius
        float offsetsX[] = {-r, r, -r, r};
        float offsetsZ[] = {-r, -r, r, r};

        // --- STEP 1: MOVEMENT AND X AXIS COLLISION CHECK ---
        float targetX = pos.x + moveDir.x * speed;
        bool collisionX = false;

        for (int i = 0; i < 4; ++i)
        {
            int checkX = static_cast<int>(targetX + offsetsX[i]);
            int checkZ = static_cast<int>(pos.z + offsetsZ[i]);

            // Boundary check now dynamically depends on MAP_SIZE
            if (checkX < 0 || checkX >= MAP_SIZE || checkZ < 0 || checkZ >= MAP_SIZE || grid[checkX][checkZ] == 1)
            {
                collisionX = true;
                break;
            }
        }
        if (!collisionX)
        {
            pos.x = targetX;
        }

        // --- STEP 2: MOVEMENT AND Z AXIS COLLISION CHECK ---
        float targetZ = pos.z + moveDir.z * speed;
        bool collisionZ = false;

        for (int i = 0; i < 4; ++i)
        {
            int checkX = static_cast<int>(pos.x + offsetsX[i]);
            int checkZ = static_cast<int>(targetZ + offsetsZ[i]);

            // Boundary check now dynamically depends on MAP_SIZE
            if (checkX < 0 || checkX >= MAP_SIZE || checkZ < 0 || checkZ >= MAP_SIZE || grid[checkX][checkZ] == 1)
            {
                collisionZ = true;
                break;
            }
        }
        if (!collisionZ)
        {
            pos.z = targetZ;
        }
    }

    void processMouse(double xposIn)
    {
        if (!isCursorLocked)
            return;
        float xpos = static_cast<float>(xposIn);
        if (firstMouse)
        {
            lastX = xpos;
            firstMouse = false;
        }
        float xoffset = xpos - lastX;
        lastX = xpos;
        xoffset *= 0.1f;
        yaw += xoffset;

        glm::vec3 f;
        f.x = cos(glm::radians(yaw));
        f.y = 0.0f;
        f.z = sin(glm::radians(yaw));
        front = glm::normalize(f);
    }
};

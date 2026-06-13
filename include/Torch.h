#pragma once

#include <random>
#include <ctime>
#include <cmath>
#include <algorithm>
#include <glm/glm.hpp>

class Torch
{
private:
    float torchFlicker;
    float torchTarget;
    float torchTimer;
    float torchNextChange;
    glm::vec3 torchColor;
    std::mt19937 torchRng;

public:
    Torch()
        : torchFlicker(1.0f), torchTarget(1.0f), torchTimer(0.0f), torchNextChange(0.35f), torchColor(1.0f, 0.55f, 0.18f), torchRng(static_cast<unsigned>(std::time(nullptr))) {}

    ~Torch() {}

    void update(float deltaTime)
    {
        std::uniform_real_distribution<float> changeDist(0.12f, 0.9f);
        std::uniform_real_distribution<float> flickerDist(0.78f, 1.18f);

        torchTimer += deltaTime;
        if (torchTimer >= torchNextChange)
        {
            torchTimer = 0.0f;
            torchNextChange = changeDist(torchRng);
            torchTarget = flickerDist(torchRng);
        }

        const float smoothingSpeed = 3.5f;
        float alpha = 1.0f - std::exp(-smoothingSpeed * deltaTime);
        torchFlicker += (torchTarget - torchFlicker) * alpha;

        torchFlicker = std::clamp(torchFlicker, 0.6f, 1.25f);
    }

    float getFlicker() const
    {
        return torchFlicker;
    }

    glm::vec3 getColor() const
    {
        return torchColor;
    }
};

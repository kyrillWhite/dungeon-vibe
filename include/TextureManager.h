#pragma once

#include <GL/glew.h>
#include <iostream>
#include <map>

// Include stb_image implementation only in this file
#define STB_IMAGE_IMPLEMENTATION
#define STBI_ONLY_PNG
#include "stb_image.h"

struct TextureManager
{
    std::map<std::string, unsigned int> textures;

    unsigned int loadPNG(const char *filename)
    {
        unsigned int textureID;
        glGenTextures(1, &textureID);
        glBindTexture(GL_TEXTURE_2D, textureID);

        // Flip texture vertically when loading, since in OpenGL Y axis goes from bottom up
        stbi_set_flip_vertically_on_load(true);

        int width, height, nrChannels;
        unsigned char *data = stbi_load(filename, &width, &height, &nrChannels, 0);

        if (data)
        {
            GLenum format = GL_RGB;
            bool isGrayscale = false;

            if (nrChannels == 1)
            {
                format = GL_RED;
                isGrayscale = true;
            }
            else if (nrChannels == 3)
            {
                format = GL_RGB;
            }
            else if (nrChannels == 4)
            {
                format = GL_RGBA;
            }

            glPixelStorei(GL_UNPACK_ALIGNMENT, 1);
            glTexImage2D(GL_TEXTURE_2D, 0, format, width, height, 0, format, GL_UNSIGNED_BYTE, data);

            if (isGrayscale)
            {
                GLint swizzleMask[] = {GL_RED, GL_RED, GL_RED, GL_ONE};
                glTexParameteriv(GL_TEXTURE_2D, GL_TEXTURE_SWIZZLE_RGBA, swizzleMask);
            }
            // Disable 4-byte alignment for non-standard resolutions
            glPixelStorei(GL_UNPACK_ALIGNMENT, 1);

            glTexImage2D(GL_TEXTURE_2D, 0, format, width, height, 0, format, GL_UNSIGNED_BYTE, data);

            // STRICT REQUIREMENT: Only pure retro pixels without smoothing
            glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
            glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
            glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_REPEAT);
            glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_REPEAT);

            stbi_image_free(data);
#ifdef GAME_DEBUG
            std::cout << "[SUCCESS] Texture loaded: " << filename << " (" << width << "x" << height << ", channels: " << nrChannels << ")" << std::endl;
#endif
        }
        else
        {
#ifdef GAME_DEBUG
            std::cerr << "[ERROR] stbi_image failed to load file: " << filename << std::endl;
#endif
            stbi_image_free(data);
            return 0;
        }

        return textureID;
    }

    void init()
    {
        textures["wall"] = loadPNG("textures/wall.png");
        textures["floor"] = loadPNG("textures/floor.png");

        for (const auto &[name, id] : textures)
        {
            if (id == 0)
            {
#ifdef GAME_DEBUG
                std::cerr << "[WARNING] Can't load " << name << " texture!" << std::endl;
#endif
            }
        }
    }

    void cleanup()
    {
        for (const auto &[name, id] : textures)
        {
            glDeleteTextures(1, &id);
        }
    }
};

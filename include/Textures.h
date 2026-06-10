#ifndef TEXTURES_H
#define TEXTURES_H

#include <GL/glew.h>
#include <iostream>

// Include stb_image implementation only in this file
#define STB_IMAGE_IMPLEMENTATION
#include "stb_image.h"

struct TextureManager {
    unsigned int wallTexture;
    unsigned int floorTexture;

    unsigned int loadPNG(const char* filename) {
        unsigned int textureID;
        glGenTextures(1, &textureID);
        glBindTexture(GL_TEXTURE_2D, textureID);

        // Flip texture vertically when loading, since in OpenGL Y axis goes from bottom up
        stbi_set_flip_vertically_on_load(true);

        int width, height, nrChannels;
        unsigned char* data = stbi_load(filename, &width, &height, &nrChannels, 0);
        
        if (data) {
            GLenum format = GL_RGB;
            bool isGrayscale = false;

            if (nrChannels == 1) {
                format = GL_RED;
                isGrayscale = true;
            } else if (nrChannels == 3) {
                format = GL_RGB;
            } else if (nrChannels == 4) {
                format = GL_RGBA;
            }

            glPixelStorei(GL_UNPACK_ALIGNMENT, 1);
            glTexImage2D(GL_TEXTURE_2D, 0, format, width, height, 0, format, GL_UNSIGNED_BYTE, data);

            if (isGrayscale) {
                GLint swizzleMask[] = { GL_RED, GL_RED, GL_RED, GL_ONE };
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
            std::cout << "[SUCCESS] Texture loaded: " << filename << " (" << width << "x" << height << ", channels: " << nrChannels << ")" << std::endl;
        } else {
            std::cerr << "[ERROR] stbi_image failed to load file: " << filename << std::endl;
            stbi_image_free(data);
            return 0;
        }

        return textureID;
    }

    void init() {
        // Load PNG files
        wallTexture = loadPNG("textures/wall.png");
        floorTexture = loadPNG("textures/floor.png");

        if (wallTexture == 0 || floorTexture == 0) {
            std::cerr << "[WARNING] Check that wall.png and floor.png exist in the working directory!" << std::endl;
        }
    }

    void cleanup() {
        glDeleteTextures(1, &wallTexture);
        glDeleteTextures(1, &floorTexture);
    }
};

#endif

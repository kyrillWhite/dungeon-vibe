#ifndef TEXTURES_H
#define TEXTURES_H

#include <GL/glew.h>
#include <iostream>

// Включаем реализацию stb_image только в этом файле
#define STB_IMAGE_IMPLEMENTATION
#include "stb_image.h"

struct TextureManager {
    unsigned int wallTexture;
    unsigned int floorTexture;

    unsigned int loadPNG(const char* filename) {
        unsigned int textureID;
        glGenTextures(1, &textureID);
        glBindTexture(GL_TEXTURE_2D, textureID);

        // Переворачиваем текстуру по вертикали при загрузке, так как в OpenGL ось Y идет снизу вверх
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
            // Отключаем выравнивание по 4 байта на случай нестандартных разрешений
            glPixelStorei(GL_UNPACK_ALIGNMENT, 1);

            glTexImage2D(GL_TEXTURE_2D, 0, format, width, height, 0, format, GL_UNSIGNED_BYTE, data);

            // СТРОГОЕ ТРЕБОВАНИЕ: Только чистые ретро-пиксели без сглаживания
            glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
            glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
            glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_REPEAT);
            glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_REPEAT);

            stbi_image_free(data);
            std::cout << "[SUCCESS] Текстура загружена: " << filename << " (" << width << "x" << height << ", каналов: " << nrChannels << ")" << std::endl;
        } else {
            std::cerr << "[ERROR] stbi_image не смог загрузить файл: " << filename << std::endl;
            stbi_image_free(data);
            return 0;
        }

        return textureID;
    }

    void init() {
        // Переходим на PNG-файлы
        wallTexture  = loadPNG("wall.png");
        floorTexture = loadPNG("floor.png");

        if (wallTexture == 0 || floorTexture == 0) {
            std::cerr << "[WARNING] Проверьте наличие wall.png и floor.png в папке запуска!" << std::endl;
        }
    }

    void cleanup() {
        glDeleteTextures(1, &wallTexture);
        glDeleteTextures(1, &floorTexture);
    }
};

#endif

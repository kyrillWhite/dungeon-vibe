#pragma once

#include <GL/glew.h>
#include <GLFW/glfw3.h>
#include <algorithm>

inline const int GAME_WIDTH = 240;
inline const int GAME_HEIGHT = 144;
inline const int MAP_SIZE = 48;

inline const bool DEBUG_WHITEBOX_MODE = false; // Temporary, main.cpp now takes real value from Settings

struct Display {
    int winW;
    int winH;
    int renderX;
    int renderY;
    int renderW;
    int renderH;

    Display() {
        winW = 960;
        winH = 720;
        renderX = 0; renderY = 0;
        renderW = GAME_WIDTH; renderH = GAME_HEIGHT;
    }

    void calculateScale(int w, int h) {
        winW = w;
        winH = h;
        int scaleX = winW / GAME_WIDTH;
        int scaleY = winH / GAME_HEIGHT;
        int scale = std::max(1, std::min(scaleX, scaleY));
        
        renderW = GAME_WIDTH * scale;
        renderH = GAME_HEIGHT * scale;
        renderX = (winW - renderW) / 2;
        renderY = (winH - renderH) / 2;
    }
};

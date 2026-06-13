#pragma once

#include <vector>
#include <cstdlib>
#include <ctime>
#include <algorithm>
#include <random>
#include <glm/glm.hpp>
#include "Display.h"

struct Room
{
    int x, z, w, h;
    int centerX, centerZ;
};

struct Map
{
    int grid[MAP_SIZE][MAP_SIZE];
    int visible[MAP_SIZE][MAP_SIZE];

    std::vector<float> wallVertices;      // Exactly 6 floats: X, Y, Z, U, V, AO
    std::vector<float> floorCeilVertices; // Exactly 6 floats: X, Y, Z, U, V, AO
    glm::vec3 spawnPos;

    int isWall(int x, int z)
    {
        if (x < 0 || x >= MAP_SIZE || z < 0 || z >= MAP_SIZE)
            return 1;
        return grid[x][z];
    }

    void addVisibleFaces(int x, int z)
    {
        float fx = static_cast<float>(x);
        float fz = static_cast<float>(z);
        float size = 1.0f;
        float h = 1.0f;

        float x0 = fx, x1 = fx + size;
        float y0 = 0.0f, y1 = h;
        float z0 = fz, z1 = fz + size;

        // Remove AO: Write hard unit, removing any shadows on walls
        float noAO = 1.00f;

        if (z + 1 < MAP_SIZE && grid[x][z + 1] == 0)
        { // South
            float face[] = {
                x0, y0, z1, 0.0f, 0.0f, noAO, x1, y0, z1, 1.0f, 0.0f, noAO, x1, y1, z1, 1.0f, 1.0f, noAO,
                x1, y1, z1, 1.0f, 1.0f, noAO, x0, y1, z1, 0.0f, 1.0f, noAO, x0, y0, z1, 0.0f, 0.0f, noAO};
            wallVertices.insert(wallVertices.end(), std::begin(face), std::end(face));
        }

        if (z - 1 >= 0 && grid[x][z - 1] == 0)
        { // North
            float face[] = {
                x1, y0, z0, 0.0f, 0.0f, noAO, x0, y0, z0, 1.0f, 0.0f, noAO, x0, y1, z0, 1.0f, 1.0f, noAO,
                x0, y1, z0, 1.0f, 1.0f, noAO, x1, y1, z0, 0.0f, 1.0f, noAO, x1, y0, z0, 0.0f, 0.0f, noAO};
            wallVertices.insert(wallVertices.end(), std::begin(face), std::end(face));
        }

        if (x - 1 >= 0 && grid[x - 1][z] == 0)
        { // West
            float face[] = {
                x0, y0, z0, 0.0f, 0.0f, noAO, x0, y0, z1, 1.0f, 0.0f, noAO, x0, y1, z1, 1.0f, 1.0f, noAO,
                x0, y1, z1, 1.0f, 1.0f, noAO, x0, y1, z0, 0.0f, 1.0f, noAO, x0, y0, z0, 0.0f, 0.0f, noAO};
            wallVertices.insert(wallVertices.end(), std::begin(face), std::end(face));
        }

        if (x + 1 < MAP_SIZE && grid[x + 1][z] == 0)
        { // East
            float face[] = {
                x1, y0, z1, 0.0f, 0.0f, noAO, x1, y0, z0, 1.0f, 0.0f, noAO, x1, y1, z0, 1.0f, 1.0f, noAO,
                x1, y1, z0, 1.0f, 1.0f, noAO, x1, y1, z1, 0.0f, 1.0f, noAO, x1, y0, z1, 0.0f, 0.0f, noAO};
            wallVertices.insert(wallVertices.end(), std::begin(face), std::end(face));
        }
    }

    void createCorridor(int x1, int z1, int x2, int z2)
    {
        int startX = std::min(x1, x2);
        int endX = std::max(x1, x2);
        for (int x = startX; x <= endX; ++x)
            grid[x][z1] = 0;
        int startZ = std::min(z1, z2);
        int endZ = std::max(z1, z2);
        for (int z = startZ; z <= endZ; ++z)
            grid[x2][z] = 0;
    }

    void generate()
    {
        for (int x = 0; x < MAP_SIZE; x++)
        {
            for (int z = 0; z < MAP_SIZE; z++)
            {
                grid[x][z] = 1;
                visible[x][z] = 0;
            }
        }

        std::vector<Room> rooms;
        int zoneSize = MAP_SIZE / 3;

        for (int rX = 0; rX < 3; ++rX)
        {
            for (int rZ = 0; rZ < 3; ++rZ)
            {
                int minX = rX * zoneSize + 2;
                int minZ = rZ * zoneSize + 2;
                int maxW = zoneSize - 4;
                int maxH = zoneSize - 4;
                int w = std::rand() % 4 + 4;
                int h = std::rand() % 4 + 4;
                int x = minX + std::rand() % (maxW - w + 1);
                int z = minZ + std::rand() % (maxH - h + 1);

                Room room = {x, z, w, h, x + w / 2, z + h / 2};
                for (int tx = room.x; tx < room.x + room.w; ++tx)
                {
                    for (int tz = room.z; tz < room.z + room.h; ++tz)
                        grid[tx][tz] = 0;
                }
                rooms.push_back(room);
            }
        }

        std::random_device rd;
        std::mt19937 g(rd());
        std::shuffle(rooms.begin(), rooms.end(), g);

        for (size_t i = 0; i < rooms.size() - 1; ++i)
        {
            createCorridor(rooms[i].centerX, rooms[i].centerZ, rooms[i + 1].centerX, rooms[i + 1].centerZ);
        }

        spawnPos = glm::vec3(static_cast<float>(rooms[0].centerX) + 0.5f, 0.5f, static_cast<float>(rooms[0].centerZ) + 0.5f);

        for (int i = 0; i < MAP_SIZE; i++)
        {
            grid[i][0] = grid[i][MAP_SIZE - 1] = grid[0][i] = grid[MAP_SIZE - 1][i] = 1;
        }

        wallVertices.clear();
        for (int x = 0; x < MAP_SIZE; x++)
        {
            for (int z = 0; z < MAP_SIZE; z++)
            {
                if (grid[x][z] == 1)
                    addVisibleFaces(x, z);
            }
        }

        floorCeilVertices.clear();
        for (int x = 0; x < MAP_SIZE; x++)
        {
            for (int z = 0; z < MAP_SIZE; z++)
            {
                if (grid[x][z] == 0)
                {
                    float fx = static_cast<float>(x);
                    float fz = static_cast<float>(z);

                    // Remove AO: All floor and ceiling corners are perfectly white
                    float ao_0_0 = 1.0f;
                    float ao_1_0 = 1.0f;
                    float ao_1_1 = 1.0f;
                    float ao_0_1 = 1.0f;

                    float floorTile[] = {
                        fx, 0.0f, fz, 0.0f, 0.0f, ao_0_0,
                        fx + 1.0f, 0.0f, fz, 1.0f, 0.0f, ao_1_0,
                        fx + 1.0f, 0.0f, fz + 1.0f, 1.0f, 1.0f, ao_1_1,
                        fx + 1.0f, 0.0f, fz + 1.0f, 1.0f, 1.0f, ao_1_1,
                        fx, 0.0f, fz + 1.0f, 0.0f, 1.0f, ao_0_1,
                        fx, 0.0f, fz, 0.0f, 0.0f, ao_0_0};
                    floorCeilVertices.insert(floorCeilVertices.end(), std::begin(floorTile), std::end(floorTile));

                    float ceilTile[] = {
                        fx, 1.0f, fz, 0.0f, 0.0f, ao_0_0,
                        fx, 1.0f, fz + 1.0f, 0.0f, 1.0f, ao_0_1,
                        fx + 1.0f, 1.0f, fz + 1.0f, 1.0f, 1.0f, ao_1_1,
                        fx + 1.0f, 1.0f, fz + 1.0f, 1.0f, 1.0f, ao_1_1,
                        fx + 1.0f, 1.0f, fz, 1.0f, 0.0f, ao_1_0,
                        fx, 1.0f, fz, 0.0f, 0.0f, ao_0_0};
                    floorCeilVertices.insert(floorCeilVertices.end(), std::begin(ceilTile), std::end(ceilTile));
                }
            }
        }
    }
};

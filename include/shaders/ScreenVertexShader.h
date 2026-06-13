#pragma once

// Screen vertex shader for quad (used in SSAO / upscale passes)
const char *screenVertexShaderSource = R"glsl(
    #version 330 core
    layout (location = 0) in vec2 aPos;
    // location 1 used for either texcoords (fullscreen quad) or color (minimap)
    layout (location = 1) in vec3 aAttr1;
    out vec3 Attr1;
    out vec2 TexCoords;
    void main() {
        Attr1 = aAttr1;
        // aAttr1 layout for fullscreen quad is (z, u, v) so use y,z as uv
        TexCoords = aAttr1.yz;
        gl_Position = vec4(aPos.xy, 0.0, 1.0);
    }
)glsl";

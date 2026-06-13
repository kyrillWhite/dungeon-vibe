#pragma once

// SSAO post-process fragment shader: reads low-res color + depth, computes simple AO and does shader-only dithering (Bayer ordered).
const char* ssaoFragmentShaderSource = R"glsl(
    #version 330 core
    out vec4 FragColor;
    in vec2 TexCoords; // full-screen quad TexCoords in [0,1]

    uniform sampler2D colorTex;
    uniform sampler2D depthTex;
    uniform float projNear;
    uniform float projFar;
    uniform float ambientLight;
    uniform float aoLightBlend; // 0..1 (0=full AO,1=ignore AO)

    // Dithering uniforms
    uniform int useDither;       // 0/1
    uniform int ditherPalette;   // 256 / 4096 / 32768
    uniform vec2 screenSize;     // in pixels (low-res)

    // Linearize depth from depth texture
    float linearizeDepth(float z) {
        float ndc = z * 2.0 - 1.0;
        return (2.0 * projNear * projFar) / (projFar + projNear - ndc * (projFar - projNear));
    }

    // 4x4 Bayer matrix values normalized [0,1)
    float bayer4(int x, int y) {
        int idx = y*4 + x;
        // Typical 4x4 Bayer order
        int tbl[16] = int[16](0,8,2,10, 12,4,14,6, 3,11,1,9, 15,7,13,5);
        return (float(tbl[idx]) + 0.5) / 16.0;
    }

    void main() {
        vec2 uv = TexCoords;
        vec4 baseColor = texture(colorTex, uv);
        float z = texture(depthTex, uv).r;
        float linearZ = linearizeDepth(z);

        // Simple AO: sample neighbors in a small radius (in pixels)
        int samples = 8;
        float radius = 3.0; // pixels in low-res buffer
        vec2 px = 1.0 / screenSize;
        float occ = 0.0;
        for (int i = 0; i < samples; ++i) {
            float angle = (float(i) / float(samples)) * 6.2831853;
            vec2 offset = vec2(cos(angle), sin(angle)) * radius * px;
            float nz = texture(depthTex, uv + offset).r;
            float nLinear = linearizeDepth(nz);
            float delta = nLinear - linearZ;
            if (delta < 0.0) {
                float contrib = clamp(-delta / 0.3, 0.0, 1.0);
                occ += contrib;
            }
        }
        occ = occ / float(samples); // 0..1
        float ssao = 1.0 - occ; // 1 = no occlusion, 0 = full occlusion

        // Blend SSAO with AO_LIGHT_BLEND
        float aoBlend = clamp(aoLightBlend, 0.0, 1.0);
        float aoEffect = mix(ssao, 1.0, aoBlend);

        // Combine with ambient
        float finalFactor = max(ambientLight, aoEffect);
        vec3 colorOut = baseColor.rgb * finalFactor;

        // Shader-only ordered dithering (Bayer 4x4) with palette quantization
        if (useDither == 1) {
            // Compute integer pixel coords in low-res buffer
            ivec2 pix = ivec2(floor(uv * screenSize));
            int bx = pix.x & 3; // %4
            int by = pix.y & 3;
            float b = bayer4(bx, by); // 0..1

            int bitsR = 5; int bitsG = 5; int bitsB = 5;
            if (ditherPalette <= 256) { bitsR = 3; bitsG = 3; bitsB = 2; }
            else if (ditherPalette <= 4096) { bitsR = 4; bitsG = 4; bitsB = 4; }
            else { bitsR = 5; bitsG = 5; bitsB = 5; }

            int levelsR = (1 << bitsR) - 1;
            int levelsG = (1 << bitsG) - 1;
            int levelsB = (1 << bitsB) - 1;

            float rq = floor(colorOut.r * float(levelsR) + b) / float(levelsR);
            float gq = floor(colorOut.g * float(levelsG) + b) / float(levelsG);
            float bq = floor(colorOut.b * float(levelsB) + b) / float(levelsB);

            colorOut = vec3(clamp(rq,0.0,1.0), clamp(gq,0.0,1.0), clamp(bq,0.0,1.0));
        }

        FragColor = vec4(colorOut, baseColor.a);
    }
)glsl";

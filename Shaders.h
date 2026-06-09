#ifndef SHADERS_H
#define SHADERS_H

const char* vertexShaderSource = R"glsl(
    #version 330 core
    layout (location = 0) in vec3 aPos;
    layout (location = 1) in vec2 aTexCoords;
    layout (location = 2) in float aAO; // Наш честный вершинный Ambient Occlusion
    
    out vec2 TexCoords;
    out vec3 FragPos;
    out float AO;
    
    uniform mat4 model;
    uniform mat4 view;
    uniform mat4 projection;
    
    void main() {
        gl_Position = projection * view * model * vec4(aPos, 1.0);
        TexCoords = aTexCoords;
        AO = aAO;
        FragPos = vec3(model * vec4(aPos, 1.0));
    }
)glsl";

// Добавленный экранный вершинный шейдер для квадрата (используется в SSAO / апскейл пассах)
const char* screenVertexShaderSource = R"glsl(
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

const char* fragmentShaderSource = R"glsl(
    #version 330 core
    out vec4 FragColor;
    
    in vec2 TexCoords;
    in vec3 FragPos;
    in float AO; // Наш честный вершинный Ambient Occlusion
    
    uniform sampler2D texSampler;
    uniform vec3 cameraPos;
    
    uniform float lightNear;
    uniform float lightFar;
    uniform float ambientLight;
    uniform float lightSharpness; // управляет "центральной" резкостью
    uniform float lightTransitionSoftness; // 0..1: смешение linear<->smoothstep для падения
    uniform float aoLightBlend; // 0..1: 0 = AO полностью, 1 = AO игнорируется
    uniform int debugWhitebox; // 1 = force white, 0 = use texture
    // Torch / player-carried light
    uniform vec3 torchColor; // RGB tint for the player light (yellow-orange)
    uniform float torchFlicker; // flicker multiplier applied to torchColor
    
    void main() {
        // Выбираем базовый цвет: либо белый при debug, либо из текстуры
        vec4 texColor = texture(texSampler, TexCoords);
        vec4 baseColor = (debugWhitebox == 1) ? vec4(1.0, 1.0, 1.0, 1.0) : texColor;
        
        float distance = length(FragPos - cameraPos);
        float denom = max(0.0001, lightFar - lightNear);
        float t = clamp((distance - lightNear) / denom, 0.0, 1.0);

        float softness = clamp(lightTransitionSoftness, 0.0, 1.0);
        float smoothT = mix(t, smoothstep(0.0, 1.0, t), softness);

        float base = max(0.0, 1.0 - smoothT);

        float sharp = max(0.0001, lightSharpness);
        float lightFactor = pow(base, sharp);

        float aoBlend = clamp(aoLightBlend, 0.0, 1.0);
        float aoEffective = mix(AO, 1.0, aoBlend);

        float finalBrightness = max(lightFactor * aoEffective, ambientLight);

        // Tint toward torch color based on lightFactor (near the player the tint is stronger)
        vec3 tint = mix(vec3(1.0,1.0,1.0), torchColor * torchFlicker, lightFactor);

        FragColor = vec4(baseColor.rgb * finalBrightness * tint, baseColor.a);
    }
)glsl";

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

    // dithering uniforms
    uniform int useDither;       // 0/1
    uniform int ditherPalette;   // 256 / 4096 / 32768
    uniform vec2 screenSize;     // in pixels (low-res)

    // linearize depth from depth texture
    float linearizeDepth(float z) {
        float ndc = z * 2.0 - 1.0;
        return (2.0 * projNear * projFar) / (projFar + projNear - ndc * (projFar - projNear));
    }

    // 4x4 Bayer matrix values normalized [0,1)
    float bayer4(int x, int y) {
        int idx = y*4 + x;
        // typical 4x4 Bayer order
        int tbl[16] = int[16](0,8,2,10, 12,4,14,6, 3,11,1,9, 15,7,13,5);
        return (float(tbl[idx]) + 0.5) / 16.0;
    }

    void main() {
        vec2 uv = TexCoords;
        vec4 baseColor = texture(colorTex, uv);
        float z = texture(depthTex, uv).r;
        float linearZ = linearizeDepth(z);

        // simple AO: sample neighbors in a small radius (in pixels)
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

        // blend SSAO with AO_LIGHT_BLEND
        float aoBlend = clamp(aoLightBlend, 0.0, 1.0);
        float aoEffect = mix(ssao, 1.0, aoBlend);

        // combine with ambient
        float finalFactor = max(ambientLight, aoEffect);
        vec3 colorOut = baseColor.rgb * finalFactor;

        // Shader-only ordered dithering (Bayer 4x4) with palette quantization
        if (useDither == 1) {
            // compute integer pixel coords in low-res buffer
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

// Final screen fragment shader: reads (SSAOed) low-res texture and applies radial distortion while upscaling.
const char* screenFragmentShaderSource = R"glsl(
    #version 330 core
    out vec4 FragColor;
    in vec2 TexCoords;
    in vec3 Attr1;
    uniform sampler2D screenTexture;
    uniform int useTexture;
    uniform float mapAlpha;
    uniform float screenDistortion;
    uniform float screenAspect;

    vec2 applyRadialDistortion(vec2 uv) {
        vec2 p = uv - 0.5;
        p.x *= screenAspect;
        float r2 = dot(p,p);
        float k = screenDistortion;
        float factor = 1.0 + k * r2;
        vec2 warped = p * factor;
        warped.x /= screenAspect;
        return warped + 0.5;
    }

    void main() {
        vec2 uv = TexCoords;
        if (useTexture == 1) {
            vec2 wuv = (abs(screenDistortion) > 1e-6) ? applyRadialDistortion(uv) : uv;
            if (wuv.x < 0.0 || wuv.x > 1.0 || wuv.y < 0.0 || wuv.y > 1.0) {
                FragColor = vec4(0.0,0.0,0.0,1.0);
            } else {
                FragColor = texture(screenTexture, wuv);
            }
        } else {
            // For minimap and other non-textured primitives, use Attr1 as RGB color (Attr1.xy available for quad texcoords)
            FragColor = vec4(Attr1.rgb, mapAlpha);
        }
    }
)glsl";

#endif

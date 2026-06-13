#pragma once

const char *fragmentShaderSource = R"glsl(
    #version 330 core
    out vec4 FragColor;
    
    in vec2 TexCoords;
    in vec3 FragPos;
    in float AO; // Our honest vertex Ambient Occlusion
    
    uniform sampler2D texSampler;
    uniform vec3 cameraPos;
    
    uniform float lightNear;
    uniform float lightFar;
    uniform float ambientLight;
    uniform float lightSharpness; // controls "center" sharpness
    uniform float lightTransitionSoftness; // 0..1: blend linear<->smoothstep for falloff
    uniform float aoLightBlend; // 0..1: 0 = full AO, 1 = ignore AO
    uniform int debugWhitebox; // 1 = force white, 0 = use texture
    // Torch / player-carried light
    uniform vec3 torchColor; // RGB tint for the player light (yellow-orange)
    uniform float torchFlicker; // flicker multiplier applied to torchColor
    
    void main() {
        // Choose base color: either white in debug, or from texture
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

#pragma once

// Final screen fragment shader: reads (SSAOed) low-res texture and applies radial distortion while upscaling.
const char *screenFragmentShaderSource = R"glsl(
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

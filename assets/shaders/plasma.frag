#version 330 core

// ============================================================================
// Classic Plasma Shader
// A colorful animated plasma effect with controllable parameters
// ============================================================================

uniform float iTime;
uniform vec3 iResolution;
uniform vec4 iMouse;

// ============================================================================
// CUSTOM PARAMETERS
// ============================================================================

// @slider(min=0.1, max=3.0, default=0.4)
uniform float uSpeed;

// @slider(min=1.0, max=15.0, default=4.0)
uniform float uScale;

// @slider(min=0.5, max=5.0, default=1.5)
uniform float uComplexity;

// @slider(min=0.3, max=1.5, default=0.7)
uniform float uSaturation;

// @slider(min=0.0, max=6.28, default=0.5)
uniform float uColorShift;

// @color(default=1.0,1.0,1.0)
uniform vec3 uTint;

// @checkbox(default=true)
uniform int uAnimated;

out vec4 fragColor;
in vec2 fragCoord;

void main() {
    vec2 uv = fragCoord * 2.0 - 1.0;  // Center coordinates [-1, 1]
    uv.x *= iResolution.x / iResolution.y;  // Aspect ratio correction
    
    float t = uAnimated > 0 ? iTime * uSpeed : 0.0;
    
    // Plasma waves with controllable complexity
    float v1 = sin(uv.x * uScale + t);
    float v2 = sin(uScale * (uv.x * sin(t * 0.5 * uComplexity) + uv.y * cos(t * 0.3 * uComplexity)) + t);
    float v3 = sin(sqrt(uScale * uScale * 4.0 * (uv.x * uv.x + uv.y * uv.y)) - t * 2.0);
    
    // Additional complexity layer
    float v4 = sin(uv.y * uScale * 0.7 + t * 0.8);
    
    // Combine waves
    float v = (v1 + v2 + v3 + v4 * uComplexity) / (3.0 + uComplexity);
    
    // Color palette with controllable shift
    vec3 col;
    float PI = 3.14159;
    col.r = sin(v * PI + uColorShift) * 0.5 + 0.5;
    col.g = sin(v * PI + uColorShift + 2.094) * 0.5 + 0.5;
    col.b = sin(v * PI + uColorShift + 4.188) * 0.5 + 0.5;
    
    // Apply saturation control
    col = pow(col, vec3(uSaturation));
    
    // Apply tint
    col *= uTint;
    
    // Mouse interaction - create ripple on click
    if (iMouse.z > 0.0) {
        vec2 mouseUV = (iMouse.xy / iResolution.xy) * 2.0 - 1.0;
        mouseUV.x *= iResolution.x / iResolution.y;
        float mouseDist = distance(uv, mouseUV);
        float mouseWave = sin(mouseDist * 20.0 - iTime * 5.0) * 0.5 + 0.5;
        mouseWave *= smoothstep(1.0, 0.0, mouseDist);
        col = mix(col, vec3(1.0), mouseWave * 0.3);
    }
    
    fragColor = vec4(col, 1.0);
}

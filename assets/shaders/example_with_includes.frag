#version 330 core

// Example shader demonstrating the #include system

#include "common/noise.glsl"
#include "common/colors.glsl"

uniform float iTime;
uniform vec3 iResolution;

// @slider(min=0.0, max=10.0, default=3.0)
uniform float uScale;

// @slider(min=1, max=8, default=5)
uniform int uOctaves;

// @slider(min=0.0, max=1.0, default=0.5)
uniform float uColorShift;

out vec4 fragColor;
in vec2 fragCoord;

void main() {
    vec2 uv = fragCoord;
    
    // Aspect ratio correction
    float aspectRatio = iResolution.x / iResolution.y;
    uv.x *= aspectRatio;
    
    // Use noise functions from common/noise.glsl
    float n = fbm(uv * uScale + iTime * 0.2, uOctaves);
    
    // Use color functions from common/colors.glsl
    vec3 col = viridis(n + uColorShift);
    
    // Add some variation with value noise
    float detail = valueNoise(uv * 20.0 + iTime);
    col = adjustContrast(col, 1.0 + detail * 0.2);
    
    fragColor = vec4(col, 1.0);
}


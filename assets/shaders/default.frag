#version 330 core

// Shadertoy-compatible uniforms
uniform float iTime;
uniform float iTimeDelta;
uniform vec3 iResolution;
uniform vec4 iMouse;

// Output
out vec4 fragColor;

// Input from vertex shader
in vec2 fragCoord;

// ============================================================================
// Your shader code goes here!
// 
// Available uniforms:
//   iTime       - playback time in seconds
//   iTimeDelta  - time since last frame
//   iResolution - viewport resolution (xy = width/height, z = 1.0)
//   iMouse      - mouse pixel coords. xy: current (if down), zw: click pos
//
// fragCoord is in [0,1] range (normalized UV coordinates)
// ============================================================================

void main() {
    // Normalized pixel coordinates (from 0 to 1)
    vec2 uv = fragCoord;
    
    // Animated gradient
    vec3 col = 0.5 + 0.5 * cos(iTime + uv.xyx * 3.0 + vec3(0, 2, 4));
    
    // Add some circular ripples
    vec2 center = vec2(0.5, 0.5);
    float dist = distance(uv, center);
    float ripple = sin(dist * 30.0 - iTime * 3.0) * 0.5 + 0.5;
    ripple *= smoothstep(0.5, 0.0, dist);
    
    col = mix(col, vec3(1.0), ripple * 0.3);
    
    // Mouse interaction - brighten near mouse
    if (iMouse.z > 0.0) {  // If mouse is down
        vec2 mouseUV = iMouse.xy / iResolution.xy;
        float mouseDist = distance(uv, mouseUV);
        col += vec3(0.3) * smoothstep(0.15, 0.0, mouseDist);
    }
    
    // Vignette effect
    float vignette = 1.0 - dot(uv - 0.5, uv - 0.5) * 1.5;
    col *= vignette;
    
    fragColor = vec4(col, 1.0);
}


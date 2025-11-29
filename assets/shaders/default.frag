#version 330 core

// ============================================================================
// Default Shader
// An animated gradient with ripples and vignette effect
// ============================================================================

// Shadertoy-compatible uniforms
uniform float iTime;
uniform float iTimeDelta;
uniform vec3 iResolution;
uniform vec4 iMouse;

// ============================================================================
// CUSTOM PARAMETERS
// ============================================================================

// @slider(min=0.1, max=5.0, default=0.8)
uniform float uSpeed;

// @slider(min=5.0, max=60.0, default=25.0)
uniform float uRippleFrequency;

// @slider(min=0.0, max=1.0, default=0.25)
uniform float uRippleIntensity;

// @slider(min=0.0, max=2.0, default=0.8)
uniform float uVignetteStrength;

// @slider(min=0.5, max=5.0, default=2.5)
uniform float uColorFrequency;

// @color(default=0.5,0.5,0.5)
uniform vec3 uColorBase;

// @checkbox(default=true)
uniform int uAnimated;

// Output
out vec4 fragColor;

// Input from vertex shader
in vec2 fragCoord;

void main() {
    // Normalized pixel coordinates (from 0 to 1)
    vec2 uv = fragCoord;
    
    // Aspect ratio for circular shapes
    float aspectRatio = iResolution.x / iResolution.y;
    vec2 uvCorrected = uv;
    uvCorrected.x = (uv.x - 0.5) * aspectRatio + 0.5;
    
    // Time
    float t = uAnimated > 0 ? iTime * uSpeed : 0.0;
    
    // Animated gradient with controllable frequency
    vec3 col = uColorBase + 0.5 * cos(t + uv.xyx * uColorFrequency + vec3(0, 2, 4));
    
    // Add circular ripples (aspect corrected)
    vec2 center = vec2(0.5, 0.5);
    float dist = distance(uvCorrected, center);
    float ripple = sin(dist * uRippleFrequency - t * 3.0) * 0.5 + 0.5;
    ripple *= smoothstep(0.5, 0.0, dist);
    
    col = mix(col, vec3(1.0), ripple * uRippleIntensity);
    
    // Mouse interaction - brighten near mouse (aspect corrected)
    if (iMouse.z > 0.0) {
        vec2 mouseUV = iMouse.xy / iResolution.xy;
        vec2 mouseUVCorrected = mouseUV;
        mouseUVCorrected.x = (mouseUV.x - 0.5) * aspectRatio + 0.5;
        float mouseDist = distance(uvCorrected, mouseUVCorrected);
        col += vec3(0.3) * smoothstep(0.15, 0.0, mouseDist);
    }
    
    // Vignette effect (aspect corrected)
    vec2 vignetteUV = (uv - 0.5) * vec2(aspectRatio, 1.0);
    float vignette = 1.0 - dot(vignetteUV, vignetteUV) * uVignetteStrength;
    col *= vignette;
    
    fragColor = vec4(col, 1.0);
}

#version 330 core

// ============================================================================
// Annotated Shader Demo
// This shader demonstrates the annotation system for auto-generating UI controls
// ============================================================================

// Built-in Shadertoy-style uniforms (no annotation needed)
uniform float iTime;
uniform float iTimeDelta;
uniform vec3 iResolution;
uniform vec4 iMouse;

// ============================================================================
// CUSTOM UNIFORMS WITH ANNOTATIONS
// These will auto-generate ImGui controls in the Properties panel!
// ============================================================================

// @slider(min=0.1, max=5.0, default=0.8)
uniform float uSpeed;

// @slider(min=1.0, max=20.0, default=8.0)
uniform float uFrequency;

// @slider(min=0.0, max=1.0, default=0.4)
uniform float uAmplitude;

// @color(default=0.05,0.2,0.5)
uniform vec3 uColor1;

// @color(default=0.95,0.3,0.4)
uniform vec3 uColor2;

// @slider(min=0.0, max=1.0, default=0.25)
uniform float uVignetteStrength;

// @slider(min=0.0, max=1.0, default=0.3)
uniform float uGlowIntensity;

// @checkbox(default=true)
uniform int uAnimated;

// Output
out vec4 fragColor;

// Input from vertex shader
in vec2 fragCoord;

// ============================================================================
// Shader code
// ============================================================================

void main() {
    // Aspect ratio correction
    float aspectRatio = iResolution.x / iResolution.y;
    
    // UV coordinates with aspect ratio correction for circular shapes
    vec2 uv = fragCoord;
    vec2 uvCorrected = uv;
    uvCorrected.x = (uv.x - 0.5) * aspectRatio + 0.5;
    
    vec2 center = vec2(0.5);
    vec2 centerCorrected = vec2(0.5);
    
    // Time-based animation (can be toggled)
    float t = uAnimated > 0 ? iTime * uSpeed : 0.0;
    
    // Create animated pattern (use corrected UV for circular distance)
    float dist = distance(uvCorrected, centerCorrected);
    float wave = sin(dist * uFrequency * 10.0 - t * 3.0) * uAmplitude;
    wave += sin(uv.x * aspectRatio * uFrequency * 5.0 + t * 2.0) * uAmplitude * 0.5;
    wave += sin(uv.y * uFrequency * 5.0 - t * 1.5) * uAmplitude * 0.5;
    
    // Normalize wave to [0, 1]
    wave = wave * 0.5 + 0.5;
    
    // Mix between two user-defined colors
    vec3 col = mix(uColor1, uColor2, wave);
    
    // Add glow effect near center (circular with aspect correction)
    float glow = smoothstep(0.5, 0.0, dist) * uGlowIntensity;
    col += vec3(glow);
    
    // Mouse interaction - brighten on click (with aspect correction)
    if (iMouse.z > 0.0) {
        vec2 mouseUV = iMouse.xy / iResolution.xy;
        vec2 mouseUVCorrected = mouseUV;
        mouseUVCorrected.x = (mouseUV.x - 0.5) * aspectRatio + 0.5;
        float mouseDist = distance(uvCorrected, mouseUVCorrected);
        col += vec3(0.3) * smoothstep(0.2, 0.0, mouseDist);
    }
    
    // Vignette effect (with aspect correction for even vignette)
    vec2 vignetteUV = (uv - center) * vec2(aspectRatio, 1.0);
    float vignette = 1.0 - dot(vignetteUV, vignetteUV) * uVignetteStrength * 2.0;
    col *= vignette;
    
    // Clamp and output
    col = clamp(col, 0.0, 1.0);
    fragColor = vec4(col, 1.0);
}


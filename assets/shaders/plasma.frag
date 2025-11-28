#version 330 core

uniform float iTime;
uniform vec3 iResolution;
uniform vec4 iMouse;

out vec4 fragColor;
in vec2 fragCoord;

// Classic plasma shader - great for testing hot reload!
// Try changing the colors or wave frequencies while the app is running.

void main() {
    vec2 uv = fragCoord * 2.0 - 1.0;  // Center coordinates
    uv.x *= iResolution.x / iResolution.y;  // Aspect ratio correction
    
    float t = iTime * 0.5;
    
    // Plasma waves
    float v1 = sin(uv.x * 5.0 + t);
    float v2 = sin(5.0 * (uv.x * sin(t * 0.5) + uv.y * cos(t * 0.3)) + t);
    float v3 = sin(sqrt(100.0 * (uv.x * uv.x + uv.y * uv.y)) - t * 2.0);
    
    // Combine waves
    float v = v1 + v2 + v3;
    
    // Color palette
    vec3 col;
    col.r = sin(v * 3.14159) * 0.5 + 0.5;
    col.g = sin(v * 3.14159 + 2.094) * 0.5 + 0.5;
    col.b = sin(v * 3.14159 + 4.188) * 0.5 + 0.5;
    
    // Boost saturation
    col = pow(col, vec3(0.8));
    
    fragColor = vec4(col, 1.0);
}


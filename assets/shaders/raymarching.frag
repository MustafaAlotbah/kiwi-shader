#version 330 core

uniform float iTime;
uniform vec3 iResolution;
uniform vec4 iMouse;

out vec4 fragColor;
in vec2 fragCoord;

// Simple raymarching demo - a rotating sphere with floor reflection

#define MAX_STEPS 100
#define MAX_DIST 100.0
#define SURF_DIST 0.001

// Signed distance function for a sphere
float sdSphere(vec3 p, float r) {
    return length(p) - r;
}

// Signed distance function for the floor plane
float sdPlane(vec3 p) {
    return p.y + 1.0;
}

// Scene SDF - combines all objects
float getDist(vec3 p) {
    // Animated sphere position
    vec3 spherePos = vec3(sin(iTime) * 0.5, 0.0, cos(iTime) * 0.5 + 4.0);
    float sphere = sdSphere(p - spherePos, 1.0);
    float plane = sdPlane(p);
    
    return min(sphere, plane);
}

// Calculate normal via gradient
vec3 getNormal(vec3 p) {
    float d = getDist(p);
    vec2 e = vec2(0.001, 0.0);
    
    vec3 n = d - vec3(
        getDist(p - e.xyy),
        getDist(p - e.yxy),
        getDist(p - e.yyx)
    );
    
    return normalize(n);
}

// Raymarching
float rayMarch(vec3 ro, vec3 rd) {
    float d = 0.0;
    
    for (int i = 0; i < MAX_STEPS; i++) {
        vec3 p = ro + rd * d;
        float ds = getDist(p);
        d += ds;
        if (d > MAX_DIST || ds < SURF_DIST) break;
    }
    
    return d;
}

// Simple lighting
float getLight(vec3 p) {
    vec3 lightPos = vec3(2.0, 5.0, 1.0);
    lightPos.xz += vec2(sin(iTime), cos(iTime)) * 2.0;
    
    vec3 l = normalize(lightPos - p);
    vec3 n = getNormal(p);
    
    float dif = clamp(dot(n, l), 0.0, 1.0);
    
    // Soft shadows
    float d = rayMarch(p + n * SURF_DIST * 2.0, l);
    if (d < length(lightPos - p)) dif *= 0.2;
    
    return dif;
}

void main() {
    vec2 uv = (fragCoord - 0.5) * 2.0;
    uv.x *= iResolution.x / iResolution.y;
    
    // Camera
    vec3 ro = vec3(0.0, 1.0, 0.0);  // Ray origin (camera position)
    vec3 rd = normalize(vec3(uv.x, uv.y, 1.0));  // Ray direction
    
    // Raymarch
    float d = rayMarch(ro, rd);
    
    vec3 col = vec3(0.0);
    
    if (d < MAX_DIST) {
        vec3 p = ro + rd * d;
        float dif = getLight(p);
        col = vec3(dif);
        
        // Add some color based on position
        col *= vec3(0.8, 0.9, 1.0);
        
        // Fog
        col = mix(col, vec3(0.1, 0.1, 0.15), 1.0 - exp(-0.05 * d * d));
    } else {
        // Sky gradient
        col = mix(vec3(0.1, 0.1, 0.15), vec3(0.3, 0.4, 0.6), uv.y * 0.5 + 0.5);
    }
    
    // Gamma correction
    col = pow(col, vec3(0.4545));
    
    fragColor = vec4(col, 1.0);
}


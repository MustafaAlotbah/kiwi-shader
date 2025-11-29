#version 330 core

// ============================================================================
// Raymarching Demo
// A raymarched scene with sphere, ground, sky and controllable camera
// ============================================================================

uniform float iTime;
uniform vec3 iResolution;
uniform vec4 iMouse;

// ============================================================================
// ANIMATION
// ============================================================================

// @slider(min=0.0, max=3.0, default=0.5)
uniform float uAnimSpeed;

// @checkbox(default=true)
uniform int uAnimated;

// ============================================================================
// CAMERA
// ============================================================================

// @slider(min=-5.0, max=5.0, default=0.0)
uniform float uCameraPosX;

// @slider(min=0.0, max=5.0, default=2.0)
uniform float uCameraPosY;

// @slider(min=-5.0, max=5.0, default=-3.0)
uniform float uCameraPosZ;

// @slider(min=-1.57, max=1.57, default=-0.2)
uniform float uCameraPitch;

// @slider(min=-3.14, max=3.14, default=0.0)
uniform float uCameraYaw;

// @slider(min=0.5, max=3.0, default=1.2)
uniform float uFOV;

// ============================================================================
// SPHERE
// ============================================================================

// @slider(min=0.3, max=2.0, default=0.8)
uniform float uSphereRadius;

// @slider(min=0.0, max=3.0, default=1.5)
uniform float uOrbitRadius;

// @color(default=0.85,0.15,0.1)
uniform vec3 uSphereColor;

// ============================================================================
// GROUND
// ============================================================================

// @color(default=0.35,0.3,0.25)
uniform vec3 uGroundColor;

// @slider(min=0.0, max=1.0, default=0.15)
uniform float uGroundReflect;

// ============================================================================
// SKY & SUN
// ============================================================================

// @color(default=0.1,0.15,0.35)
uniform vec3 uSkyColorTop;

// @color(default=0.55,0.65,0.85)
uniform vec3 uSkyColorHorizon;

// @color(default=1.0,0.9,0.7)
uniform vec3 uSunColor;

// @slider(min=1.0, max=100.0, default=64.0)
uniform float uSunSharpness;

// @slider(min=-3.14, max=3.14, default=0.8)
uniform float uSunAngleX;

// @slider(min=0.0, max=1.57, default=0.6)
uniform float uSunAngleY;

// ============================================================================
// ATMOSPHERE
// ============================================================================

// @slider(min=0.0, max=0.2, default=0.015)
uniform float uFogDensity;

// @checkbox(default=true)
uniform int uShadows;

out vec4 fragColor;
in vec2 fragCoord;

#define MAX_STEPS 100
#define MAX_DIST 100.0
#define SURF_DIST 0.001

// ============================================================================
// SDF Functions
// ============================================================================

float sdSphere(vec3 p, float r) {
    return length(p) - r;
}

float sdPlane(vec3 p, float h) {
    return p.y - h;
}

// Get sun direction from angles
vec3 getSunDir() {
    return normalize(vec3(
        cos(uSunAngleY) * sin(uSunAngleX),
        sin(uSunAngleY),
        cos(uSunAngleY) * cos(uSunAngleX)
    ));
}

// Get sphere position (animated)
vec3 getSpherePos() {
    float t = uAnimated > 0 ? iTime * uAnimSpeed : 0.0;
    return vec3(
        sin(t) * uOrbitRadius,
        uSphereRadius - 1.0,  // Rest on ground (ground is at y=-1)
        cos(t) * uOrbitRadius + 3.0  // In front of default camera
    );
}

// Scene SDF - returns distance and material ID
vec2 getDist(vec3 p) {
    vec3 spherePos = getSpherePos();
    float sphere = sdSphere(p - spherePos, uSphereRadius);
    float ground = sdPlane(p, -1.0);
    
    // Return distance and material ID (0 = ground, 1 = sphere)
    if (sphere < ground) {
        return vec2(sphere, 1.0);
    }
    return vec2(ground, 0.0);
}

float getDistSimple(vec3 p) {
    return getDist(p).x;
}

vec3 getNormal(vec3 p) {
    float d = getDistSimple(p);
    vec2 e = vec2(0.001, 0.0);
    
    vec3 n = d - vec3(
        getDistSimple(p - e.xyy),
        getDistSimple(p - e.yxy),
        getDistSimple(p - e.yyx)
    );
    
    return normalize(n);
}

// Raymarching - returns (distance, materialID)
vec2 rayMarch(vec3 ro, vec3 rd) {
    float d = 0.0;
    float matID = -1.0;
    
    for (int i = 0; i < MAX_STEPS; i++) {
        vec3 p = ro + rd * d;
        vec2 res = getDist(p);
        float ds = res.x;
        matID = res.y;
        d += ds;
        if (d > MAX_DIST || ds < SURF_DIST) break;
    }
    
    return vec2(d, matID);
}

// Soft shadow
float getShadow(vec3 p, vec3 lightDir) {
    if (uShadows == 0) return 1.0;
    
    float d = rayMarch(p + lightDir * 0.02, lightDir).x;
    return d > 10.0 ? 1.0 : 0.2;
}

// Sky color based on direction
vec3 getSkyColor(vec3 rd) {
    vec3 sunDir = getSunDir();
    
    // Gradient from top to horizon
    float t = pow(max(0.0, rd.y), 0.5);
    vec3 sky = mix(uSkyColorHorizon, uSkyColorTop, t);
    
    // Sun disc and glow
    float sunDot = max(0.0, dot(rd, sunDir));
    vec3 sun = uSunColor * pow(sunDot, uSunSharpness);
    
    // Sun glow (softer halo)
    vec3 sunGlow = uSunColor * 0.3 * pow(sunDot, 4.0);
    
    return sky + sun + sunGlow;
}

// Camera rotation matrix
mat3 getCameraMatrix() {
    float cp = cos(uCameraPitch);
    float sp = sin(uCameraPitch);
    float cy = cos(uCameraYaw);
    float sy = sin(uCameraYaw);
    
    // Yaw (around Y axis)
    mat3 yawMat = mat3(
        cy, 0, -sy,
        0, 1, 0,
        sy, 0, cy
    );
    
    // Pitch (around X axis)
    mat3 pitchMat = mat3(
        1, 0, 0,
        0, cp, sp,
        0, -sp, cp
    );
    
    return yawMat * pitchMat;
}

void main() {
    vec2 uv = (fragCoord - 0.5) * 2.0;
    uv.x *= iResolution.x / iResolution.y;
    
    // Camera setup
    vec3 ro = vec3(uCameraPosX, uCameraPosY, uCameraPosZ);
    vec3 rd = normalize(vec3(uv.x, uv.y, uFOV));
    
    // Apply camera rotation
    mat3 camMat = getCameraMatrix();
    rd = camMat * rd;
    
    // Sun direction for lighting
    vec3 sunDir = getSunDir();
    
    // Raymarch
    vec2 result = rayMarch(ro, rd);
    float d = result.x;
    float matID = result.y;
    
    vec3 col = vec3(0.0);
    
    if (d < MAX_DIST) {
        vec3 p = ro + rd * d;
        vec3 n = getNormal(p);
        
        // Lighting
        float diff = max(0.0, dot(n, sunDir));
        float amb = 0.1;
        float shadow = getShadow(p, sunDir);
        
        // Material color based on ID
        vec3 matColor;
        if (matID > 0.5) {
            // Sphere
            matColor = uSphereColor;
        } else {
            // Ground - checkerboard pattern for visual interest
            float checker = mod(floor(p.x) + floor(p.z), 2.0);
            matColor = mix(uGroundColor * 0.7, uGroundColor, checker);
            
            // Ground reflection of sky
            vec3 reflDir = reflect(rd, n);
            vec3 reflCol = getSkyColor(reflDir);
            matColor = mix(matColor, reflCol, uGroundReflect * 0.3);
        }
        
        // Combine lighting
        col = matColor * (amb + diff * shadow) * uSunColor;
        
        // Specular highlight
        vec3 reflDir = reflect(-sunDir, n);
        float spec = pow(max(0.0, dot(reflDir, -rd)), 32.0);
        col += uSunColor * spec * 0.5 * shadow;
        
        // Fog
        vec3 fogColor = mix(uSkyColorHorizon, uSkyColorTop, 0.3);
        col = mix(col, fogColor, 1.0 - exp(-uFogDensity * d * d));
    } else {
        // Sky
        col = getSkyColor(rd);
    }
    
    // Gamma correction
    col = pow(col, vec3(0.4545));
    
    // Clamp
    col = clamp(col, 0.0, 1.0);
    
    fragColor = vec4(col, 1.0);
}

#version 330 core

// =============================================================================
// Modular Raymarching Architecture
// =============================================================================
// A well-structured raymarching shader demonstrating separation of concerns.
//
// Architecture Overview:
//   1. Scene Definition    - What objects exist and where
//   2. Material System     - How objects look (color, properties)
//   3. Camera System       - Where we're viewing from
//   4. Lighting System     - How light interacts with surfaces
//   5. Sky/Environment     - Background rendering
//   6. Raymarching Core    - The actual rendering algorithm
//   7. Main Pipeline       - Orchestrates everything
//
// This modular approach makes it easy to:
//   - Add/remove objects
//   - Change materials
//   - Modify lighting
//   - Swap rendering techniques
//   - Experiment without breaking other systems
// =============================================================================

#include "sdf/primitives.glsl"
#include "sdf/operations.glsl"

uniform float iTime;
uniform vec3 iResolution;

// =============================================================================
// RAYMARCHING CONFIGURATION
// =============================================================================

// Maximum ray march iterations (higher = more accurate but slower)
// @group("Raymarching")
// @slider(min=10, max=300, default=100)
uniform int maxSteps;

// Maximum ray travel distance
// @group("Raymarching")
// @slider(min=10.0, max=500.0, default=100.0)
uniform float maxDistance;

// Surface detection threshold (lower = more precise but slower)
// @group("Raymarching")
// @slider(min=0.0001, max=0.1, default=0.001)
uniform float surfaceThreshold;

// =============================================================================
// MATERIAL IDS
// =============================================================================
// Material IDs (used to identify which object was hit)
#define MAT_GROUND 1.0
#define MAT_SPHERE_1 2.0
#define MAT_SPHERE_2 3.0
#define MAT_SPHERE_BLENDED 3.5  // Blended spheres - uses blendFactor for color mixing
#define MAT_BOX 4.0

// =============================================================================
// UNIFORMS - Scene Configuration
// =============================================================================

// Sky colors
// @group("Sky")
// @color(default=0.3,0.5,0.8)
uniform vec3 skyColorTop;

// @group("Sky")
// @color(default=0.6,0.7,0.9)
uniform vec3 skyColorHorizon;

// Ground
// @group("Ground")
// @color(default=0.3,0.25,0.2)
uniform vec3 groundColor;

// @group("Ground")
// @slider(min=-50.0, max=50.0, default=0.0)
uniform float groundHeight;

// Sphere 1
// @group("Sphere 1")
// @color(default=0.9,0.3,0.2)
uniform vec3 sphere1Color;

// @group("Sphere 1")
// @vec3(default=-1.5,0.8,6.0)
uniform vec3 sphere1Position;

// @group("Sphere 1")
// @slider(min=0.1, max=2.0, default=0.8)
uniform float sphere1Radius;

// Sphere 2
// @group("Sphere 2")
// @color(default=0.2,0.6,0.9)
uniform vec3 sphere2Color;

// @group("Sphere 2")
// @vec3(default=1.5,0.8,6.0)
uniform vec3 sphere2Position;

// @group("Sphere 2")
// @slider(min=0.1, max=2.0, default=0.8)
uniform float sphere2Radius;

// Sphere Blending (Smooth Union)
// @group("Sphere Blending")
// @checkbox(default=false)
uniform int enableSphereBlend;

// @group("Sphere Blending")
// @slider(min=0.0, max=2.0, default=0.5)
uniform float sphereBlendFactor;

// Box
// @group("Box")
// @color(default=0.4,0.8,0.3)
uniform vec3 boxColor;

// @group("Box")
// @vec3(default=0.0,0.6,8.0)
uniform vec3 boxPosition;

// @group("Box")
// @vec3(default=0.6,0.6,0.6)
uniform vec3 boxSize;

// =============================================================================
// CAMERA (Built-in uniforms from interactive camera controller)
// =============================================================================
// These uniforms are automatically provided by the framework.
// Use WASD to move, Right-click + drag to look around, Scroll to zoom.

uniform vec3 uCameraPosition;    // Camera world position
uniform vec3 uCameraForward;     // Normalized forward direction
uniform vec3 uCameraRight;       // Normalized right direction
uniform vec3 uCameraUp;          // Normalized up direction
uniform float uCameraFOV;        // Field of view in degrees

// Lighting (direction FROM light source, pointing down into scene)
// @group("Lighting")
// @vec3(default=0.5,0.8,-0.3)
uniform vec3 lightDirection;

// @group("Lighting")
// @slider(min=0.0, max=1.0, default=0.15)
uniform float ambientStrength;

// @group("Lighting")
// @slider(min=0.0, max=2.0, default=0.8)
uniform float diffuseStrength;

// @group("Lighting")
// @slider(min=0.0, max=2.0, default=0.3)
uniform float specularStrength;

// =============================================================================
// DATA STRUCTURES
// =============================================================================

// Result from scene query - contains distance and material ID
struct SceneResult {
    float distance;    // Distance to nearest surface
    float materialID;  // Which object/material was hit
    float blendFactor; // For smooth blending between materials (0-1)
};

// Material properties for shading
struct Material {
    vec3 color;
    float shininess;
    float reflectivity;
};

// =============================================================================
// SCENE DEFINITION
// =============================================================================
// This is where we define what exists in our world.
// Easy to add/remove/modify objects here.

SceneResult getScene(vec3 point) {
    SceneResult result;
    result.distance = maxDistance;
    result.materialID = 0.0;
    result.blendFactor = 0.0;
    
    // Ground plane (horizontal plane at y = groundHeight)
    // Simple SDF: distance above/below the plane
    float ground = point.y - groundHeight;
    if (ground < result.distance) {
        result.distance = ground;
        result.materialID = MAT_GROUND;
    }
    
    // Spheres - either separate or blended
    float sphere1 = sdSphere(point - sphere1Position, sphere1Radius);
    float sphere2 = sdSphere(point - sphere2Position, sphere2Radius);
    
    if (enableSphereBlend > 0) {
        // Smooth union of both spheres
        float blendedSpheres = opSmoothUnion(sphere1, sphere2, sphereBlendFactor);
        if (blendedSpheres < result.distance) {
            result.distance = blendedSpheres;
            result.materialID = MAT_SPHERE_BLENDED;
            
            // Calculate smooth blend factor between sphere colors
            // 0.0 = fully sphere2 color, 1.0 = fully sphere1 color
            result.blendFactor = clamp(0.5 + 0.5 * (sphere2 - sphere1) / max(sphereBlendFactor, 0.001), 0.0, 1.0);
        }
    } else {
        // Separate spheres (no blending)
        if (sphere1 < result.distance) {
            result.distance = sphere1;
            result.materialID = MAT_SPHERE_1;
        }
        if (sphere2 < result.distance) {
            result.distance = sphere2;
            result.materialID = MAT_SPHERE_2;
        }
    }
    
    // Box
    float box = sdBox(point - boxPosition, boxSize);
    if (box < result.distance) {
        result.distance = box;
        result.materialID = MAT_BOX;
    }
    
    return result;
}

// Simplified distance-only query (for normal calculation)
float getSceneDistance(vec3 point) {
    return getScene(point).distance;
}

// =============================================================================
// MATERIAL SYSTEM
// =============================================================================
// Maps material IDs to actual material properties.
// Easy to add new materials or modify existing ones.

Material getMaterial(float materialID, float blendFactor) {
    Material mat;
    mat.shininess = 32.0;
    mat.reflectivity = 0.0;
    
    if (materialID == MAT_GROUND) {
        mat.color = groundColor;
        mat.shininess = 8.0;
    }
    else if (materialID == MAT_SPHERE_1) {
        mat.color = sphere1Color;
        mat.shininess = 64.0;
    }
    else if (materialID == MAT_SPHERE_2) {
        mat.color = sphere2Color;
        mat.shininess = 64.0;
    }
    else if (materialID == MAT_SPHERE_BLENDED) {
        // Smooth color transition between both sphere colors
        // blendFactor: 0.0 = sphere2Color, 1.0 = sphere1Color
        mat.color = mix(sphere2Color, sphere1Color, blendFactor);
        mat.shininess = 64.0;
    }
    else if (materialID == MAT_BOX) {
        mat.color = boxColor;
        mat.shininess = 32.0;
    }
    else {
        // Default material
        mat.color = vec3(0.5);
    }
    
    return mat;
}

// =============================================================================
// CAMERA SYSTEM
// =============================================================================
// Uses the built-in camera uniforms provided by the framework.
// Controls: WASD to move, Right-click + drag to look, Scroll to zoom.

vec3 getCameraRay(vec2 uv) {
    // Convert FOV from degrees to a focal length factor
    float focalLength = 1.0 / tan(radians(uCameraFOV) * 0.5);
    
    // Generate ray direction using camera basis vectors
    vec3 rayDir = normalize(
        uCameraForward * focalLength +
        uCameraRight * uv.x +
        uCameraUp * uv.y
    );
    return rayDir;
}

// =============================================================================
// SURFACE NORMAL CALCULATION
// =============================================================================
// Calculates the direction a surface is facing.
// Uses gradient of distance field (finite differences).

vec3 calculateNormal(vec3 point) {
    vec2 epsilon = vec2(0.001, 0.0);
    
    float centerDist = getSceneDistance(point);
    
    vec3 normal = centerDist - vec3(
        getSceneDistance(point - epsilon.xyy),
        getSceneDistance(point - epsilon.yxy),
        getSceneDistance(point - epsilon.yyx)
    );
    
    return normalize(normal);
}

// =============================================================================
// RAYMARCHING CORE
// =============================================================================
// The main rendering algorithm.
// Marches a ray through the scene until it hits something.

SceneResult rayMarch(vec3 rayOrigin, vec3 rayDirection) {
    float totalDistance = 0.0;
    SceneResult result;
    result.distance = -1.0;
    result.materialID = 0.0;
    result.blendFactor = 0.0;
    
    for (int i = 0; i < maxSteps; i++) {
        vec3 currentPos = rayOrigin + rayDirection * totalDistance;
        result = getScene(currentPos);
        
        totalDistance += result.distance;
        
        // Hit surface
        if (result.distance < surfaceThreshold) {
            result.distance = totalDistance;
            return result;
        }
        
        // Too far
        if (totalDistance > maxDistance) {
            result.distance = -1.0;
            return result;
        }
    }
    
    result.distance = -1.0;
    return result;
}

// =============================================================================
// LIGHTING SYSTEM
// =============================================================================
// Calculates how light interacts with surfaces.
// Supports ambient, diffuse, and specular lighting.

vec3 calculateLighting(
    vec3 point,
    vec3 normal,
    vec3 viewDir,
    Material material,
    vec3 lightDir
) {
    // Ambient light (base illumination)
    vec3 ambient = material.color * ambientStrength;
    
    // Diffuse light (Lambert)
    // Surfaces facing light are brighter
    float diffuseFactor = max(dot(normal, lightDir), 0.0);
    vec3 diffuse = material.color * diffuseFactor * diffuseStrength;
    
    // Specular light (Blinn-Phong)
    // Creates shiny highlights
    vec3 halfVector = normalize(lightDir + viewDir);
    float specularFactor = pow(max(dot(normal, halfVector), 0.0), material.shininess);
    vec3 specular = vec3(1.0) * specularFactor * specularStrength;
    
    // Combine all lighting components
    return ambient + diffuse + specular;
}

// =============================================================================
// SKY / ENVIRONMENT
// =============================================================================
// Renders the background when rays don't hit anything.
// Simple gradient sky.

vec3 getSkyColor(vec3 rayDirection) {
    // Gradient based on vertical direction
    float t = rayDirection.y * 0.5 + 0.5; // Map -1..1 to 0..1
    return mix(skyColorHorizon, skyColorTop, t);
}

// =============================================================================
// MAIN RENDERING PIPELINE
// =============================================================================
// Orchestrates all systems to produce final image.

out vec4 fragColor;
in vec2 fragCoord;

void main() {
    // 1. Setup screen coordinates (NDC: -1 to 1)
    vec2 uv = (fragCoord - 0.5) * 2.0;
    uv.x *= iResolution.x / iResolution.y;
    
    // 2. Generate camera ray using built-in camera uniforms
    // Camera is controlled via WASD + mouse in the application
    vec3 rayDir = getCameraRay(uv);
    
    // 3. March the ray from camera position
    SceneResult hit = rayMarch(uCameraPosition, rayDir);
    
    vec3 finalColor;
    
    if (hit.distance > 0.0) {
        // 4. We hit something!
        vec3 hitPoint = uCameraPosition + rayDir * hit.distance;
        vec3 normal = calculateNormal(hitPoint);
        vec3 viewDir = -rayDir;
        
        // 5. Get material properties (with blend factor for smooth color transitions)
        Material material = getMaterial(hit.materialID, hit.blendFactor);
        
        // 6. Calculate lighting
        vec3 lightDir = normalize(lightDirection);
        finalColor = calculateLighting(hitPoint, normal, viewDir, material, lightDir);
    }
    else {
        // 7. No hit - render sky
        finalColor = getSkyColor(rayDir);
    }
    
    // 8. Output final color
    fragColor = vec4(finalColor, 1.0);
}

// =============================================================================
// CAMERA CONTROLS (Interactive!)
// =============================================================================
//
// The camera is controlled via keyboard and mouse:
//   - W/S or Up/Down:     Move forward/backward
//   - A/D or Left/Right:  Strafe left/right
//   - Q/Space:            Move up
//   - E/Ctrl:             Move down
//   - Right-click + Drag: Look around (rotate camera)
//   - Mouse Scroll:       Zoom (change FOV)
//   - Shift:              Sprint (2x movement speed)
//
// =============================================================================
// EXTENDING THIS SHADER
// =============================================================================
//
// To add a new object:
//   1. Add uniforms for its properties (color, position, size)
//   2. Define a new material ID constant
//   3. Add it to getScene() function
//   4. Add material properties in getMaterial()
//
// To add shadows:
//   1. Create a shadowMarch() function similar to rayMarch()
//   2. Call it from calculateLighting()
//   3. Multiply diffuse/specular by shadow factor
//
// To add reflections:
//   1. In main(), if material.reflectivity > 0
//   2. Calculate reflection ray: reflect(rayDir, normal)
//   3. March reflection ray
//   4. Mix reflected color with surface color
//
// To animate objects:
//   1. Use iTime in position calculations
//   2. Example: sphere1Position + vec3(sin(iTime), 0, 0)
//
// =============================================================================


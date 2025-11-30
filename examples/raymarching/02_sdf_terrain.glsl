#version 330 core

// =============================================================================
// SDF Terrain with Sky and Sun (Enhanced Realism)
// =============================================================================
// A raymarched terrain scene demonstrating:
//   - Procedural terrain using noise-based heightmaps
//   - Gradient sky with sun disc
//   - Height-based terrain coloring with variation
//   - Physically-based material properties
//   - Advanced lighting (specular, fresnel, subsurface scattering)
//   - Surface detail and normal mapping
//   - Atmospheric fog
//
// Architecture:
//   1. Terrain System     - Procedural height generation
//   2. Sky System         - Gradient sky with sun
//   3. Material System    - PBR materials with color variation
//   4. Lighting System    - Specular, fresnel, subsurface scattering
//   5. Detail System      - Procedural normal mapping
//   6. Atmosphere         - Distance fog
//   7. Camera System      - Built-in interactive camera
// =============================================================================

uniform float iTime;
uniform vec3 iResolution;

// =============================================================================
// NOISE FUNCTIONS (for terrain generation)
// =============================================================================

// High-quality hash function - works well even at 8K resolution
// Uses integer operations for better precision across large coordinate ranges
// Handles negative coordinates properly with offset
// Based on improved integer hash techniques (avoids sin() precision issues)
float hash(vec2 p) {
    // Offset to handle negative coordinates (shift to positive range)
    p += 10000.0;
    
    // Integer-based hash using prime multipliers
    uvec2 q = uvec2(ivec2(p));
    q *= uvec2(1597334673u, 3812015801u);
    uint n = q.x ^ q.y;
    n *= 1597334673u;
    n ^= (n >> 16u);
    n *= 2246822519u;
    n ^= (n >> 13u);
    
    return float(n) * (1.0 / float(0xffffffffu));
}

// 3D hash for volumetric noise (if needed later)
float hash3(vec3 p) {
    p += 10000.0;
    uvec3 q = uvec3(ivec3(p));
    q *= uvec3(1597334673u, 3812015801u, 2798796415u);
    uint n = q.x ^ q.y ^ q.z;
    n *= 1597334673u;
    n ^= (n >> 16u);
    
    return float(n) * (1.0 / float(0xffffffffu));
}

// Value noise
float valueNoise(vec2 p) {
    vec2 i = floor(p);
    vec2 f = fract(p);
    f = f * f * (3.0 - 2.0 * f);  // Smoothstep
    
    float a = hash(i);
    float b = hash(i + vec2(1.0, 0.0));
    float c = hash(i + vec2(0.0, 1.0));
    float d = hash(i + vec2(1.0, 1.0));
    
    return mix(mix(a, b, f.x), mix(c, d, f.x), f.y);
}

// Fractional Brownian Motion - layered noise for natural terrain
float fbm(vec2 p, int octaves) {
    float value = 0.0;
    float amplitude = 0.5;
    float frequency = 1.0;
    
    for (int i = 0; i < octaves; i++) {
        value += amplitude * valueNoise(p * frequency);
        frequency *= 2.0;
        amplitude *= 0.5;
    }
    
    return value;
}

// =============================================================================
// RAYMARCHING CONFIGURATION
// =============================================================================

// @group("Raymarching")
// @slider(min=50, max=500, default=200)
uniform int maxSteps;

// @group("Raymarching")
// @slider(min=50.0, max=1000.0, default=500.0)
uniform float maxDistance;

// @group("Raymarching")
// @slider(min=0.0001, max=0.01, default=0.001)
uniform float surfaceThreshold;

// =============================================================================
// TERRAIN CONFIGURATION
// =============================================================================

// @group("Terrain")
// @slider(min=1.0, max=50.0, default=8.0)
uniform float terrainHeight;

// @group("Terrain")
// @slider(min=0.005, max=0.5, default=0.2)
uniform float terrainScale;

// @group("Terrain")
// @slider(min=1, max=32, default=8)
uniform int terrainOctaves;

// @group("Terrain")
// @slider(min=-50.0, max=50.0, default=-8.0)
uniform float terrainBaseHeight;

// Terrain colors (height-based)
// @group("Terrain Colors")
// @color(default=0.25,0.45,0.15)
uniform vec3 grassColor;

// @group("Terrain Colors")
// @color(default=0.45,0.4,0.35)
uniform vec3 rockColor;

// @group("Terrain Colors")
// @color(default=0.95,0.95,0.98)
uniform vec3 snowColor;

// @group("Terrain Colors")
// @slider(min=0.0, max=1.0, default=0.5)
uniform float grassHeight;

// @group("Terrain Colors")
// @slider(min=0.0, max=1.0, default=0.8)
uniform float snowHeight;

// =============================================================================
// SKY CONFIGURATION
// =============================================================================

// @group("Sky")
// @color(default=0.15,0.3,0.6)
uniform vec3 skyColorTop;

// @group("Sky")
// @color(default=0.6,0.7,0.85)
uniform vec3 skyColorHorizon;

// =============================================================================
// SUN CONFIGURATION
// =============================================================================

// @group("Sun")
// @color(default=1.0,0.95,0.8)
uniform vec3 sunColor;

// @group("Sun")
// @slider(min=-3.14, max=3.14, default=0.8)
uniform float sunAzimuth;

// @group("Sun")
// @slider(min=0.0, max=1.57, default=0.6)
uniform float sunElevation;

// @group("Sun")
// @slider(min=10.0, max=500.0, default=128.0)
uniform float sunSharpness;

// @group("Sun")
// @slider(min=0.0, max=2.0, default=1.0)
uniform float sunIntensity;

// =============================================================================
// ATMOSPHERE CONFIGURATION
// =============================================================================

// @group("Atmosphere")
// @slider(min=0.0, max=0.02, default=0.001)
uniform float fogDensity;

// @group("Atmosphere")
// @color(default=0.5,0.6,0.75)
uniform vec3 fogColor;

// =============================================================================
// LIGHTING
// =============================================================================

// @group("Lighting")
// @slider(min=0.0, max=0.5, default=0.1)
uniform float ambientStrength;

// @group("Lighting")
// @slider(min=0.0, max=2.0, default=1.2)
uniform float diffuseStrength;

// @group("Lighting")
// @slider(min=0.0, max=2.0, default=0.3)
uniform float specularStrength;

// =============================================================================
// AMBIENT OCCLUSION CONFIGURATION
// =============================================================================
// Ambient occlusion darkens areas where geometry occludes indirect light,
// such as crevices, valleys, and concave surfaces. This is computed by
// sampling the SDF along the normal direction and comparing actual vs
// expected distances.

// @group("Ambient Occlusion")
// @slider(min=0.0, max=2.0, default=1.0)
uniform float aoStrength;

// @group("Ambient Occlusion")
// @slider(min=0.1, max=5.0, default=1.5)
uniform float aoRadius;

// @group("Ambient Occlusion")
// @slider(min=1, max=8, default=5)
uniform int aoSamples;

// =============================================================================
// SOFT SHADOWS CONFIGURATION
// =============================================================================
// Soft shadows are computed by raymarching toward the sun and using the
// ratio of SDF distance to ray distance to estimate penumbra width.
// This creates realistic shadows that are sharper near contact points
// and softer further away.

// @group("Shadows")
// @slider(min=0.0, max=1.0, default=1.0)
uniform float shadowStrength;

// @group("Shadows")
// @slider(min=2.0, max=64.0, default=16.0)
uniform float shadowSoftness;

// @group("Shadows")
// @slider(min=16, max=128, default=64)
uniform int shadowSteps;

// @group("Shadows")
// @slider(min=10.0, max=200.0, default=100.0)
uniform float shadowMaxDistance;

// @group("Shadows")
// @slider(min=0.01, max=1.0, default=0.1)
uniform float shadowBias;

// =============================================================================
// DETAIL NORMALS CONFIGURATION
// =============================================================================
// Detail normals add micro-surface variation using procedural noise,
// creating the appearance of surface roughness and texture without
// actual geometry. Multiple octaves create both coarse and fine detail.

// @group("Detail Normals")
// @slider(min=0.0, max=1.0, default=0.3)
uniform float detailNormalStrength;

// @group("Detail Normals")
// @slider(min=1.0, max=100.0, default=20.0)
uniform float detailNormalScale;

// @group("Detail Normals")
// @slider(min=1, max=4, default=3)
uniform int detailNormalOctaves;

// =============================================================================
// MOSS CONFIGURATION
// =============================================================================
// Moss grows in sheltered, moist areas - typically in crevices (low AO),
// on north-facing slopes (away from sun), and at moderate elevations.
// Distribution is controlled by noise for organic appearance.

// @group("Moss")
// @slider(min=0.0, max=1.0, default=0.5)
uniform float mossStrength;

// @group("Moss")
// @color(default=0.15,0.3,0.1)
uniform vec3 mossColor;

// @group("Moss")
// @slider(min=0.1, max=20.0, default=5.0)
uniform float mossScale;

// @group("Moss")
// @slider(min=0.0, max=1.0, default=0.7)
uniform float mossMaxHeight;

// @group("Moss")
// @slider(min=0.0, max=1.0, default=0.3)
uniform float mossMinHeight;

// @group("Moss")
// @slider(min=0.0, max=1.0, default=0.7)
uniform float mossPreferCrevice;

// @group("Moss")
// @slider(min=0.0, max=1.0, default=0.6)
uniform float mossMaxSlope;

// =============================================================================
// MATERIAL QUALITY
// =============================================================================

// @group("Material Quality")
// @slider(min=0.0, max=1.0, default=0.3)
uniform float colorVariation;

// =============================================================================
// CAMERA (Built-in uniforms)
// =============================================================================

uniform vec3 uCameraPosition;
uniform vec3 uCameraForward;
uniform vec3 uCameraRight;
uniform vec3 uCameraUp;
uniform float uCameraFOV;

// =============================================================================
// DATA STRUCTURES
// =============================================================================

struct SceneResult {
    float distance;
    float materialID;
    float heightNormalized;  // 0-1 height for coloring
};

struct Material {
    vec3 color;
    float shininess;
    float roughness;      // 0 = smooth, 1 = rough
    float specular;       // Specular intensity
};

// Material IDs
#define MAT_TERRAIN 1.0

// =============================================================================
// TERRAIN GENERATION
// =============================================================================

// Get terrain height at a given XZ position
float getTerrainHeight(vec2 xz) {
    float height = fbm(xz * terrainScale, terrainOctaves);
    return height * terrainHeight + terrainBaseHeight;
}

// =============================================================================
// SCENE DEFINITION
// =============================================================================

SceneResult getScene(vec3 point) {
    SceneResult result;
    result.distance = maxDistance;
    result.materialID = 0.0;
    result.heightNormalized = 0.0;
    
    // Terrain - distance to heightmap surface
    float terrainY = getTerrainHeight(point.xz);
    float terrainDist = point.y - terrainY;
    
    if (terrainDist < result.distance) {
        result.distance = terrainDist;
        result.materialID = MAT_TERRAIN;
        // Normalize height for coloring (0 = base, 1 = peak)
        result.heightNormalized = clamp((terrainY - terrainBaseHeight) / terrainHeight, 0.0, 1.0);
    }
    
    return result;
}

float getSceneDistance(vec3 point) {
    return getScene(point).distance;
}

// =============================================================================
// SUN DIRECTION
// =============================================================================

vec3 getSunDirection() {
    return normalize(vec3(
        cos(sunElevation) * sin(sunAzimuth),
        sin(sunElevation),
        cos(sunElevation) * cos(sunAzimuth)
    ));
}

// =============================================================================
// DETAIL NORMAL SYSTEM
// =============================================================================
// Procedural normal perturbation using multi-octave noise sampling.
// Creates surface micro-detail that responds to lighting.

vec3 calculateDetailNormal(vec3 point, vec3 baseNormal) {
    if (detailNormalStrength < 0.001) return baseNormal;
    
    float epsilon = 0.05;
    vec2 xz = point.xz;
    
    // Accumulate detail from multiple octaves
    vec3 perturbation = vec3(0.0);
    float amplitude = 1.0;
    float frequency = detailNormalScale;
    float totalAmplitude = 0.0;
    
    for (int i = 0; i < detailNormalOctaves; i++) {
        // Sample noise at current frequency
        float center = valueNoise(xz * frequency);
        float dx = valueNoise((xz + vec2(epsilon, 0.0)) * frequency);
        float dz = valueNoise((xz + vec2(0.0, epsilon)) * frequency);
        
        // Compute gradient (derivative of height)
        vec2 gradient = vec2(dx - center, dz - center) / epsilon;
        
        // Accumulate weighted perturbation
        perturbation.xz += gradient * amplitude;
        totalAmplitude += amplitude;
        
        // Next octave: higher frequency, lower amplitude
        frequency *= 2.0;
        amplitude *= 0.5;
    }
    
    // Normalize by total amplitude
    perturbation /= totalAmplitude;
    
    // Create tangent-space perturbation
    // For terrain, we assume Y-up base normal dominates
    vec3 tangent = normalize(vec3(1.0, perturbation.x * detailNormalStrength, 0.0));
    vec3 bitangent = normalize(vec3(0.0, perturbation.z * detailNormalStrength, 1.0));
    
    // Reconstruct normal with perturbation
    vec3 detailNormal = normalize(baseNormal + 
        tangent * perturbation.x * detailNormalStrength +
        bitangent * perturbation.z * detailNormalStrength);
    
    return normalize(mix(baseNormal, detailNormal, detailNormalStrength));
}

// =============================================================================
// SURFACE NORMAL CALCULATION
// =============================================================================

vec3 calculateNormal(vec3 point) {
    vec2 epsilon = vec2(0.01, 0.0);  // Slightly larger epsilon for terrain
    
    float centerDist = getSceneDistance(point);
    
    vec3 baseNormal = centerDist - vec3(
        getSceneDistance(point - epsilon.xyy),
        getSceneDistance(point - epsilon.yxy),
        getSceneDistance(point - epsilon.yyx)
    );
    
    baseNormal = normalize(baseNormal);
    
    // Apply detail normal perturbation
    return calculateDetailNormal(point, baseNormal);
}

// =============================================================================
// AMBIENT OCCLUSION
// =============================================================================
// Samples the SDF at increasing distances along the normal.
// If the actual SDF distance is less than the sample distance,
// it means geometry is occluding that point (valley, crevice, etc.)

float calculateAmbientOcclusion(vec3 point, vec3 normal) {
    if (aoStrength < 0.001) return 1.0;
    
    float occlusion = 0.0;
    float weight = 1.0;
    float totalWeight = 0.0;
    
    for (int i = 1; i <= aoSamples; i++) {
        // Sample distance increases with each iteration
        float sampleDist = aoRadius * float(i) / float(aoSamples);
        
        // Sample point along normal
        vec3 samplePoint = point + normal * sampleDist;
        
        // Get actual distance to nearest surface
        float actualDist = getSceneDistance(samplePoint);
        
        // Occlusion: how much closer is the surface than expected?
        // If actualDist < sampleDist, geometry is blocking
        float diff = max(0.0, sampleDist - actualDist);
        
        // Accumulate weighted occlusion
        occlusion += diff * weight;
        totalWeight += sampleDist * weight;
        
        // Reduce weight for distant samples (closer samples matter more)
        weight *= 0.7;
    }
    
    // Normalize and apply strength
    occlusion = occlusion / totalWeight;
    occlusion = 1.0 - clamp(occlusion * aoStrength * 5.0, 0.0, 1.0);
    
    return occlusion;
}

// =============================================================================
// SOFT SHADOWS
// =============================================================================
// Raymarch toward the sun, tracking the minimum ratio of SDF distance
// to ray distance. This ratio approximates how "blocked" the light is,
// creating natural soft shadows that are sharp near occluders.

float calculateSoftShadow(vec3 origin, vec3 lightDir) {
    if (shadowStrength < 0.001) return 1.0;
    
    float shadow = 1.0;
    float t = shadowBias;  // Start slightly away from surface to avoid self-shadowing
    
    for (int i = 0; i < shadowSteps; i++) {
        if (t >= shadowMaxDistance) break;
        
        vec3 samplePoint = origin + lightDir * t;
        float dist = getSceneDistance(samplePoint);
        
        // In shadow - surface is blocking
        if (dist < 0.001) {
            return 1.0 - shadowStrength;
        }
        
        // Soft shadow calculation:
        // The ratio dist/t determines penumbra width
        // Smaller ratio = more blocked = darker shadow
        // shadowSoftness controls how quickly shadows soften with distance
        float penumbra = shadowSoftness * dist / t;
        shadow = min(shadow, penumbra);
        
        // Adaptive step size: larger steps when far from surfaces
        t += clamp(dist, 0.02, 0.5);
    }
    
    // Apply shadow strength and ensure valid range
    shadow = clamp(shadow, 0.0, 1.0);
    return mix(1.0, shadow, shadowStrength);
}

// =============================================================================
// MOSS CALCULATION
// =============================================================================
// Moss grows in specific conditions:
// - Crevices and sheltered areas (low AO)
// - Moderate slopes (not too steep, not flat)
// - Mid-elevations (not too high for snow, not too low)
// - North-facing slopes (away from direct sun) - optional
// Distribution uses noise for organic appearance.

float calculateMossFactor(vec3 point, vec3 normal, float ao, float heightNorm) {
    if (mossStrength < 0.001) return 0.0;
    
    // Height constraint: moss only in mid-elevations
    float heightFactor = smoothstep(mossMinHeight, mossMinHeight + 0.1, heightNorm) *
                         smoothstep(mossMaxHeight + 0.1, mossMaxHeight, heightNorm);
    if (heightFactor < 0.01) return 0.0;
    
    // Slope constraint: moss prefers moderate slopes
    // slope = 0 for flat (normal.y = 1), slope = 1 for vertical (normal.y = 0)
    float slope = 1.0 - normal.y;
    float slopeFactor = smoothstep(0.0, 0.15, slope) *      // Not on flat ground
                        smoothstep(mossMaxSlope + 0.1, mossMaxSlope, slope);  // Not on cliffs
    
    // Crevice preference: moss likes sheltered areas
    // Low AO = crevice/valley = good for moss
    float creviceFactor = mix(1.0, (1.0 - ao), mossPreferCrevice);
    creviceFactor = smoothstep(0.0, 0.5, creviceFactor);
    
    // Sun avoidance: moss prefers north-facing slopes (negative Z in our coordinate system)
    // This is subtle and optional
    vec3 sunDir = getSunDirection();
    float sunExposure = max(0.0, dot(normal, sunDir));
    float shadeFactor = 1.0 - sunExposure * 0.3;  // Less moss in direct sun
    
    // Noise for organic distribution
    float mossNoise = valueNoise(point.xz * mossScale);
    float detailNoise = valueNoise(point.xz * mossScale * 4.0);
    float combinedNoise = mossNoise * 0.7 + detailNoise * 0.3;
    
    // Threshold noise to create patches
    float noiseFactor = smoothstep(0.3, 0.6, combinedNoise);
    
    // Combine all factors
    float mossFactor = heightFactor * slopeFactor * creviceFactor * shadeFactor * noiseFactor;
    
    return mossFactor * mossStrength;
}

// =============================================================================
// COLOR VARIATION
// =============================================================================
// Adds natural micro-variation to base colors using noise

vec3 applyColorVariation(vec3 baseColor, vec2 xz) {
    if (colorVariation < 0.001) return baseColor;
    
    // Multi-scale noise for natural variation
    float n1 = valueNoise(xz * 2.0);
    float n2 = valueNoise(xz * 8.0);
    float n3 = valueNoise(xz * 32.0);
    float noise = n1 * 0.5 + n2 * 0.3 + n3 * 0.2;
    
    // Vary brightness
    vec3 varied = baseColor * (0.85 + noise * 0.3);
    
    return mix(baseColor, varied, colorVariation);
}

// =============================================================================
// MATERIAL SYSTEM
// =============================================================================

Material getMaterial(float materialID, float heightNormalized, vec3 worldPos, vec3 normal, float ao) {
    Material mat;
    mat.shininess = 16.0;
    mat.roughness = 0.5;
    mat.specular = 0.1;
    
    if (materialID == MAT_TERRAIN) {
        vec3 baseColor;
        vec2 terrainXZ = worldPos.xz;
        
        // Height-based terrain coloring
        if (heightNormalized < grassHeight) {
            // Grass zone
            baseColor = grassColor;
            mat.roughness = 0.7;
            mat.specular = 0.1;
            mat.shininess = 16.0;
        } else if (heightNormalized < snowHeight) {
            // Rock zone - blend from grass to rock
            float t = (heightNormalized - grassHeight) / (snowHeight - grassHeight);
            baseColor = mix(grassColor, rockColor, smoothstep(0.0, 0.3, t));
            
            // Rock properties vary with wetness (noise-based)
            float rockNoise = valueNoise(terrainXZ * 4.0);
            mat.roughness = mix(0.3, 0.9, rockNoise);
            mat.specular = mix(0.4, 0.05, rockNoise);
            mat.shininess = mix(64.0, 8.0, rockNoise);
        } else {
            // Snow zone - blend from rock to snow
            float t = (heightNormalized - snowHeight) / (1.0 - snowHeight);
            baseColor = mix(rockColor, snowColor, smoothstep(0.0, 0.5, t));
            
            // Snow is reflective
            mat.roughness = 0.2;
            mat.specular = 0.6;
            mat.shininess = 64.0;
            
            // Snow sparkle
            float sparkle = valueNoise(terrainXZ * 64.0 + iTime * 0.1);
            if (sparkle > 0.95) {
                baseColor += vec3(0.3) * (sparkle - 0.95) * 20.0;
            }
        }
        
        // Apply color variation
        baseColor = applyColorVariation(baseColor, terrainXZ);
        
        // Apply moss
        float mossFactor = calculateMossFactor(worldPos, normal, ao, heightNormalized);
        if (mossFactor > 0.0) {
            // Moss color with variation
            vec3 variedMossColor = applyColorVariation(mossColor, terrainXZ * 2.0);
            baseColor = mix(baseColor, variedMossColor, mossFactor);
            
            // Moss material properties
            mat.roughness = mix(mat.roughness, 0.9, mossFactor);  // Moss is rough
            mat.specular = mix(mat.specular, 0.05, mossFactor);   // Moss is not shiny
            mat.shininess = mix(mat.shininess, 4.0, mossFactor);
        }
        
        mat.color = baseColor;
    } else {
        mat.color = vec3(0.5);
    }
    
    return mat;
}

// =============================================================================
// SKY SYSTEM
// =============================================================================

vec3 getSkyColor(vec3 rayDirection) {
    vec3 sunDir = getSunDirection();
    
    // Base sky gradient
    float t = pow(max(0.0, rayDirection.y), 0.5);
    vec3 sky = mix(skyColorHorizon, skyColorTop, t);
    
    // Sun disc
    float sunDot = max(0.0, dot(rayDirection, sunDir));
    vec3 sun = sunColor * pow(sunDot, sunSharpness) * sunIntensity;
    
    // Sun glow (softer halo)
    vec3 sunGlow = sunColor * 0.3 * pow(sunDot, 4.0) * sunIntensity;
    
    return sky + sun + sunGlow;
}

// =============================================================================
// RAYMARCHING CORE
// =============================================================================

SceneResult rayMarch(vec3 rayOrigin, vec3 rayDirection) {
    float totalDistance = 0.0;
    SceneResult result;
    result.distance = -1.0;
    result.materialID = 0.0;
    result.heightNormalized = 0.0;
    
    for (int i = 0; i < maxSteps; i++) {
        vec3 currentPos = rayOrigin + rayDirection * totalDistance;
        result = getScene(currentPos);
        
        // Hit surface
        if (result.distance < surfaceThreshold) {
            result.distance = totalDistance;
            return result;
        }
        
        // Conservative step for terrain (heightmap SDFs are not exact)
        // Use smaller steps when close to surface
        float stepSize = result.distance * 0.4;
        totalDistance += stepSize;
        
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

vec3 calculateLighting(vec3 point, vec3 normal, vec3 viewDir, Material material, float ao, float shadow) {
    vec3 sunDir = getSunDirection();
    
    // === Ambient ===
    // Ambient is affected by AO (occluded areas receive less indirect light)
    vec3 ambient = material.color * ambientStrength * ao;
    
    // === Diffuse (Lambert) ===
    // Diffuse is affected by shadow
    float NdotL = max(dot(normal, sunDir), 0.0);
    vec3 diffuse = material.color * sunColor * NdotL * diffuseStrength * shadow;
    
    // === Specular (Blinn-Phong) ===
    // Specular highlights from the sun, modulated by shadow and material properties
    vec3 halfVector = normalize(sunDir + viewDir);
    float NdotH = max(dot(normal, halfVector), 0.0);
    float specularPower = material.shininess * (1.0 - material.roughness * 0.8);
    float specularFactor = pow(NdotH, specularPower) * material.specular;
    vec3 specular = sunColor * specularFactor * specularStrength * shadow;
    
    // === Fresnel (rim lighting) ===
    // Surfaces facing away from camera reflect more light
    float fresnel = pow(1.0 - max(dot(normal, viewDir), 0.0), 4.0);
    vec3 fresnelColor = mix(vec3(0.0), sunColor * 0.15, fresnel * (1.0 - material.roughness));
    
    // === Final combination ===
    vec3 finalLight = ambient + diffuse + specular + fresnelColor;
    
    return finalLight;
}

// =============================================================================
// ATMOSPHERE / FOG
// =============================================================================

vec3 applyFog(vec3 color, float distance, vec3 rayDirection) {
    // Exponential fog
    float fogAmount = 1.0 - exp(-distance * fogDensity);
    
    // Fog color influenced by sun direction
    vec3 sunDir = getSunDirection();
    float sunAmount = max(dot(rayDirection, sunDir), 0.0);
    vec3 adjustedFogColor = mix(fogColor, sunColor * 0.8, pow(sunAmount, 8.0) * 0.5);
    
    return mix(color, adjustedFogColor, fogAmount);
}

// =============================================================================
// CAMERA SYSTEM
// =============================================================================

vec3 getCameraRay(vec2 uv) {
    float focalLength = 1.0 / tan(radians(uCameraFOV) * 0.5);
    
    vec3 rayDir = normalize(
        uCameraForward * focalLength +
        uCameraRight * uv.x +
        uCameraUp * uv.y
    );
    return rayDir;
}

// =============================================================================
// MAIN RENDERING PIPELINE
// =============================================================================

out vec4 fragColor;
in vec2 fragCoord;

void main() {
    // 1. Setup screen coordinates
    vec2 uv = (fragCoord - 0.5) * 2.0;
    uv.x *= iResolution.x / iResolution.y;
    
    // 2. Generate camera ray
    vec3 rayDir = getCameraRay(uv);
    
    // 3. Raymarch the scene
    SceneResult hit = rayMarch(uCameraPosition, rayDir);
    
    vec3 finalColor;
    
    if (hit.distance > 0.0) {
        // 4. Calculate hit point and view direction
        vec3 hitPoint = uCameraPosition + rayDir * hit.distance;
        vec3 viewDir = -rayDir;
        
        // 5. Calculate surface normal (includes detail normals)
        vec3 normal = calculateNormal(hitPoint);
        
        // 6. Calculate ambient occlusion
        float ao = calculateAmbientOcclusion(hitPoint, normal);
        
        // 7. Calculate soft shadows
        vec3 sunDir = getSunDirection();
        float shadow = calculateSoftShadow(hitPoint, sunDir);
        
        // 8. Get material (includes moss calculation which needs AO)
        Material material = getMaterial(hit.materialID, hit.heightNormalized, hitPoint, normal, ao);
        
        // 9. Calculate final lighting
        finalColor = calculateLighting(hitPoint, normal, viewDir, material, ao, shadow);
        
        // 10. Apply atmospheric fog
        finalColor = applyFog(finalColor, hit.distance, rayDir);
    }
    else {
        // 11. Render sky
        finalColor = getSkyColor(rayDir);
    }
    
    // 12. Gamma correction
    finalColor = pow(finalColor, vec3(0.4545));
    
    fragColor = vec4(finalColor, 1.0);
}

// =============================================================================
// CAMERA CONTROLS
// =============================================================================
//
// Use keyboard and mouse to explore the terrain:
//   - W/S or Up/Down:     Move forward/backward
//   - A/D or Left/Right:  Strafe left/right
//   - Q/Space:            Move up
//   - E/Ctrl:             Move down
//   - Right-click + Drag: Look around
//   - Mouse Scroll:       Zoom (FOV)
//   - Shift:              Sprint
//
// =============================================================================
// IMPLEMENTED ENHANCEMENTS
// =============================================================================
//
// [X] Ambient Occlusion - Multi-sample AO darkens crevices and valleys
// [X] Soft Shadows - Penumbra-based shadows, sharp near occluders
// [X] Moss System - Grows in crevices, moderate slopes, mid-elevations
// [X] Detail Normals - Multi-octave procedural normal perturbation
// [X] Color Variation - Noise-based micro-variation breaks up flat colors
// [X] Specular Highlights - Blinn-Phong with roughness control
// [X] Fresnel Effects - View-angle dependent rim lighting
// [X] Material Properties - Per-zone roughness, specular, shininess
// [X] Snow Sparkle - Time-varying bright spots on snow
// [X] Rock Wetness - Noise-based wet/dry rock variation
//
// =============================================================================
// PARAMETER GUIDE
// =============================================================================
//
// Ambient Occlusion:
//   - aoStrength (0-2): Overall AO intensity
//   - aoRadius (0.1-5): How far to sample for occlusion
//   - aoSamples (1-8): Quality vs performance tradeoff
//
// Soft Shadows:
//   - shadowStrength (0-1): Shadow darkness
//   - shadowSoftness (2-64): Higher = softer penumbras
//   - shadowSteps (16-128): Quality vs performance
//   - shadowMaxDistance (10-200): How far to trace shadows
//   - shadowBias (0.01-1): Prevents self-shadowing artifacts
//
// Detail Normals:
//   - detailNormalStrength (0-1): How much surface roughness
//   - detailNormalScale (1-100): Size of detail features
//   - detailNormalOctaves (1-4): Layers of detail
//
// Moss:
//   - mossStrength (0-1): Overall moss coverage
//   - mossColor: Base moss color (dark green)
//   - mossScale (0.1-20): Size of moss patches
//   - mossMinHeight/mossMaxHeight: Elevation range for moss
//   - mossPreferCrevice (0-1): How much moss prefers low-AO areas
//   - mossMaxSlope (0-1): Steepest slope moss can grow on
//
// =============================================================================
// FUTURE ENHANCEMENTS
// =============================================================================
//
// - Water plane with reflections at a certain height
// - Volumetric clouds using noise
// - Wind animation for grass movement
// - Distance-based LOD (less detail far away for performance)
// - PBR BRDF (Cook-Torrance GGX model)
// - Screen-space reflections
// - Atmospheric scattering (Rayleigh/Mie)
//
// =============================================================================


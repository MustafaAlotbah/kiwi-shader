#version 330 core

// =============================================================================
// Procedural City - Raymarched Urban Landscape
// =============================================================================
// A raymarched procedural city demonstrating:
//   - Grid-based building generation with varied heights
//   - Downtown core with taller skyscrapers
//   - Day/night cycle with atmospheric lighting
//   - Procedural lit windows at night
//   - Street lights with glow effects
//   - Soft shadows and ambient occlusion
//   - Fog/atmosphere for depth
//
// Architecture:
//   1. Grid System       - City block layout with streets
//   2. Building System   - Procedural heights and styles
//   3. Window System     - Lit/unlit randomization
//   4. Street System     - Roads with lane markings
//   5. Lighting System   - Day/night with street lights
//   6. Atmosphere        - Fog and sky gradients
//   7. Camera System     - Interactive free camera
// =============================================================================

uniform float iTime;
uniform vec3 iResolution;

// =============================================================================
// NOISE & HASH FUNCTIONS
// =============================================================================

// High-quality 2D hash
float hash(vec2 p) {
    p += 10000.0;
    uvec2 q = uvec2(ivec2(p));
    q *= uvec2(1597334673u, 3812015801u);
    uint n = q.x ^ q.y;
    n *= 1597334673u;
    n ^= (n >> 16u);
    n *= 2246822519u;
    n ^= (n >> 13u);
    return float(n) * (1.0 / float(0xffffffffu));
}

// 3D hash for window randomization
float hash3(vec3 p) {
    p += 10000.0;
    uvec3 q = uvec3(ivec3(p));
    q *= uvec3(1597334673u, 3812015801u, 2798796415u);
    uint n = q.x ^ q.y ^ q.z;
    n *= 1597334673u;
    n ^= (n >> 16u);
    return float(n) * (1.0 / float(0xffffffffu));
}

// 2D value noise
float valueNoise(vec2 p) {
    vec2 i = floor(p);
    vec2 f = fract(p);
    f = f * f * (3.0 - 2.0 * f);
    
    float a = hash(i);
    float b = hash(i + vec2(1.0, 0.0));
    float c = hash(i + vec2(0.0, 1.0));
    float d = hash(i + vec2(1.0, 1.0));
    
    return mix(mix(a, b, f.x), mix(c, d, f.x), f.y);
}

// =============================================================================
// RAYMARCHING CONFIGURATION
// =============================================================================

// @group("Raymarching")
// @slider(min=50, max=1000, default=150)
uniform int maxSteps;

// @group("Raymarching")
// @slider(min=100.0, max=1000.0, default=500.0)
uniform float maxDistance;

// @group("Raymarching")
// @slider(min=0.0001, max=0.01, default=0.001)
uniform float surfaceThreshold;

// =============================================================================
// CITY LAYOUT CONFIGURATION
// =============================================================================

// @group("City Layout")
// @slider(min=5.0, max=30.0, default=12.0)
uniform float blockSize;

// @group("City Layout")
// @slider(min=1.0, max=8.0, default=4.0)
uniform float streetWidth;

// @group("City Layout")
// @slider(min=1.0, max=50.0, default=22.0)
uniform float maxBuildingHeight;

// @group("City Layout")
// @slider(min=0.0, max=30.0, default=8.0)
uniform float minBuildingHeight;

// @group("City Layout")
// @slider(min=0.0, max=1.0, default=0.7)
uniform float buildingDensity;

// @group("City Layout")
// @slider(min=0.0, max=200.0, default=80.0)
uniform float downtownRadius;

// @group("City Layout")
// @slider(min=1.0, max=3.0, default=1.8)
uniform float downtownHeightMultiplier;

// =============================================================================
// BUILDING APPEARANCE
// =============================================================================

// @group("Buildings")
// @color(default=0.4,0.42,0.45)
uniform vec3 buildingColorBase;

// @group("Buildings")
// @color(default=0.3,0.32,0.35)
uniform vec3 buildingColorDark;

// @group("Buildings")
// @slider(min=0.0, max=1.0, default=0.3)
uniform float buildingColorVariation;

// @group("Buildings")
// @slider(min=0.5, max=3.0, default=1.2)
uniform float windowSpacingH;

// @group("Buildings")
// @slider(min=0.5, max=3.0, default=1.0)
uniform float windowSpacingV;

// @group("Buildings")
// @slider(min=0.1, max=0.9, default=0.6)
uniform float windowSize;

// =============================================================================
// DAY/NIGHT CYCLE
// =============================================================================

// 0.0 = midnight, 0.25 = sunrise, 0.5 = noon, 0.75 = sunset
// @group("Time of Day")
// @slider(min=0.0, max=1.0, default=0.3)
uniform float timeOfDay;

// @group("Time of Day")
// @slider(min=0.0, max=1.0, default=0.0)
// Enable to animate time automatically
uniform float animateTime;

// @group("Time of Day")
// @slider(min=0.01, max=0.5, default=0.05)
uniform float timeSpeed;

// =============================================================================
// NIGHT LIGHTING
// =============================================================================

// @group("Night Lighting")
// @slider(min=0.0, max=1.0, default=0.4)
uniform float windowLitProbability;

// @group("Night Lighting")
// @color(default=1.0,0.9,0.7)
uniform vec3 windowLightColor;

// @group("Night Lighting")
// @slider(min=0.0, max=5.0, default=2.0)
uniform float windowGlowIntensity;

// @group("Night Lighting")
// @color(default=1.0,0.85,0.6)
uniform vec3 streetLightColor;

// @group("Night Lighting")
// @slider(min=0.0, max=25.0, default=10.0)
uniform float streetLightIntensity;

// @group("Night Lighting")
// @slider(min=1.0, max=20.0, default=1.0)
uniform float streetLightRadius;

// @group("Night Lighting")
// @slider(min=2.0, max=6.0, default=4.5)
uniform float streetLightHeight;

// @group("Night Lighting")
// @slider(min=0.0, max=2.0, default=0.5)
uniform float lightGlowSize;

// @group("Night Lighting")
// @slider(min=0.0, max=3.0, default=1.5)
uniform float windowBloomStrength;

// @group("Night Lighting")
// @slider(min=0.0, max=1.0, default=0.05)
uniform float atmosphericScatter;

// =============================================================================
// STREET APPEARANCE
// =============================================================================

// @group("Streets")
// @color(default=0.15,0.15,0.17)
uniform vec3 streetColor;

// @group("Streets")
// @color(default=0.25,0.25,0.25)
uniform vec3 sidewalkColor;

// @group("Streets")
// @slider(min=0.0, max=2.0, default=1.2)
uniform float sidewalkWidth;

// =============================================================================
// SKY CONFIGURATION
// =============================================================================

// @group("Sky")
// @color(default=0.4,0.6,0.9)
uniform vec3 daySkyTop;

// @group("Sky")
// @color(default=0.7,0.8,0.95)
uniform vec3 daySkyHorizon;

// @group("Sky")
// @color(default=0.02,0.02,0.05)
uniform vec3 nightSkyTop;

// @group("Sky")
// @color(default=0.05,0.05,0.1)
uniform vec3 nightSkyHorizon;

// @group("Sky")
// @color(default=1.0,0.6,0.3)
uniform vec3 sunsetColor;

// =============================================================================
// MOON CONFIGURATION
// =============================================================================

// @group("Moon")
// @slider(min=0.02, max=0.25, default=0.1)
uniform float moonSize;

// @group("Moon")
// @slider(min=0.0, max=3.0, default=2.0)
uniform float moonBrightness;

// @group("Moon")
// @color(default=0.9,0.92,1.0)
uniform vec3 moonColor;

// @group("Moon")
// @slider(min=0.0, max=1.0, default=0.15)
uniform float moonGlowSize;

// =============================================================================
// AIRPLANE LIGHTS CONFIGURATION
// =============================================================================

// @group("Airplanes")
// @slider(min=0, max=8, default=3)
uniform int airplaneCount;

// @group("Airplanes")
// @slider(min=0.1, max=2.0, default=0.5)
uniform float airplaneSpeed;

// @group("Airplanes")
// @slider(min=0.5, max=3.0, default=1.5)
uniform float airplaneLightSize;

// @group("Airplanes")
// @slider(min=50.0, max=300.0, default=150.0)
uniform float airplaneAltitude;

// =============================================================================
// ATMOSPHERE
// =============================================================================

// @group("Atmosphere")
// @slider(min=0.0, max=0.02, default=0.003)
uniform float fogDensity;

// @group("Atmosphere")
// @slider(min=0.0, max=1.0, default=0.3)
uniform float fogHeightFalloff;

// =============================================================================
// SHADOWS & AO
// =============================================================================

// @group("Shadows")
// @slider(min=0.0, max=1.0, default=0.8)
uniform float shadowStrength;

// @group("Shadows")
// @slider(min=2.0, max=32.0, default=12.0)
uniform float shadowSoftness;

// @group("Shadows")
// @slider(min=16, max=64, default=32)
uniform int shadowSteps;

// @group("Ambient Occlusion")
// @slider(min=0.0, max=2.0, default=0.8)
uniform float aoStrength;

// @group("Ambient Occlusion")
// @slider(min=0.5, max=5.0, default=2.0)
uniform float aoRadius;

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
    int materialID;
    vec3 cellId;      // Building cell identifier
    vec2 windowCoord; // For window calculations
};

// Material IDs
#define MAT_GROUND 0
#define MAT_STREET 1
#define MAT_SIDEWALK 2
#define MAT_BUILDING 3
#define MAT_ROOF 4
#define MAT_LAMPPOST 5
#define MAT_LAMP 6

// =============================================================================
// TIME CALCULATION
// =============================================================================

float getTimeOfDay() {
    float t = timeOfDay;
    if (animateTime > 0.5) {
        t = fract(iTime * timeSpeed);
    }
    return t;
}

// Sun/moon position based on time
vec3 getSunDirection() {
    float t = getTimeOfDay();
    // Sun arc: rises in east (0.25), peaks at noon (0.5), sets in west (0.75)
    float angle = (t - 0.25) * 3.14159 * 2.0;
    return normalize(vec3(
        cos(angle) * 0.5,
        sin(angle),
        -0.5
    ));
}

// Day/night blend factor (0 = night, 1 = day)
float getDayNightFactor() {
    float t = getTimeOfDay();
    // Smooth transition at sunrise (0.2-0.3) and sunset (0.7-0.8)
    float dawn = smoothstep(0.2, 0.35, t);
    float dusk = smoothstep(0.8, 0.65, t);
    return dawn * dusk;
}

// Sunset factor for color grading
float getSunsetFactor() {
    float t = getTimeOfDay();
    float sunrise = smoothstep(0.2, 0.28, t) * smoothstep(0.38, 0.3, t);
    float sunset = smoothstep(0.62, 0.7, t) * smoothstep(0.8, 0.72, t);
    return max(sunrise, sunset);
}

// =============================================================================
// SDF PRIMITIVES
// =============================================================================

float sdBox(vec3 p, vec3 b) {
    vec3 q = abs(p) - b;
    return length(max(q, 0.0)) + min(max(q.x, max(q.y, q.z)), 0.0);
}

float sdPlane(vec3 p, float h) {
    return p.y - h;
}

// Vertical capsule for lamp posts
float sdCapsuleY(vec3 p, float h, float r) {
    p.y -= clamp(p.y, 0.0, h);
    return length(p) - r;
}

// Sphere for lamp heads
float sdSphere(vec3 p, float r) {
    return length(p) - r;
}

// Cylinder for rooftop structures
float sdCylinder(vec3 p, float h, float r) {
    vec2 d = abs(vec2(length(p.xz), p.y)) - vec2(r, h);
    return min(max(d.x, d.y), 0.0) + length(max(d, 0.0));
}

// Rooftop style enum
#define ROOF_FLAT 0
#define ROOF_AC_UNITS 1
#define ROOF_WATER_TANK 2
#define ROOF_PENTHOUSE 3
#define ROOF_ANTENNA 4
#define ROOF_HELIPAD 5

// =============================================================================
// CITY GENERATION
// =============================================================================

// Get building data for a cell
struct BuildingData {
    float height;
    bool exists;
    vec3 color;
    float style;      // 0-1 for different window styles
    int roofType;     // Type of rooftop structure
    float roofParam;  // Random parameter for roof variation
};

BuildingData getBuildingData(vec2 cellId) {
    BuildingData data;
    
    // Hash for this cell
    float h = hash(cellId);
    float h2 = hash(cellId + 100.0);
    float h3 = hash(cellId + 200.0);
    float h4 = hash(cellId + 500.0);
    float h5 = hash(cellId + 600.0);
    
    // Check if building exists (density)
    data.exists = h < buildingDensity;
    
    if (!data.exists) {
        data.height = 0.0;
        data.color = vec3(0.0);
        data.style = 0.0;
        data.roofType = ROOF_FLAT;
        data.roofParam = 0.0;
        return data;
    }
    
    // Base height from noise
    float baseHeight = mix(minBuildingHeight, maxBuildingHeight, h2);
    
    // Downtown multiplier (taller buildings near center)
    float distFromCenter = length(cellId * blockSize);
    float downtownFactor = 1.0 - smoothstep(0.0, downtownRadius, distFromCenter);
    float heightMult = mix(1.0, downtownHeightMultiplier, downtownFactor);
    
    // Add some randomness to downtown effect
    heightMult *= 0.7 + 0.6 * h3;
    
    data.height = baseHeight * heightMult;
    
    // Building color variation
    float colorVar = hash(cellId + 300.0);
    data.color = mix(buildingColorBase, buildingColorDark, colorVar * buildingColorVariation);
    
    // Style (for window patterns, etc.)
    data.style = hash(cellId + 400.0);
    
    // Roof type - weighted random selection
    // Taller buildings more likely to have interesting roofs
    float roofRand = h4;
    float tallBonus = smoothstep(15.0, 30.0, data.height) * 0.3;
    
    if (roofRand < 0.25) {
        data.roofType = ROOF_FLAT;  // 25% flat
    } else if (roofRand < 0.45) {
        data.roofType = ROOF_AC_UNITS;  // 20% AC units
    } else if (roofRand < 0.60) {
        data.roofType = ROOF_WATER_TANK;  // 15% water tank
    } else if (roofRand < 0.75 + tallBonus) {
        data.roofType = ROOF_PENTHOUSE;  // 15-30% penthouse (more on tall buildings)
    } else if (roofRand < 0.90) {
        data.roofType = ROOF_ANTENNA;  // 15% antenna
    } else {
        data.roofType = ROOF_HELIPAD;  // 10% helipad (rare)
    }
    
    data.roofParam = h5;
    
    return data;
}

// Check if position is on a street
bool isStreet(vec2 pos) {
    vec2 blockPos = mod(pos + streetWidth * 0.5, blockSize);
    return blockPos.x < streetWidth || blockPos.y < streetWidth;
}

// Check if position is on sidewalk
bool isSidewalk(vec2 pos) {
    if (isStreet(pos)) return false;
    
    vec2 blockPos = mod(pos + streetWidth * 0.5, blockSize);
    float distToStreetX = min(blockPos.x - streetWidth, blockSize - blockPos.x);
    float distToStreetY = min(blockPos.y - streetWidth, blockSize - blockPos.y);
    float distToStreet = min(distToStreetX, distToStreetY);
    
    return distToStreet < sidewalkWidth;
}

// Get cell ID for a position
vec2 getCellId(vec2 pos) {
    return floor((pos + streetWidth * 0.5) / blockSize);
}

// Get position within cell (0-1)
vec2 getCellPos(vec2 pos) {
    return fract((pos + streetWidth * 0.5) / blockSize);
}

// =============================================================================
// SCENE SDF
// =============================================================================

SceneResult getScene(vec3 p) {
    SceneResult result;
    result.distance = maxDistance;
    result.materialID = MAT_GROUND;
    result.cellId = vec3(0.0);
    result.windowCoord = vec2(0.0);
    
    // Ground plane
    float groundDist = sdPlane(p, 0.0);
    
    // Determine ground material
    if (isStreet(p.xz)) {
        result.materialID = MAT_STREET;
    } else if (isSidewalk(p.xz)) {
        result.materialID = MAT_SIDEWALK;
    }
    
    result.distance = groundDist;
    
    // Street lamps at intersections
    float spacing = blockSize;
    vec2 lampGrid = floor(p.xz / spacing + 0.5) * spacing;
    vec2 lampOffset = vec2(streetWidth * 0.5 + 0.8);
    
    for (int i = 0; i < 4; i++) {
        vec2 corner = lampGrid + lampOffset * vec2(
            (i & 1) == 0 ? -1.0 : 1.0,
            (i & 2) == 0 ? -1.0 : 1.0
        );
        
        // Lamp post (thin cylinder)
        vec3 postPos = p - vec3(corner.x, 0.0, corner.y);
        float postDist = sdCapsuleY(postPos, streetLightHeight - 0.3, 0.08);
        
        if (postDist < result.distance) {
            result.distance = postDist;
            result.materialID = MAT_LAMPPOST;
        }
        
        // Lamp head (glowing sphere)
        vec3 lampPos = p - vec3(corner.x, streetLightHeight, corner.y);
        float lampDist = sdSphere(lampPos, 0.25);
        
        if (lampDist < result.distance) {
            result.distance = lampDist;
            result.materialID = MAT_LAMP;
            result.cellId = vec3(corner, 0.0);
        }
    }
    
    // Buildings - check nearby cells
    vec2 cellId = getCellId(p.xz);
    
    for (int dx = -1; dx <= 1; dx++) {
        for (int dz = -1; dz <= 1; dz++) {
            vec2 checkCell = cellId + vec2(float(dx), float(dz));
            BuildingData building = getBuildingData(checkCell);
            
            if (!building.exists) continue;
            
            // Building center in world space
            // Center of the usable block area (excluding street)
            vec2 buildingCenter = (checkCell + 0.5) * blockSize;
            
            // Building size (smaller than block to leave room for sidewalk)
            float buildingMargin = streetWidth * 0.5 + sidewalkWidth;
            vec2 buildingSize2D = vec2(blockSize - buildingMargin * 2.0) * 0.5;
            
            // 3D building box (main structure)
            vec3 buildingPos = p - vec3(buildingCenter.x, building.height * 0.5, buildingCenter.y);
            vec3 buildingSize = vec3(buildingSize2D.x, building.height * 0.5, buildingSize2D.y);
            
            float buildingDist = sdBox(buildingPos, buildingSize);
            
            // =========== ROOFTOP STRUCTURES ===========
            float roofDist = maxDistance;
            vec3 roofTop = vec3(buildingCenter.x, building.height, buildingCenter.y);
            vec3 roofLocalPos = p - roofTop;
            
            if (building.roofType == ROOF_AC_UNITS) {
                // Multiple AC unit boxes on roof
                float acSize = min(buildingSize2D.x, buildingSize2D.y) * 0.25;
                
                // 2-4 AC units based on roofParam
                int numAC = 2 + int(building.roofParam * 3.0);
                for (int ac = 0; ac < 4; ac++) {
                    if (ac >= numAC) break;
                    
                    // Position AC units in a pattern
                    float acHash = hash(checkCell + float(ac) * 10.0);
                    float acHash2 = hash(checkCell + float(ac) * 20.0);
                    vec2 acOffset = vec2(
                        (acHash - 0.5) * buildingSize2D.x * 1.2,
                        (acHash2 - 0.5) * buildingSize2D.y * 1.2
                    );
                    
                    vec3 acPos = roofLocalPos - vec3(acOffset.x, acSize * 0.5 + 0.1, acOffset.y);
                    float acDist = sdBox(acPos, vec3(acSize, acSize * 0.5, acSize * 0.8));
                    roofDist = min(roofDist, acDist);
                }
            }
            else if (building.roofType == ROOF_WATER_TANK) {
                // Cylindrical water tank
                float tankRadius = min(buildingSize2D.x, buildingSize2D.y) * 0.3;
                float tankHeight = tankRadius * (1.0 + building.roofParam);
                
                // Offset from center
                vec2 tankOffset = vec2(
                    (building.roofParam - 0.5) * buildingSize2D.x * 0.5,
                    (hash(checkCell + 700.0) - 0.5) * buildingSize2D.y * 0.5
                );
                
                vec3 tankPos = roofLocalPos - vec3(tankOffset.x, tankHeight * 0.5, tankOffset.y);
                roofDist = sdCylinder(tankPos, tankHeight * 0.5, tankRadius);
                
                // Support legs
                for (int leg = 0; leg < 4; leg++) {
                    float angle = float(leg) * 1.5708;
                    vec2 legOffset = tankOffset + vec2(cos(angle), sin(angle)) * tankRadius * 0.7;
                    vec3 legPos = roofLocalPos - vec3(legOffset.x, 0.0, legOffset.y);
                    float legDist = sdCapsuleY(legPos, tankHeight * 0.3, 0.1);
                    roofDist = min(roofDist, legDist);
                }
            }
            else if (building.roofType == ROOF_PENTHOUSE) {
                // Smaller box on top (penthouse level)
                float phScale = 0.5 + building.roofParam * 0.3;
                vec3 phSize = vec3(buildingSize2D.x * phScale, building.height * 0.15, buildingSize2D.y * phScale);
                
                vec3 phPos = roofLocalPos - vec3(0.0, phSize.y, 0.0);
                roofDist = sdBox(phPos, phSize);
            }
            else if (building.roofType == ROOF_ANTENNA) {
                // Tall antenna/spire
                float antennaHeight = 2.0 + building.roofParam * 4.0;
                
                // Main antenna mast
                vec3 antennaPos = roofLocalPos;
                roofDist = sdCapsuleY(antennaPos, antennaHeight, 0.05);
                
                // Support structure at base
                float baseSize = 0.5;
                vec3 basePos = roofLocalPos - vec3(0.0, baseSize * 0.5, 0.0);
                float baseDist = sdBox(basePos, vec3(baseSize, baseSize * 0.5, baseSize));
                roofDist = min(roofDist, baseDist);
                
                // Cross bars
                if (building.roofParam > 0.5) {
                    vec3 crossPos = roofLocalPos - vec3(0.0, antennaHeight * 0.6, 0.0);
                    float crossDist = sdBox(crossPos, vec3(0.8, 0.05, 0.05));
                    roofDist = min(roofDist, crossDist);
                }
            }
            else if (building.roofType == ROOF_HELIPAD) {
                // Flat helipad platform slightly raised
                float padSize = min(buildingSize2D.x, buildingSize2D.y) * 0.7;
                vec3 padPos = roofLocalPos - vec3(0.0, 0.15, 0.0);
                roofDist = sdBox(padPos, vec3(padSize, 0.15, padSize));
                
                // Safety railing posts
                for (int post = 0; post < 8; post++) {
                    float angle = float(post) * 0.785398; // 45 degrees
                    vec2 postOffset = vec2(cos(angle), sin(angle)) * padSize * 0.95;
                    vec3 postPos = roofLocalPos - vec3(postOffset.x, 0.5, postOffset.y);
                    float postDist = sdCapsuleY(postPos, 1.0, 0.03);
                    roofDist = min(roofDist, postDist);
                }
            }
            
            // Combine building and roof
            float combinedDist = min(buildingDist, roofDist);
            
            if (combinedDist < result.distance) {
                result.distance = combinedDist;
                result.cellId = vec3(checkCell, building.height);
                
                // Determine material
                if (roofDist < buildingDist) {
                    result.materialID = MAT_ROOF;
                    result.windowCoord = vec2(0.0);
                } else {
                    result.materialID = MAT_BUILDING;
                    
                    // Calculate window coordinate for this point on building surface
                    vec3 localPos = p - vec3(buildingCenter.x, 0.0, buildingCenter.y);
                    
                    // Check if on X-facing wall or Z-facing wall
                    float facadeCoord;
                    if (abs(localPos.x) > abs(localPos.z)) {
                        facadeCoord = localPos.z + buildingSize2D.y;
                    } else {
                        facadeCoord = localPos.x + buildingSize2D.x;
                    }
                    
                    result.windowCoord = vec2(facadeCoord, localPos.y);
                }
            }
        }
    }
    
    return result;
}

float getSceneDistance(vec3 p) {
    return getScene(p).distance;
}

// =============================================================================
// WINDOW SYSTEM
// =============================================================================

// Check if we're inside a window area (for daytime glass appearance)
// Returns: 0.0 = wall, 1.0 = window glass
float getWindowMask(vec2 windowCoord) {
    // Window grid
    vec2 windowGrid = vec2(windowSpacingH, windowSpacingV);
    vec2 windowPos = fract(windowCoord / windowGrid);
    
    // Check if above ground floor
    if (windowCoord.y < windowSpacingV * 0.8) return 0.0;
    
    // Window shape
    vec2 centered = windowPos - 0.5;
    float halfSize = windowSize * 0.5;
    vec2 d = abs(centered) - vec2(halfSize);
    float windowDist = max(d.x, d.y);
    
    // Sharp window edge with slight softness
    return smoothstep(0.02, -0.02, windowDist);
}

// Get window properties for both day and night
// Returns: x = glass factor (0-1), y = lit factor at night (0-1)
vec2 getWindowFactors(vec3 cellId, vec2 windowCoord, float buildingStyle) {
    // Window grid
    vec2 windowGrid = vec2(windowSpacingH, windowSpacingV);
    vec2 windowCell = floor(windowCoord / windowGrid);
    vec2 windowPos = fract(windowCoord / windowGrid);
    
    // Check if above ground floor
    if (windowCoord.y < windowSpacingV * 0.8) return vec2(0.0);
    
    // Window shape
    vec2 centered = windowPos - 0.5;
    float halfSize = windowSize * 0.5;
    vec2 d = abs(centered) - vec2(halfSize);
    float windowDist = max(d.x, d.y);
    
    // Glass mask (where window is)
    float glassMask = smoothstep(0.02, -0.02, windowDist);
    
    if (glassMask < 0.01) return vec2(0.0);
    
    // Check if this window is lit at night
    vec3 windowId = vec3(cellId.xy, windowCell.x * 100.0 + windowCell.y);
    float litChance = hash3(windowId + vec3(0.0, 0.0, buildingStyle * 50.0));
    
    // Time-based flickering for some windows
    float flicker = hash3(windowId + 500.0);
    if (flicker > 0.95) {
        litChance += sin(iTime * 3.0 + flicker * 100.0) * 0.3;
    }
    
    float isLit = (litChance < windowLitProbability) ? 1.0 : 0.0;
    
    // Add soft glow around lit windows at night
    float insideDist = min(max(d.x, d.y), 0.0);
    float outsideDist = length(max(d, 0.0));
    float glowDist = insideDist + outsideDist;
    float glow = exp(-glowDist * glowDist * 50.0) * 0.5 * windowBloomStrength;
    
    float litFactor = isLit * (glassMask + glow);
    
    return vec2(glassMask, litFactor);
}

// Check if a window is lit and get its glow factor (0-1, with soft edges)
float getWindowLitFactor(vec3 cellId, vec2 windowCoord, float buildingStyle) {
    return getWindowFactors(cellId, windowCoord, buildingStyle).y;
}

// Get window emission color with glow
vec3 getWindowEmission(vec3 cellId, vec2 windowCoord, float buildingStyle) {
    float litFactor = getWindowLitFactor(cellId, windowCoord, buildingStyle);
    
    if (litFactor < 0.001) return vec3(0.0);
    
    // Slight color variation per window
    vec2 windowCell = floor(windowCoord / vec2(windowSpacingH, windowSpacingV));
    float colorVar = hash3(vec3(cellId.xy, windowCell.x * 100.0 + windowCell.y) + 1000.0);
    
    // Warm/cool variation for variety
    vec3 warmTint = vec3(1.0, 0.85, 0.6);   // Warm incandescent
    vec3 coolTint = vec3(0.7, 0.85, 1.0);   // Cool fluorescent/LED
    vec3 tint = mix(warmTint, coolTint, colorVar);
    
    // Some windows have colored curtains/blinds
    float curtainHue = hash3(vec3(cellId.xy, windowCell.x * 100.0 + windowCell.y) + 2000.0);
    if (curtainHue > 0.85) {
        // Occasional colored window (blue TV glow, etc)
        vec3 tvColor = vec3(0.4, 0.5, 0.9);
        tint = mix(tint, tvColor, 0.5);
    }
    
    return windowLightColor * tint * windowGlowIntensity * litFactor;
}

// =============================================================================
// STREET LIGHTS
// =============================================================================

// Get nearest street light position
vec3 getNearestStreetLight(vec3 p) {
    // Street lights at intersections
    float spacing = blockSize;
    vec2 lightGrid = floor(p.xz / spacing + 0.5) * spacing;
    
    // Only place lights near street intersections
    vec2 offset = vec2(streetWidth * 0.5 + 0.5);
    
    // Find closest of 4 corners
    vec3 bestLight = vec3(0.0);
    float bestDist = 1000.0;
    
    for (int i = 0; i < 4; i++) {
        vec2 corner = lightGrid + offset * vec2(
            (i & 1) == 0 ? -1.0 : 1.0,
            (i & 2) == 0 ? -1.0 : 1.0
        );
        
        vec3 lightPos = vec3(corner.x, streetLightHeight, corner.y);
        float d = length(p - lightPos);
        
        if (d < bestDist) {
            bestDist = d;
            bestLight = lightPos;
        }
    }
    
    return bestLight;
}

// Calculate street light contribution with beautiful light pools
vec3 calculateStreetLightContribution(vec3 p, vec3 normal) {
    float nightFactor = 1.0 - getDayNightFactor();
    if (nightFactor < 0.01) return vec3(0.0);
    
    vec3 totalLight = vec3(0.0);
    
    // Check multiple nearby lights
    float spacing = blockSize;
    vec2 baseGrid = floor(p.xz / spacing) * spacing;
    
    for (int gx = -1; gx <= 1; gx++) {
        for (int gz = -1; gz <= 1; gz++) {
            vec2 gridCell = baseGrid + vec2(float(gx), float(gz)) * spacing;
            
            // 4 lights per intersection
            vec2 offset = vec2(streetWidth * 0.5 + 0.8);
            for (int i = 0; i < 4; i++) {
                vec2 corner = gridCell + offset * vec2(
                    (i & 1) == 0 ? -1.0 : 1.0,
                    (i & 2) == 0 ? -1.0 : 1.0
                );
                
                vec3 lightPos = vec3(corner.x, streetLightHeight, corner.y);
                vec3 toLight = lightPos - p;
                float dist = length(toLight);
                
                if (dist < streetLightRadius * 3.0) {
                    vec3 lightDir = toLight / dist;
                    float NdotL = max(dot(normal, lightDir), 0.0);
                    
                    // Smooth circular falloff for beautiful light pools
                    float normalizedDist = dist / streetLightRadius;
                    float attenuation = 1.0 / (1.0 + normalizedDist * normalizedDist);
                    
                    // Downward cone - wider and softer
                    float coneAngle = max(0.0, -lightDir.y);
                    float coneFalloff = smoothstep(0.0, 0.8, coneAngle);
                    attenuation *= coneFalloff;
                    
                    // Extra ground illumination (light pool on street)
                    float groundBoost = 1.0 + max(0.0, normal.y) * 0.5;
                    
                    // Warm color gradient - warmer at center
                    vec3 lightColor = mix(
                        streetLightColor * vec3(1.0, 0.95, 0.85),
                        streetLightColor * vec3(1.0, 0.8, 0.6),
                        attenuation
                    );
                    
                    totalLight += lightColor * streetLightIntensity * NdotL * attenuation * groundBoost;
                }
            }
        }
    }
    
    return totalLight * nightFactor;
}

// =============================================================================
// NORMAL CALCULATION
// =============================================================================

vec3 calculateNormal(vec3 p) {
    vec2 e = vec2(0.001, 0.0);
    float d = getSceneDistance(p);
    return normalize(vec3(
        d - getSceneDistance(p - e.xyy),
        d - getSceneDistance(p - e.yxy),
        d - getSceneDistance(p - e.yyx)
    ));
}

// =============================================================================
// SHADOWS
// =============================================================================

float calculateShadow(vec3 origin, vec3 lightDir) {
    if (shadowStrength < 0.001) return 1.0;
    
    float shadow = 1.0;
    float t = 0.1;
    
    for (int i = 0; i < shadowSteps; i++) {
        if (t > maxDistance * 0.5) break;
        
        vec3 p = origin + lightDir * t;
        float d = getSceneDistance(p);
        
        if (d < 0.001) {
            return 1.0 - shadowStrength;
        }
        
        shadow = min(shadow, shadowSoftness * d / t);
        t += clamp(d, 0.05, 2.0);
    }
    
    return mix(1.0, clamp(shadow, 0.0, 1.0), shadowStrength);
}

// =============================================================================
// AMBIENT OCCLUSION
// =============================================================================

float calculateAO(vec3 p, vec3 n) {
    if (aoStrength < 0.001) return 1.0;
    
    float ao = 0.0;
    float weight = 1.0;
    
    for (int i = 1; i <= 5; i++) {
        float dist = aoRadius * float(i) / 5.0;
        float d = getSceneDistance(p + n * dist);
        ao += weight * (dist - d);
        weight *= 0.7;
    }
    
    return 1.0 - clamp(ao * aoStrength, 0.0, 1.0);
}

// =============================================================================
// SKY
// =============================================================================

// Get moon direction (opposite to sun, slightly offset)
vec3 getMoonDirection() {
    vec3 sunDir = getSunDirection();
    return normalize(-sunDir + vec3(0.1, 0.2, 0.1));
}

// Render realistic moon with visible surface features
vec3 renderMoon(vec3 rayDir, float nightFactor) {
    if (nightFactor < 0.1 || moonBrightness < 0.01) return vec3(0.0);
    
    vec3 moonDir = getMoonDirection();
    if (moonDir.y < -0.1) return vec3(0.0);
    
    float moonDot = dot(rayDir, moonDir);
    float moonAngle = acos(clamp(moonDot, -1.0, 1.0));
    float moonRadius = moonSize;
    
    // Glow around moon (render even outside disc)
    vec3 result = vec3(0.0);
    if (moonGlowSize > 0.01 && moonAngle < moonRadius * 3.0) {
        float glowDist = moonAngle / moonRadius;
        float glow = exp(-glowDist * glowDist * 1.5) * 0.2;
        glow *= smoothstep(3.0, 1.2, glowDist);
        result = moonColor * glow * moonGlowSize * moonBrightness * nightFactor * 0.3;
    }
    
    // Moon disc
    float discMask = smoothstep(moonRadius, moonRadius * 0.95, moonAngle);
    if (discMask < 0.001) return result;
    
    // === UV ON MOON SURFACE ===
    vec3 right = normalize(cross(vec3(0.0, 1.0, 0.001), moonDir));
    vec3 up = normalize(cross(moonDir, right));
    vec3 toMoon = rayDir - moonDir * moonDot;
    
    // UV coordinates on moon disc (-1 to 1)
    vec2 uv = vec2(dot(toMoon, right), dot(toMoon, up)) / moonRadius;
    
    // Distance from center (for limb effects)
    float r = length(uv);
    
    // === MARIA (DARK REGIONS) ===
    // Large irregular dark patches - the "seas" of the moon
    float maria = 0.0;
    
    // Create large dark patches using smooth blobs
    // Mare Imbrium area (upper left)
    maria += smoothstep(0.5, 0.1, length(uv - vec2(-0.3, 0.25))) * 0.8;
    // Mare Serenitatis (upper middle-right)
    maria += smoothstep(0.35, 0.05, length(uv - vec2(0.1, 0.3))) * 0.7;
    // Mare Tranquillitatis (right side)
    maria += smoothstep(0.4, 0.1, length(uv - vec2(0.35, 0.0))) * 0.75;
    // Mare Crisium (far right, isolated)
    maria += smoothstep(0.2, 0.05, length(uv - vec2(0.6, 0.2))) * 0.85;
    // Oceanus Procellarum (large left region)
    maria += smoothstep(0.6, 0.15, length(uv - vec2(-0.5, -0.1))) * 0.65;
    // Mare Nubium (lower center)
    maria += smoothstep(0.35, 0.08, length(uv - vec2(-0.05, -0.4))) * 0.7;
    // Mare Humorum (lower left)
    maria += smoothstep(0.22, 0.05, length(uv - vec2(-0.45, -0.35))) * 0.75;
    
    maria = clamp(maria, 0.0, 1.0);
    
    // === CRATERS ===
    float craters = 0.0;
    
    // Large named craters with bright rims
    // Tycho (southern, very prominent with rays)
    float tycho = length(uv - vec2(-0.05, -0.7));
    craters += smoothstep(0.12, 0.08, tycho) * 0.5;  // dark interior
    craters -= smoothstep(0.08, 0.06, tycho) * 0.3;  // bright rim
    
    // Copernicus (center-left)
    float copernicus = length(uv - vec2(-0.3, 0.0));
    craters += smoothstep(0.1, 0.06, copernicus) * 0.4;
    craters -= smoothstep(0.06, 0.04, copernicus) * 0.25;
    
    // Kepler
    float kepler = length(uv - vec2(-0.55, 0.05));
    craters += smoothstep(0.06, 0.03, kepler) * 0.35;
    craters -= smoothstep(0.03, 0.02, kepler) * 0.2;
    
    // Aristarchus (very bright)
    float aristarchus = length(uv - vec2(-0.65, 0.2));
    craters -= smoothstep(0.05, 0.02, aristarchus) * 0.4; // Extra bright
    
    // Plato (dark-floored in highlands)
    float plato = length(uv - vec2(-0.05, 0.55));
    craters += smoothstep(0.08, 0.04, plato) * 0.5;
    
    // Many smaller random craters
    for (int i = 0; i < 20; i++) {
        vec2 pos = vec2(
            hash(vec2(float(i), 11.0)) * 1.8 - 0.9,
            hash(vec2(float(i), 22.0)) * 1.8 - 0.9
        );
        float size = 0.02 + hash(vec2(float(i), 33.0)) * 0.06;
        float d = length(uv - pos);
        // Dark bowl with bright rim
        craters += smoothstep(size * 1.3, size * 0.8, d) * 0.2;
        craters -= smoothstep(size * 0.8, size * 0.5, d) * 0.15;
    }
    
    // Tiny crater texture
    for (int i = 0; i < 30; i++) {
        vec2 pos = vec2(
            hash(vec2(float(i), 44.0)) * 2.0 - 1.0,
            hash(vec2(float(i), 55.0)) * 2.0 - 1.0
        );
        float size = 0.01 + hash(vec2(float(i), 66.0)) * 0.02;
        float d = length(uv - pos);
        craters += smoothstep(size, size * 0.3, d) * 0.1;
    }
    
    // === SURFACE NOISE (fine texture) ===
    float noise = 0.0;
    noise += valueNoise(uv * 8.0) * 0.15;
    noise += valueNoise(uv * 16.0) * 0.1;
    noise += valueNoise(uv * 32.0) * 0.05;
    
    // === COMBINE INTO ALBEDO ===
    // Highlands are bright (0.7-0.9), maria are dark (0.3-0.5)
    float albedo = mix(0.8, 0.35, maria);
    
    // Add crater detail
    albedo -= craters * 0.3;
    
    // Add fine noise
    albedo += (noise - 0.15) * 0.2;
    
    // Keep in valid range
    albedo = clamp(albedo, 0.2, 1.0);
    
    // === LIMB DARKENING ===
    // Edges of moon are darker (viewing angle effect)
    float limb = sqrt(1.0 - r * r * 0.9);
    limb = mix(0.6, 1.0, limb);
    
    // === PHASE LIGHTING ===
    // Simple phase: full moon at night (sun behind us)
    // The moon we see at night is mostly full, so mostly lit
    vec3 sunDir = getSunDirection();
    float phase = dot(moonDir, -sunDir) * 0.5 + 0.5; // 0 = new moon, 1 = full
    phase = mix(0.7, 1.0, phase); // Keep it mostly lit
    
    // === FINAL MOON COLOR ===
    vec3 moonSurface = moonColor * albedo * limb * phase * moonBrightness;
    
    // Slight warm tint variation
    moonSurface *= vec3(1.0, 0.98, 0.95);
    
    // Add to glow
    result += moonSurface * discMask * nightFactor;
    
    return result;
}

// Render airplane lights
vec3 renderAirplanes(vec3 rayDir, float nightFactor) {
    if (nightFactor < 0.3 || airplaneCount < 1) return vec3(0.0);
    
    vec3 totalLight = vec3(0.0);
    
    for (int i = 0; i < 8; i++) {
        if (i >= airplaneCount) break;
        
        // Each airplane has unique parameters based on index
        float planeId = float(i);
        float planeHash1 = hash(vec2(planeId, 0.0));
        float planeHash2 = hash(vec2(planeId, 1.0));
        float planeHash3 = hash(vec2(planeId, 2.0));
        float planeHash4 = hash(vec2(planeId, 3.0));
        
        // Airplane path - flies in a straight line across the sky
        float pathAngle = planeHash1 * 6.28318; // Random direction
        float pathOffset = (planeHash2 - 0.5) * 2.0; // Random offset from center
        float startTime = planeHash3 * 100.0; // Random start time offset
        
        // Calculate airplane position over time
        float flightProgress = fract((iTime * airplaneSpeed * 0.02 + startTime) * (0.5 + planeHash4 * 0.5));
        
        // Path goes from one horizon to the other
        float pathX = (flightProgress - 0.5) * 4.0;
        float pathY = 0.3 + planeHash2 * 0.4; // Altitude in sky (0.3 to 0.7)
        float pathZ = pathOffset;
        
        // Rotate path by angle
        vec3 planeDir = normalize(vec3(
            pathX * cos(pathAngle) - pathZ * sin(pathAngle),
            pathY,
            pathX * sin(pathAngle) + pathZ * cos(pathAngle)
        ));
        
        // Check if looking at airplane
        float planeDot = dot(rayDir, planeDir);
        float planeAngle = acos(clamp(planeDot, -1.0, 1.0));
        
        // Airplane light size (very small point)
        float lightRadius = 0.003 * airplaneLightSize;
        
        if (planeAngle < lightRadius * 3.0) {
            // Blinking pattern - aircraft have specific light patterns
            float blinkTime = iTime * 2.0 + planeId * 10.0;
            
            // Navigation lights pattern:
            // - Steady white (tail)
            // - Blinking red (anti-collision)
            // - Blinking white strobe
            
            float steadyWhite = 0.5;
            float blinkingRed = step(0.7, fract(blinkTime * 0.8)) * 0.8;
            float strobe = step(0.9, fract(blinkTime * 1.5)) * 1.5;
            
            // Combine lights
            float intensity = steadyWhite + blinkingRed + strobe;
            
            // Point light falloff
            float lightFalloff = exp(-planeAngle * planeAngle / (lightRadius * lightRadius));
            
            // Light color - mix of white and red
            vec3 lightColor = mix(vec3(1.0, 1.0, 1.0), vec3(1.0, 0.2, 0.1), blinkingRed / (intensity + 0.01));
            
            totalLight += lightColor * lightFalloff * intensity * nightFactor;
        }
    }
    
    return totalLight;
}

vec3 getSkyColor(vec3 rayDir) {
    float dayFactor = getDayNightFactor();
    float nightFactor = 1.0 - dayFactor;
    float sunsetFactor = getSunsetFactor();
    
    // Vertical gradient
    float t = pow(max(0.0, rayDir.y), 0.6);
    
    // Day sky
    vec3 daySky = mix(daySkyHorizon, daySkyTop, t);
    
    // Night sky
    vec3 nightSky = mix(nightSkyHorizon, nightSkyTop, t);
    
    // Blend day/night
    vec3 sky = mix(nightSky, daySky, dayFactor);
    
    // Add sunset colors at horizon
    vec3 sunsetSky = mix(sunsetColor, sky, t);
    sky = mix(sky, sunsetSky, sunsetFactor * (1.0 - t));
    
    // Sun disc (daytime)
    vec3 sunDir = getSunDirection();
    float sunDot = max(0.0, dot(rayDir, sunDir));
    
    if (dayFactor > 0.1) {
        vec3 sunCol = mix(sunsetColor, vec3(1.0, 0.95, 0.9), dayFactor);
        sky += sunCol * pow(sunDot, 256.0) * 2.0;
        sky += sunCol * pow(sunDot, 8.0) * 0.3 * dayFactor;
    }
    
    // Moon (nighttime)
    sky += renderMoon(rayDir, nightFactor);
    
    // Stars at night
    if (dayFactor < 0.3 && rayDir.y > 0.0) {
        vec2 starCoord = rayDir.xz / (rayDir.y + 0.1) * 100.0;
        vec2 starCell = floor(starCoord);
        float starDensity = hash(starCell);
        
        if (starDensity > 0.997) {
            float starPhase = hash(starCell + 100.0) * 6.28318;
            float starSpeed = 1.5 + hash(starCell + 200.0) * 3.0;
            float starBrightness = 0.5 + hash(starCell + 300.0) * 0.5;
            
            float twinkle = 0.5 + 0.5 * sin(iTime * starSpeed + starPhase);
            sky += vec3(1.0) * twinkle * starBrightness * nightFactor * 2.0;
        }
    }
    
    // Airplane lights
    sky += renderAirplanes(rayDir, nightFactor);
    
    return sky;
}

// =============================================================================
// FOG
// =============================================================================

vec3 applyFog(vec3 color, float distance, vec3 rayDir, vec3 rayOrigin) {
    float dayFactor = getDayNightFactor();
    
    // Distance fog
    float fogAmount = 1.0 - exp(-distance * fogDensity);
    
    // Height fog (denser at ground level)
    float heightFog = exp(-rayOrigin.y * fogHeightFalloff);
    fogAmount = mix(fogAmount, fogAmount * 1.5, heightFog);
    
    // Fog color based on time of day
    vec3 dayFogColor = mix(daySkyHorizon, vec3(0.8, 0.85, 0.9), 0.5);
    vec3 nightFogColor = mix(nightSkyHorizon, vec3(0.02, 0.02, 0.04), 0.5);
    vec3 fogColor = mix(nightFogColor, dayFogColor, dayFactor);
    
    // Sunset tint
    fogColor = mix(fogColor, sunsetColor * 0.5, getSunsetFactor() * 0.3);
    
    return mix(color, fogColor, clamp(fogAmount, 0.0, 1.0));
}

// =============================================================================
// MATERIALS
// =============================================================================

vec3 getMaterialColor(SceneResult hit, vec3 p, vec3 normal) {
    float dayFactor = getDayNightFactor();
    float nightFactor = 1.0 - dayFactor;
    
    if (hit.materialID == MAT_STREET) {
        // Street with subtle variation and wetness at night
        float noise = valueNoise(p.xz * 2.0) * 0.1;
        vec3 color = streetColor * (0.9 + noise);
        // Wet street reflection at night
        float wetness = nightFactor * 0.3;
        color = mix(color, color * 1.2, wetness);
        return color;
    }
    else if (hit.materialID == MAT_SIDEWALK) {
        float noise = valueNoise(p.xz * 4.0) * 0.15;
        return sidewalkColor * (0.85 + noise);
    }
    else if (hit.materialID == MAT_BUILDING) {
        BuildingData building = getBuildingData(hit.cellId.xy);
        
        // Base building color (wall/concrete)
        vec3 wallColor = building.color;
        
        // Darken based on face orientation (fake AO)
        float faceDark = 0.7 + 0.3 * abs(normal.y);
        wallColor *= faceDark;
        
        // Add subtle variation to walls
        float noise = valueNoise(p.xz * 0.5 + p.y * 0.1) * 0.1;
        wallColor *= (0.95 + noise);
        
        // === WINDOW GLASS (visible day and night) ===
        vec2 windowFactors = getWindowFactors(hit.cellId, hit.windowCoord, building.style);
        float glassMask = windowFactors.x;
        
        if (glassMask > 0.01) {
            // Glass color - darker, slightly blue tinted
            vec3 glassColor = vec3(0.15, 0.18, 0.22);
            
            // Sky reflection on glass during day
            vec3 sunDir = getSunDirection();
            float skyReflect = max(0.0, dot(normal, vec3(0.0, 1.0, 0.0)));
            vec3 reflectedSky = mix(daySkyHorizon, daySkyTop, skyReflect * 0.5) * 0.3;
            
            // Sun reflection (specular glint on glass)
            vec3 viewDir = normalize(uCameraPosition - p);
            vec3 reflectDir = reflect(-sunDir, normal);
            float sunSpec = pow(max(0.0, dot(viewDir, reflectDir)), 64.0);
            vec3 sunGlint = vec3(1.0, 0.95, 0.9) * sunSpec * 0.5 * dayFactor;
            
            // Combine glass appearance
            vec3 dayGlass = glassColor + reflectedSky * dayFactor + sunGlint;
            
            // At night, unlit windows are darker, lit ones glow
            vec3 nightGlassUnlit = glassColor * 0.3;
            
            // Blend wall and glass
            vec3 color = mix(wallColor, dayGlass, glassMask);
            
            // Add window frame (thin border around glass)
            float frameMask = glassMask * (1.0 - smoothstep(0.0, 0.08, abs(fract(hit.windowCoord.x / windowSpacingH) - 0.5) - windowSize * 0.5 + 0.03));
            frameMask += glassMask * (1.0 - smoothstep(0.0, 0.08, abs(fract(hit.windowCoord.y / windowSpacingV) - 0.5) - windowSize * 0.5 + 0.03));
            vec3 frameColor = wallColor * 0.7;
            color = mix(color, frameColor, clamp(frameMask * 0.3, 0.0, 1.0));
            
            return color;
        }
        
        return wallColor;
    }
    else if (hit.materialID == MAT_LAMPPOST) {
        // Dark metal post
        return vec3(0.15, 0.15, 0.17);
    }
    else if (hit.materialID == MAT_LAMP) {
        // Lamp color depends on time - bright at night
        return vec3(0.3, 0.3, 0.32); // Base color when not emitting
    }
    else if (hit.materialID == MAT_ROOF) {
        // Rooftop structures - industrial look
        BuildingData building = getBuildingData(hit.cellId.xy);
        
        float noise = valueNoise(p.xz * 2.0 + p.y) * 0.15;
        
        if (building.roofType == ROOF_AC_UNITS) {
            // AC units - metallic gray/white
            return vec3(0.6, 0.62, 0.65) * (0.85 + noise);
        }
        else if (building.roofType == ROOF_WATER_TANK) {
            // Water tank - rusty metal or painted
            float rust = hash(hit.cellId.xy + 800.0);
            vec3 tankColor = mix(
                vec3(0.5, 0.45, 0.4),   // Rusty
                vec3(0.3, 0.35, 0.5),   // Painted blue
                step(0.5, rust)
            );
            return tankColor * (0.9 + noise);
        }
        else if (building.roofType == ROOF_PENTHOUSE) {
            // Penthouse - matches building but slightly different
            return building.color * 1.1 * (0.9 + noise);
        }
        else if (building.roofType == ROOF_ANTENNA) {
            // Antenna - metallic
            return vec3(0.5, 0.5, 0.52) * (0.9 + noise);
        }
        else if (building.roofType == ROOF_HELIPAD) {
            // Helipad - concrete with markings would be nice but keep simple
            return vec3(0.45, 0.45, 0.43) * (0.9 + noise);
        }
        
        return vec3(0.4, 0.4, 0.42); // Default roof
    }
    
    return vec3(0.3); // Default ground
}

// =============================================================================
// RAYMARCHING
// =============================================================================

SceneResult rayMarch(vec3 ro, vec3 rd) {
    float t = 0.0;
    SceneResult result;
    result.distance = -1.0;
    result.materialID = -1;
    
    for (int i = 0; i < maxSteps; i++) {
        vec3 p = ro + rd * t;
        result = getScene(p);
        
        if (result.distance < surfaceThreshold) {
            result.distance = t;
            return result;
        }
        
        t += result.distance * 0.8;
        
        if (t > maxDistance) {
            result.distance = -1.0;
            return result;
        }
    }
    
    result.distance = -1.0;
    return result;
}

// =============================================================================
// MAIN LIGHTING
// =============================================================================

vec3 calculateLighting(vec3 p, vec3 normal, vec3 viewDir, SceneResult hit) {
    float dayFactor = getDayNightFactor();
    float nightFactor = 1.0 - dayFactor;
    
    // Get base material color
    vec3 baseColor = getMaterialColor(hit, p, normal);
    
    // Sun direction and color
    vec3 sunDir = getSunDirection();
    vec3 sunCol = mix(vec3(0.1, 0.1, 0.2), vec3(1.0, 0.95, 0.9), dayFactor);
    sunCol = mix(sunCol, sunsetColor, getSunsetFactor());
    
    // Ambient - warmer at night due to artificial lights
    vec3 nightAmbient = vec3(0.03, 0.025, 0.04);
    vec3 dayAmbientColor = vec3(0.15);
    vec3 ambient = baseColor * mix(nightAmbient, dayAmbientColor, dayFactor);
    
    // Diffuse (sun/moon)
    float NdotL = max(dot(normal, sunDir), 0.0);
    float shadow = calculateShadow(p + normal * 0.01, sunDir);
    vec3 diffuse = baseColor * sunCol * NdotL * shadow * mix(0.3, 1.0, dayFactor);
    
    // Ambient occlusion
    float ao = calculateAO(p, normal);
    
    // Street lights illuminate all surfaces at night
    vec3 streetLight = vec3(0.0);
    if (nightFactor > 0.01) {
        streetLight = calculateStreetLightContribution(p, normal) * baseColor;
        
        // Specular highlight from street lights on wet surfaces
        if (hit.materialID == MAT_STREET) {
            float spacing = blockSize;
            vec2 lampGrid = floor(p.xz / spacing + 0.5) * spacing;
            vec2 lampOffset = vec2(streetWidth * 0.5 + 0.8);
            
            for (int i = 0; i < 4; i++) {
                vec2 corner = lampGrid + lampOffset * vec2(
                    (i & 1) == 0 ? -1.0 : 1.0,
                    (i & 2) == 0 ? -1.0 : 1.0
                );
                vec3 lightPos = vec3(corner.x, streetLightHeight, corner.y);
                vec3 toLight = normalize(lightPos - p);
                vec3 halfVec = normalize(toLight + viewDir);
                float spec = pow(max(dot(normal, halfVec), 0.0), 32.0);
                float dist = length(lightPos - p);
                float atten = 1.0 / (1.0 + dist * dist * 0.05);
                streetLight += streetLightColor * spec * atten * 0.5 * nightFactor;
            }
        }
    }
    
    // Window emission with bloom effect (buildings only, at night)
    vec3 emission = vec3(0.0);
    if (hit.materialID == MAT_BUILDING && nightFactor > 0.1) {
        BuildingData building = getBuildingData(hit.cellId.xy);
        vec3 windowLight = getWindowEmission(hit.cellId, hit.windowCoord, building.style);
        
        // Add bloom by checking nearby window cells
        if (length(windowLight) > 0.01) {
            emission = windowLight * nightFactor * (1.0 + windowBloomStrength);
        }
    }
    
    // Lamp head emission - bright glowing sphere at night
    if (hit.materialID == MAT_LAMP && nightFactor > 0.1) {
        emission = streetLightColor * streetLightIntensity * 2.0 * nightFactor;
    }
    
    // Combine
    vec3 finalColor = (ambient + diffuse) * ao + streetLight + emission;
    
    return finalColor;
}

// =============================================================================
// CAMERA
// =============================================================================

vec3 getCameraRay(vec2 uv) {
    float focalLength = 1.0 / tan(radians(uCameraFOV) * 0.5);
    return normalize(
        uCameraForward * focalLength +
        uCameraRight * uv.x +
        uCameraUp * uv.y
    );
}

// =============================================================================
// MAIN
// =============================================================================

out vec4 fragColor;
in vec2 fragCoord;

// Calculate glow from nearby light sources (volumetric light approximation)
vec3 calculateLightGlows(vec3 rayOrigin, vec3 rayDir, float maxDist) {
    float nightFactor = 1.0 - getDayNightFactor();
    if (nightFactor < 0.01 || lightGlowSize < 0.01) return vec3(0.0);
    
    vec3 totalGlow = vec3(0.0);
    
    // Sample street lamps
    float spacing = blockSize;
    vec2 baseGrid = floor(rayOrigin.xz / spacing) * spacing;
    
    for (int gx = -1; gx <= 2; gx++) {
        for (int gz = -1; gz <= 2; gz++) {
            vec2 gridCell = baseGrid + vec2(float(gx), float(gz)) * spacing;
            vec2 lampOffset = vec2(streetWidth * 0.5 + 0.8);
            
            for (int i = 0; i < 4; i++) {
                vec2 corner = gridCell + lampOffset * vec2(
                    (i & 1) == 0 ? -1.0 : 1.0,
                    (i & 2) == 0 ? -1.0 : 1.0
                );
                
                vec3 lightPos = vec3(corner.x, streetLightHeight, corner.y);
                
                // Ray-sphere closest approach for glow
                vec3 toLight = lightPos - rayOrigin;
                float tClosest = max(0.0, dot(toLight, rayDir));
                
                // Don't show glow behind camera or past hit point
                if (tClosest > maxDist) continue;
                
                vec3 closestPoint = rayOrigin + rayDir * tClosest;
                float distToLight = length(lightPos - closestPoint);
                
                // Glow falloff
                float glowRadius = lightGlowSize * 2.0;
                float glow = exp(-distToLight * distToLight / (glowRadius * glowRadius));
                
                // Distance fade
                float distFade = 1.0 / (1.0 + tClosest * 0.02);
                
                // Warm glow color
                vec3 glowColor = streetLightColor * vec3(1.0, 0.95, 0.85);
                totalGlow += glowColor * glow * distFade * streetLightIntensity * 0.3;
            }
        }
    }
    
    return totalGlow * nightFactor;
}

// Calculate window glow contribution to atmosphere
vec3 calculateWindowGlow(vec3 rayOrigin, vec3 rayDir, float maxDist) {
    float nightFactor = 1.0 - getDayNightFactor();
    if (nightFactor < 0.1 || windowBloomStrength < 0.01) return vec3(0.0);
    
    vec3 totalGlow = vec3(0.0);
    
    // Sample buildings for window glow
    vec2 cellId = getCellId(rayOrigin.xz);
    
    for (int dx = -2; dx <= 2; dx++) {
        for (int dz = -2; dz <= 2; dz++) {
            vec2 checkCell = cellId + vec2(float(dx), float(dz));
            BuildingData building = getBuildingData(checkCell);
            
            if (!building.exists) continue;
            
            // Building center
            vec2 buildingCenter = (checkCell + 0.5) * blockSize - streetWidth * 0.5;
            vec3 buildingPos = vec3(buildingCenter.x, building.height * 0.5, buildingCenter.y);
            
            // Simple distance-based glow
            vec3 toBuilding = buildingPos - rayOrigin;
            float tClosest = max(0.0, dot(toBuilding, rayDir));
            
            if (tClosest > maxDist) continue;
            
            vec3 closestPoint = rayOrigin + rayDir * tClosest;
            float dist = length(buildingPos - closestPoint);
            
            // Only add glow if close to building
            float buildingRadius = blockSize * 0.7;
            if (dist < buildingRadius * 2.0) {
                // Glow based on window density
                float glow = exp(-dist * dist / (buildingRadius * buildingRadius)) * 0.15;
                float distFade = 1.0 / (1.0 + tClosest * 0.01);
                
                // Warm window glow
                totalGlow += windowLightColor * glow * distFade * windowLitProbability;
            }
        }
    }
    
    return totalGlow * nightFactor * windowBloomStrength;
}

void main() {
    vec2 uv = (fragCoord - 0.5) * 2.0;
    uv.x *= iResolution.x / iResolution.y;
    
    vec3 rayDir = getCameraRay(uv);
    
    // Raymarch
    SceneResult hit = rayMarch(uCameraPosition, rayDir);
    
    vec3 finalColor;
    float hitDist = hit.distance > 0.0 ? hit.distance : maxDistance;
    
    if (hit.distance > 0.0) {
        vec3 hitPoint = uCameraPosition + rayDir * hit.distance;
        vec3 normal = calculateNormal(hitPoint);
        vec3 viewDir = -rayDir;
        
        finalColor = calculateLighting(hitPoint, normal, viewDir, hit);
        finalColor = applyFog(finalColor, hit.distance, rayDir, uCameraPosition);
    } else {
        finalColor = getSkyColor(rayDir);
    }
    
    // Add volumetric light glows (street lamps)
    vec3 lampGlow = calculateLightGlows(uCameraPosition, rayDir, hitDist);
    finalColor += lampGlow;
    
    // Add atmospheric window glow
    vec3 windowGlow = calculateWindowGlow(uCameraPosition, rayDir, hitDist);
    finalColor += windowGlow * atmosphericScatter;
    
    // Tone mapping for HDR-like effect
    finalColor = finalColor / (finalColor + vec3(1.0)); // Reinhard
    
    // Gamma correction
    finalColor = pow(finalColor, vec3(0.4545));
    
    // Subtle vignette
    float vignette = 1.0 - dot(uv * 0.5, uv * 0.5) * 0.15;
    finalColor *= vignette;
    
    fragColor = vec4(finalColor, 1.0);
}

// =============================================================================
// CAMERA CONTROLS
// =============================================================================
//
// Use keyboard and mouse to explore the city:
//   - W/S or Up/Down:     Move forward/backward
//   - A/D or Left/Right:  Strafe left/right
//   - Q/Space:            Move up
//   - E/Ctrl:             Move down
//   - Right-click + Drag: Look around
//   - Mouse Scroll:       Zoom (FOV)
//   - Shift:              Sprint
//
// =============================================================================
// FEATURES
// =============================================================================
//
// [X] Procedural building generation with varied heights
// [X] Downtown core with taller skyscrapers
// [X] VARIED ROOFTOP STRUCTURES:
//    - AC units (multiple boxes)
//    - Water tanks (cylinders with support legs)
//    - Penthouses (smaller upper floor)
//    - Antennas/spires (tall masts with cross bars)
//    - Helipads (platforms with safety rails)
// [X] Street grid with sidewalks
// [X] Day/night cycle with smooth transitions
// [X] Windows visible day AND night:
//    - Daytime: dark glass with sky reflections & sun glints
//    - Nighttime: lit windows with warm glow
// [X] Window color variation (warm, cool, TV blue)
// [X] Visible street lamp poles and glowing bulbs
// [X] Volumetric light halos around street lamps
// [X] Beautiful light pools on streets
// [X] Wet street reflections at night
// [X] Specular highlights from street lights
// [X] Atmospheric window glow
// [X] Soft shadows from sun/moon
// [X] Ambient occlusion between buildings
// [X] Atmospheric fog with day/night variation
// [X] HDR tone mapping (Reinhard)
// [X] Sunset/sunrise color grading
// [X] Twinkling stars at night
// [X] Subtle window flickering
// [X] REALISTIC MOON with visible lunar features:
//    - Maria (dark "seas"): Imbrium, Serenitatis, Tranquillitatis, Crisium, etc.
//    - Highlands (bright regions) with texture
//    - Named craters: Tycho, Copernicus, Kepler, Aristarchus, Plato
//    - Many random craters of various sizes with dark bowls and bright rims
//    - Fine surface noise texture
//    - Limb darkening for spherical appearance
//    - Phase lighting based on sun position
//    - Subtle atmospheric glow
// [X] AIRPLANE LIGHTS:
//    - Multiple planes flying across sky
//    - Realistic blinking pattern (strobe + beacon)
//    - Red and white navigation lights
//    - Configurable count and speed
//
// =============================================================================
// TIPS
// =============================================================================
//
// Good starting camera positions:
//   - Bird's eye: Position (0, 80, 0), look down
//   - Street level: Position (5, 2, 5), look forward
//   - Skyline: Position (-100, 30, 0), look toward center
//
// For beautiful night scene:
//   - Set timeOfDay to 0.0-0.2 or 0.8-1.0
//   - Increase windowLitProbability (0.5-0.7) for more lit windows
//   - Increase streetLightIntensity (2.0+) for brighter streets
//   - Increase lightGlowSize (1.5) for larger light halos
//   - Increase windowBloomStrength (2.0) for softer window glow
//   - Set atmosphericScatter (0.5) for moody atmosphere
//
// For sunset:
//   - Set timeOfDay to ~0.72-0.78
//   - Adjust sunsetColor for different moods
//
// For cinematic look:
//   - Lower fogDensity (0.001) for clearer view
//   - Increase windowBloomStrength for dreamy glow
//   - Set animateTime = 1.0 with slow timeSpeed (0.02)
//
// For beautiful moon view:
//   - Set timeOfDay to 0.0 (midnight) for full moon
//   - Increase moonSize (0.08-0.12) for larger, more visible moon
//   - Increase moonBrightness (1.5) for brighter lunar surface
//   - Set moonGlowSize (0.3-0.6) for soft atmospheric halo
//   - Lower city lights to see moon better
//
// For airplane watching:
//   - Set airplaneCount to 5-8 for more traffic
//   - Adjust airplaneSpeed (0.3 slow, 1.0 fast)
//   - airplaneLightSize (2.0+) for more visible lights
//   - Best at night or dusk
//
// =============================================================================



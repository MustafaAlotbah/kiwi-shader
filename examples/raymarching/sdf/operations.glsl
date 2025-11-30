// =============================================================================
// SDF Operations Library
// =============================================================================
// Boolean operations and modifiers for combining and transforming SDFs.
//
// These operations allow you to combine primitive shapes into complex objects
// using Constructive Solid Geometry (CSG) techniques.
//
// Usage:
//   float sphere1 = sdSphere(p - vec3(-1,0,0), 1.0);
//   float sphere2 = sdSphere(p - vec3(1,0,0), 1.0);
//   float combined = opUnion(sphere1, sphere2);
//
// =============================================================================


// =============================================================================
// UNION (Boolean OR)
// =============================================================================
// Combines two shapes - keeps the volume covered by EITHER shape
//
// Parameters:
//   d1, d2 - Distance fields of two shapes
//
// Returns: Distance to nearest surface of either shape
//
// Visual: A ∪ B
//      ___        ___
//    /    \     /    \
//   |  A  |    |  B  |    Union:   __________
//   \____/     \____/            /           \
//                               |    A ∪ B   |
//                               \___________/
//
// Math: min(d1, d2)
//   - Takes the minimum distance (closest surface wins)
//
float opUnion(float d1, float d2) {
    return min(d1, d2);
}


// =============================================================================
// SUBTRACTION (Boolean DIFFERENCE)
// =============================================================================
// Subtracts second shape from first - carves B out of A
//
// Parameters:
//   d1 - Distance field of base shape (A)
//   d2 - Distance field of shape to subtract (B)
//
// Returns: Distance to resulting carved shape
//
// Visual: A \ B (A minus B)
//    ___________           ___
//   /           \         /   \
//  |      A      |  -    |  B  |  =   ___  ___
//   \___________/         \___/      /   \/   \
//                                   |  A  \/  |
//                                    \________/
//
// Math: max(d1, -d2)
//   - Inverts B (negative becomes positive)
//   - Takes maximum (intersection of A with inverted B)
//
float opSubtraction(float d1, float d2) {
    return max(d1, -d2);
}


// =============================================================================
// INTERSECTION (Boolean AND)
// =============================================================================
// Keeps only the volume where BOTH shapes overlap
//
// Parameters:
//   d1, d2 - Distance fields of two shapes
//
// Returns: Distance to intersection volume
//
// Visual: A ∩ B
//    ___       ___
//   /   \     /   \
//  |  A  \___/  B  |    Intersection:  ___
//   \____/   \____/                   /   \
//                                    | A∩B |
//                                     \___/
//
// Math: max(d1, d2)
//   - Takes the maximum distance (both must be satisfied)
//
float opIntersection(float d1, float d2) {
    return max(d1, d2);
}


// =============================================================================
// SMOOTH UNION (Smooth Blend)
// =============================================================================
// Smoothly blends two shapes together with a rounded transition
//
// Parameters:
//   d1, d2  - Distance fields of two shapes
//   k       - Smoothness factor (larger = smoother blend)
//             Typical values: 0.1 to 1.0
//
// Returns: Smoothly blended distance field
//
// Visual: Smooth A ∪ B
//    ___       ___
//   /   \     /   \        Smooth Union:  _______________
//  |  A  |   |  B  |  ->                /               \
//   \___/     \___/                    |  Smooth A ∪ B  |
//                                       \_______________/
//        Hard edges  ->  Rounded, organic blend
//
// Math: Polynomial smooth minimum function
//   - Uses exponential-based blending
//   - Preserves distance field properties
//
float opSmoothUnion(float d1, float d2, float k) {
    float h = clamp(0.5 + 0.5 * (d2 - d1) / k, 0.0, 1.0);
    return mix(d2, d1, h) - k * h * (1.0 - h);
}


// =============================================================================
// SMOOTH SUBTRACTION
// =============================================================================
// Smoothly subtracts second shape from first with rounded edges
//
// Parameters:
//   d1, d2  - Distance fields (d1 - d2)
//   k       - Smoothness factor (larger = smoother)
//
// Returns: Smoothly carved distance field
//
// Visual: Smooth A \ B
//   ___________               ___________
//  /           \    ___      /          |\
// |      A      |  /   \  = |    A      | \___
//  \___________/  |  B  |    \_________/      (rounded edge)
//                  \___/
//
// Math: Similar to smooth union, but with inverted d2
//
float opSmoothSubtraction(float d1, float d2, float k) {
    float h = clamp(0.5 - 0.5 * (d2 + d1) / k, 0.0, 1.0);
    return mix(d1, -d2, h) + k * h * (1.0 - h);
}


// =============================================================================
// SMOOTH INTERSECTION
// =============================================================================
// Smoothly intersects two shapes with rounded transition
//
// Parameters:
//   d1, d2  - Distance fields of two shapes
//   k       - Smoothness factor
//
// Returns: Smooth intersection distance field
//
// Visual: Smooth A ∩ B
//    ___       ___
//   /   \     /   \        Smooth:      ___
//  |  A  \___/  B  |  ->              /     \
//   \____/   \____/                  | A ∩ B |
//                                     \_____/
//      Sharp corner  ->  Rounded, organic shape
//
float opSmoothIntersection(float d1, float d2, float k) {
    float h = clamp(0.5 - 0.5 * (d2 - d1) / k, 0.0, 1.0);
    return mix(d2, d1, h) + k * h * (1.0 - h);
}


// =============================================================================
// ROUNDED (Onion)
// =============================================================================
// Creates a shell/hollow version of a shape with rounded edges
//
// Parameters:
//   distance  - Original distance field
//   thickness - Thickness of the shell
//
// Returns: Hollowed distance field
//
// Visual:
//     ___          ___
//    /   \        // \\        (shell of thickness t)
//   |  ●  |  ->  ||   ||
//    \___/        \\ //
//
//   Solid    ->   Hollow shell
//
// Math: abs(d) - thickness
//   - Creates two surfaces: inner and outer
//
float opRounded(float distance, float thickness) {
    return abs(distance) - thickness;
}


// =============================================================================
// REPETITION (Infinite)
// =============================================================================
// Repeats a shape infinitely in all directions
//
// Parameters:
//   position - Point in 3D space
//   spacing  - Distance between repetitions (cell size)
//
// Returns: Modified position in local cell coordinates
//
// Visual (2D view):
//   ●   ●   ●   ●
//   ●   ●   ●   ●    <- infinite grid of repeated shapes
//   ●   ●   ●   ●       spacing = distance between
//   ●   ●   ●   ●
//
// Usage:
//   vec3 repeated = opRepeat(position, vec3(5.0));
//   float dist = sdSphere(repeated, 1.0);
//
vec3 opRepeat(vec3 position, vec3 spacing) {
    // Modulo operation to fold space
    return mod(position + 0.5 * spacing, spacing) - 0.5 * spacing;
}


// =============================================================================
// REPETITION (Limited/Finite)
// =============================================================================
// Repeats a shape a limited number of times
//
// Parameters:
//   position - Point in 3D space
//   spacing  - Distance between repetitions
//   count    - Number of repetitions along each axis (half-count from center)
//
// Returns: Modified position
//
// Visual (2D view):
//           3x3 grid
//        ●   ●   ●
//        ●   ●   ●     <- limited repetition
//        ●   ●   ●        count = vec3(3,3,1)
//
vec3 opRepeatLimited(vec3 position, vec3 spacing, vec3 count) {
    // Clamp to limited range
    vec3 q = position - spacing * clamp(round(position / spacing), -count, count);
    return q;
}


// =============================================================================
// SYMMETRY (Mirror)
// =============================================================================
// Mirrors a shape across a plane
//
// Parameters:
//   position - Point in 3D space
//   axis     - Axis to mirror across (0=X, 1=Y, 2=Z)
//
// Returns: Mirrored position
//
// Visual (mirroring across Y axis):
//     |
//   ● | ●     <- shape mirrored
//     |          across vertical axis
//
vec3 opSymmetryX(vec3 position) {
    position.x = abs(position.x);
    return position;
}

vec3 opSymmetryY(vec3 position) {
    position.y = abs(position.y);
    return position;
}

vec3 opSymmetryZ(vec3 position) {
    position.z = abs(position.z);
    return position;
}

// Mirror across all axes (8-way symmetry)
vec3 opSymmetryAll(vec3 position) {
    return abs(position);
}


// =============================================================================
// DISPLACEMENT (Distortion)
// =============================================================================
// Warps/displaces a shape using a function
//
// Parameters:
//   distance - Original distance field
//   position - Point being sampled
//
// Returns: Displaced distance field
//
// Note: You need to provide the displacement function
//
// Visual:
//    ___              ~~~
//   /   \            /~~~\     <- displaced/warped sphere
//  |  ●  |    ->    |~~●~~|       using noise or sin functions
//   \___/            \~~~/
//
// Example usage:
//   float displacement = sin(5.0 * p.x) * sin(5.0 * p.y) * sin(5.0 * p.z) * 0.1;
//   float dist = sdSphere(p, 1.0) + displacement;
//


// =============================================================================
// ELONGATION (Stretch)
// =============================================================================
// Stretches a shape along specified axes
//
// Parameters:
//   position - Point in 3D space
//   stretch  - How much to elongate along each axis
//
// Returns: Modified position for elongated shape
//
// Visual (elongating a sphere along X):
//    ___           _______
//   /   \         /       \
//  |  ●  |  ->   |    ●    |  <- stretched/elongated
//   \___/         \_______/
//
vec3 opElongate(vec3 position, vec3 stretch) {
    return position - clamp(position, -stretch, stretch);
}


// =============================================================================
// TWIST (Rotation along axis)
// =============================================================================
// Twists a shape around an axis
//
// Parameters:
//   position - Point in 3D space
//   amount   - Twist amount (higher = more twist)
//
// Returns: Twisted position
//
// Visual (twist around Y axis):
//    ___             ___
//   /   \           / | \
//  | ||| |   ->    | / \ |   <- twisted/rotated
//   \___/           \___/
//
vec3 opTwistY(vec3 position, float amount) {
    float angle = amount * position.y;
    float c = cos(angle);
    float s = sin(angle);
    mat2 rotation = mat2(c, -s, s, c);
    return vec3(rotation * position.xz, position.y);
}


// =============================================================================
// BENDING (Curve along axis)
// =============================================================================
// Bends a shape along an axis
//
// Parameters:
//   position - Point in 3D space
//   amount   - Bend amount
//
// Returns: Bent position
//
// Visual (bend along X):
//   |||||          /////
//   |||||    ->   /////    <- bent shape
//   |||||        /////
//
vec3 opBendX(vec3 position, float amount) {
    float c = cos(amount * position.x);
    float s = sin(amount * position.x);
    mat2 rotation = mat2(c, -s, s, c);
    return vec3(position.x, rotation * position.yz);
}


// =============================================================================
// Usage Examples:
// =============================================================================
//
// Example 1: Two overlapping spheres with smooth blend
//   float s1 = sdSphere(p - vec3(-0.8, 0, 0), 1.0);
//   float s2 = sdSphere(p - vec3(0.8, 0, 0), 1.0);
//   float result = opSmoothUnion(s1, s2, 0.5);
//
// Example 2: Sphere with cylindrical hole
//   float sphere = sdSphere(p, 2.0);
//   float cylinder = sdCylinderInfinite(p, 0.5);
//   float result = opSubtraction(sphere, cylinder);
//
// Example 3: Infinite grid of spheres
//   vec3 repeated = opRepeat(p, vec3(5.0));
//   float result = sdSphere(repeated, 1.0);
//
// Example 4: Mirrored shape (only model one quarter)
//   vec3 mirrored = opSymmetryAll(p);
//   float result = sdBox(mirrored - vec3(2,2,2), vec3(1,1,1));
//
// Example 5: Complex CSG object
//   float base = sdBox(p, vec3(2,0.5,2));
//   float hole1 = sdCylinder(p - vec3(1,0,0), 0.3);
//   float hole2 = sdCylinder(p - vec3(-1,0,0), 0.3);
//   float withHoles = opSubtraction(opSubtraction(base, hole1), hole2);
//   float decoration = sdSphere(p - vec3(0,1,0), 0.8);
//   float result = opSmoothUnion(withHoles, decoration, 0.2);
//
// =============================================================================


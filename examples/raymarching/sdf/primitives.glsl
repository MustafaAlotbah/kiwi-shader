// =============================================================================
// SDF Primitive Shapes Library
// =============================================================================
// Collection of Signed Distance Functions for 3D primitive shapes.
//
// SDF Definition:
//   A function that returns the shortest distance from a point to a surface.
//   - Negative value = point is INSIDE the shape
//   - Positive value = point is OUTSIDE the shape  
//   - Zero = point is exactly ON the surface
//
// Usage:
//   float dist = sdSphere(point - position, radius);
//   if (dist < 0.0) { /* inside sphere */ }
//
// All primitives assume the shape is centered at the origin (0,0,0).
// To position elsewhere, subtract the desired position from the input point:
//   sdSphere(point - vec3(2.0, 1.0, 3.0), 1.0)  // sphere at (2,1,3)
// =============================================================================


// =============================================================================
// SPHERE
// =============================================================================
// A perfect sphere centered at origin
//
// Parameters:
//   position - Point in 3D space to sample
//   radius   - Radius of the sphere
//
// Math: f(x,y,z) = sqrt(x² + y² + z²) - radius
//
// Visual:
//        y
//        |
//    .-"   "-.
//  .'         '.
// |      o      |  <- sphere (radius r)
//  '.         .'
//    '-.___..-'
//         |_____ x
//        /
//       z
//
float sdSphere(vec3 position, float radius) {
    return length(position) - radius;
}


// =============================================================================
// BOX (Rectangular Prism)
// =============================================================================
// An axis-aligned box (cube or rectangular prism) centered at origin
//
// Parameters:
//   position - Point in 3D space to sample
//   size     - Half-extents (width/2, height/2, depth/2)
//              Example: size = vec3(1,2,3) creates box 2×4×6 units
//
// Math: Uses the L-infinity norm (max of absolute coordinates)
//
// Visual:
//      +-------+
//     /|      /|
//    / |     / |
//   +-------+  |  <- box with size (sx, sy, sz)
//   |  +----+--+
//   | /     | /
//   |/      |/
//   +-------+
//
float sdBox(vec3 position, vec3 size) {
    // Distance from center to each face
    vec3 distanceToFace = abs(position) - size;
    
    // Outside distance + inside distance
    return length(max(distanceToFace, 0.0)) + min(max(distanceToFace.x, max(distanceToFace.y, distanceToFace.z)), 0.0);
}


// =============================================================================
// TORUS (Donut)
// =============================================================================
// A torus (donut shape) lying flat on the XZ plane, centered at origin
//
// Parameters:
//   position     - Point in 3D space to sample
//   majorRadius  - Distance from center to tube center (donut radius)
//   minorRadius  - Radius of the tube itself (tube thickness)
//
// Math: Distance to circle in XZ plane, then distance to that circle in 3D
//
// Visual (top view):
//         ___
//      .-'   '-.
//     /    o    \   <- torus (major radius R, minor radius r)
//     \         /       o = center
//      '-.___.-'
//
// Visual (side view):
//        (  )
//      --------  <- tube cross-section (minor radius r)
//
float sdTorus(vec3 position, float majorRadius, float minorRadius) {
    // Distance from center to point in XZ plane
    vec2 distanceXZ = vec2(length(position.xz) - majorRadius, position.y);
    return length(distanceXZ) - minorRadius;
}


// =============================================================================
// CYLINDER (Infinite)
// =============================================================================
// An infinite vertical cylinder aligned with Y axis, centered at origin
//
// Parameters:
//   position - Point in 3D space to sample
//   radius   - Radius of the cylinder
//
// Math: Distance to Y axis (ignores Y coordinate)
//
// Visual:
//       |     |
//       |     |
//    ---|-----|---
//       |  o  |    <- infinite cylinder (radius r)
//    ---|-----|---  o = center axis (Y axis)
//       |     |
//       |     |
//
float sdCylinderInfinite(vec3 position, float radius) {
    // Distance to the Y axis (vertical)
    return length(position.xz) - radius;
}


// =============================================================================
// CAPPED CYLINDER (Finite)
// =============================================================================
// A finite cylinder with flat caps, aligned with Y axis, centered at origin
//
// Parameters:
//   position - Point in 3D space to sample
//   height   - Total height of cylinder
//   radius   - Radius of the cylinder
//
// Visual:
//       +-----+  <- top cap
//       |     |
//       |  o  |  <- cylinder (height h, radius r)
//       |     |     o = center
//       +-----+  <- bottom cap
//
float sdCappedCylinder(vec3 position, float height, float radius) {
    // Distance to cylinder body and caps
    vec2 distance = abs(vec2(length(position.xz), position.y)) - vec2(radius, height);
    return min(max(distance.x, distance.y), 0.0) + length(max(distance, 0.0));
}


// =============================================================================
// CAPSULE (Pill Shape)
// =============================================================================
// A capsule (rounded cylinder) aligned with Y axis, centered at origin
//
// Parameters:
//   position - Point in 3D space to sample
//   height   - Distance between hemisphere centers
//   radius   - Radius of the hemispheres/tube
//
// Math: Distance to line segment, then subtract radius
//
// Visual:
//        ___
//       /   \   <- hemisphere (radius r)
//       |   |
//       |   |   <- cylinder body
//       |   |
//       \___/   <- hemisphere (radius r)
//
// Total height = height + 2*radius
//
float sdCapsule(vec3 position, float height, float radius) {
    // Clamp to line segment
    position.y = clamp(position.y, -height, height);
    return length(position) - radius;
}


// =============================================================================
// PLANE (Infinite)
// =============================================================================
// An infinite plane defined by a point and normal vector
//
// Parameters:
//   position - Point in 3D space to sample
//   normal   - Plane normal (must be normalized!)
//   distance - Distance from origin along normal
//
// Math: Dot product with normal
//
// Visual:
//              ^ normal (n)
//              |
//    ----------+---------- plane
//              |
//              | distance (d)
//              o origin
//
float sdPlane(vec3 position, vec3 normal, float distance) {
    return dot(position, normal) + distance;
}


// =============================================================================
// CONE
// =============================================================================
// An infinite cone pointing up along Y axis, centered at origin
//
// Parameters:
//   position - Point in 3D space to sample
//   angle    - Half-angle of cone in radians (tip angle / 2)
//                Example: angle = 0.5 creates ~57° cone
//
// Visual:
//           ^  <- tip (at origin)
//          /|\
//         / | \
//        /  |  \   <- cone with angle θ
//       /   o   \     o = origin (tip)
//      /    |    \
//     /____ | ____\
//           y
//
float sdCone(vec3 position, float angle) {
    // Cone opening ratio
    vec2 c = vec2(sin(angle), cos(angle));
    
    // Distance to cone surface
    float q = length(position.xz);
    return dot(c, vec2(q, -position.y));
}


// =============================================================================
// ELLIPSOID
// =============================================================================
// An ellipsoid (stretched sphere) centered at origin
//
// Parameters:
//   position - Point in 3D space to sample
//   radius   - Radii along each axis (rx, ry, rz)
//
// Math: Normalized distance field (approximation)
//
// Visual:
//        ___
//      .'   '.
//     /   o   \  <- ellipsoid stretched along axes
//     \       /     o = center
//      '.___.'
//
float sdEllipsoid(vec3 position, vec3 radius) {
    // Normalize by radii
    vec3 p = position / radius;
    float k0 = length(p);
    float k1 = length(p / radius);
    return k0 * (k0 - 1.0) / k1;
}


// =============================================================================
// OCTAHEDRON
// =============================================================================
// A regular octahedron (8 faces) centered at origin
//
// Parameters:
//   position - Point in 3D space to sample
//   size     - Distance from center to vertex
//
// Visual:
//         ^
//        /|\
//       / | \
//      +--o--+   <- octahedron (8 triangular faces)
//       \ | /       o = center
//        \|/
//         v
//
float sdOctahedron(vec3 position, float size) {
    position = abs(position);
    float m = position.x + position.y + position.z - size;
    
    vec3 q;
    if (3.0 * position.x < m) {
        q = position.xyz;
    } else if (3.0 * position.y < m) {
        q = position.yzx;
    } else if (3.0 * position.z < m) {
        q = position.zxy;
    } else {
        return m * 0.57735027; // sqrt(1/3)
    }
    
    float k = clamp(0.5 * (q.z - q.y + size), 0.0, size);
    return length(vec3(q.x, q.y - size + k, q.z - k));
}


// =============================================================================
// Usage Examples:
// =============================================================================
//
// Example 1: Simple sphere at position
//   vec3 spherePos = vec3(0.0, 1.0, 5.0);
//   float dist = sdSphere(point - spherePos, 1.5);
//
// Example 2: Rotated box
//   mat3 rotation = rotationMatrix(angle);
//   vec3 localPoint = rotation * (point - boxPos);
//   float dist = sdBox(localPoint, vec3(1.0, 2.0, 0.5));
//
// Example 3: Multiple shapes in scene
//   float sphere = sdSphere(point - vec3(0,0,5), 1.0);
//   float box = sdBox(point - vec3(3,0,5), vec3(1,1,1));
//   float dist = min(sphere, box); // Closest surface
//
// =============================================================================


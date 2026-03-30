# NervSDK

A C++ procedural content generation (PCG) and rendering framework.

## Overview

NervSDK is a modern C++ framework providing:

- **PCG**: Procedural content generation for 2D planar graphs, path finding, intersection analysis
- **GLTF**: glTF 2.0 asset loading and scene graph
- **Math**: Vector, matrix, quaternion, geometry, and spline utilities
- **DX**: DirectX 11 and 12 engine backends
- **Task**: Promise-based async programming with cancellation support
- **Resource**: Unified resource loading and caching system
- **Log**: Multi-sink logging with thread support

## Core Modules

### Math

Mathematical primitives and utilities:

- **Vectors**: Vec2, Vec3, Vec4 with full operator overloads
- **Matrices**: Mat2, Mat3, Mat4 for transformations
- **Quaternion**: Rotation representation
- **Geometry**: Box2, Box3, Box4 bounding volumes
- **Spline**: Cubic spline interpolation (2D/3D)
- **SDF**: Signed distance field utilities
- **Range**: Generic range type

### PCG (Procedural Content Generation)

Complete PCG system for 2D planar graph analysis and path manipulation.

#### Core Types

- **PointArray**: Container for points with typed attributes
- **PointAttribute**: Typed data storage (Vec3d, F32, I32, etc.)
- **PCGPoint**: Value-type point (independent copy)
- **PCGPointRef**: Reference-type point (modifies underlying array)
- **WeightedPoint**: Point with weight for averaging operations
- **PCGContext**: ExecutionContext with input/output slots

#### Built-in Attributes

| Attribute | Type | Description |
|-----------|------|-------------|
| `$Index` | I32 | Point index |
| `$Position` | Vec3d | 3D position |
| `$Rotation` | Vec3d | Rotation Euler |
| `$Scale` | Vec3d | Scale factor |
| `$Color` | Vec4d | RGBA color |
| `$Density` | F64 | Density value |
| `$Steepness` | F64 | Slope steepness |
| `$BoundsMin` | Vec3d | Bounding min |
| `$BoundsMax` | Vec3d | Bounding max |

#### Operations

| Function | Description |
|----------|-------------|
| `pcg_set_data_id(ctx)` | Assign unique ID attribute to each path |
| `pcg_find_path_2d_intersections(ctx)` | Find path crossing points |
| `pcg_build_intersection_contours(ctx)` | Generate closed loops at intersections |
| `pcg_resample_paths(ctx)` | Resample at equal intervals |
| `pcg_compute_path_offsets(ctx)` | Generate parallel offset paths |

#### PCG Usage Examples

**Creating a PointArray with custom attributes:**
```cpp
#include <nvk_pcg.h>

using namespace nv;

// Create a square path with positions
auto pos = PointAttribute::create<Vec3d>(pt_position_attr, {
    {0.0, 0.0, 0.0},
    {1.0, 0.0, 0.0},
    {1.0, 1.0, 0.0},
    {0.0, 1.0, 0.0},
});

auto color = PointAttribute::create<Vec4d>("$Color", {
    {1.0, 0.0, 0.0, 1.0},
    {0.0, 1.0, 0.0, 1.0},
    {0.0, 0.0, 1.0, 1.0},
    {1.0, 1.0, 0.0, 1.0},
});

auto points = PointArray::create({pos, color});
points->set_closed_loop(true);
```

**Accessing and modifying points:**
```cpp
// Get a point reference (modifies underlying array)
auto pointRef = points->get_point(0);
Vec3d pos = pointRef.position();  // Get position
pointRef.set_position(Vec3d(2.0, 0.0, 0.0));

// Get a point copy (independent)
PCGPoint pointCopy = points->copy_point(0);
pointCopy.set_position(Vec3d(3.0, 0.0, 0.0));  // Doesn't affect array

// Access attributes by type
const auto& positions = points->get<Vec3d>(pt_position_attr);
const auto& colors = points->get<Vec4d>("$Color");
```

**PCG Context workflow:**
```cpp
auto ctx = PCGContext::create();
auto& in = ctx->inputs();

// Add multiple paths as input
PointArrayVector paths;
paths.push_back(points);
in.set("In", paths);

// Set operation parameters
in.set("Distance", 0.5);  // Offset distance

// Execute operation
pcg_compute_path_offsets(*ctx);

// Get results
PointArrayVector outputs = ctx->outputs().get("Out");
for (const auto& contour : outputs) {
    const auto& positions = contour->get<Vec3d>(pt_position_attr);
    // Process offset contour...
}
```

**Weighted averaging of point attributes:**
```cpp
Vector<WeightedPoint> weighted;
weighted.emplace_back(points->get_point(0), 0.7);
weighted.emplace_back(points->get_point(1), 0.3);

PCGPoint averaged;
averaged.set_weighted_average(weighted, {"$Position"});
```

**Resampling paths:**
```cpp
in.set("Resolution", 0.1);  // Target segment length
pcg_resample_paths(*ctx);
```

**Intersection contour generation:**
```cpp
// Create intersecting paths (forming a cross)
auto pos1 = PointAttribute::create<Vec3d>(pt_position_attr, {
    {0.0, 0.0, 0.0}, {2.0, 0.0, 0.0}, {2.0, 1.0, 0.0},
    {1.0, 1.0, 0.0}, {1.0, -1.0, 0.0},
});
auto paths = PointArrayVector{PointArray::create({pos1})};

auto ctx = PCGContext::create();
ctx->inputs().set("In", paths);

pcg_find_path_2d_intersections(*ctx);
pcg_build_intersection_contours(*ctx);

auto contours = ctx->outputs().get("Out");
// Contours contain closed loops at intersection points
```

### GLTF

Full glTF 2.0 loader with scene graph construction:

- **GLTFAsset**: Top-level document container
- **GLTFScene**: Hierarchical scene root
- **GLTFNode**: Transform node with children/pivot
- **GLTFMesh**: Mesh with multiple primitives
- **GLTFPrimitive**: Submesh with attributes/material
- **GLTFMaterial**: PBR material properties
- **GLTFTexture/GLTFImage**: Texture chain
- **GLTFBuffer/BufferView**: Binary data storage
- **GLTFAccessor**: Typed element access into buffers
- **GLTFAnimation**: Keyframe animation tracks
- **GLTFSkin**: Skeletal rig data
- **GLTFCamera**: Perspective/orthographic camera

**Features**:
- Position, normal, tangent, UV, color, joint, weight attributes
- PBR workflow with all standard textures
- Morph targets support
- Skeleton skinning
- Custom extensions

### Task System

Promise-based asynchronous programming:

- **Promise<T>**: Future value container
- **Defer**: Deferred resolution helper
- **CancellationToken**: Cooperative cancellation
- **JobDispatcher**: Thread pool task scheduling

**Combinators**:
- `promise_all()` - Wait for all promises to resolve
- `promise_all_settled()` - Get all results (success or failure)
- `promise_race()` - First promise to settle
- `then()`, `catch_error()`, `finally()` - Chaining

**Cancellation**: Attach tokens to propagate cancellation through chains.

### Resource System

Unified resource management:

- **ResourceManager**: Global registry with lookup by type/string ID
- **ResourceProvider**: Abstract provider interface
- **ResourcePacker**: Aggregate multiple providers
- **ResourceLoader**: .nvrx archive format support

### Logging

Flexible logging system:

- **LogManager**: Singleton access
- **LogSink**: Base interface for backends
- **StdLogger**: Console output with ANSI colors
- **FileLogger**: File output with rotation

**Levels**: trace, debug, info, warn, error, fatal

### DX (DirectX Engine)

Graphics engine implementations:

- **DX11Engine**: DirectX 11 backend
- **DX12Engine**: DirectX 12 backend

## Types

Core type definitions:

| Type | Description |
|------|-------------|
| Bool | bool |
| Byte | unsigned char |
| I8/U8 | int8_t / uint8_t |
| I16/U16 | int16_t / uint16_t |
| I32/U32 | int32_t / uint32_t |
| I64/U64 | int64_t / uint64_t |
| F16 | half_float::half |
| F32 | float |
| F64 | double |
| StringID | FNV-1a hash of string (compile-time constant) |

String ID macros:
```cpp
#define SID(str) nv::str_id_const(str)  // Compile-time hash
auto id = my_string"_sid;             // User literal
```

## Data Structures

- **Vector<T>**: Resizable array with custom allocator
- **Set<T>**: Unique sorted collection
- **Map<K,V>**: Sorted associative container
- **SlotMap<K,T>**: Stable container with slot invalidation
- **WeakPtr<T>**/**WeakRefObject**: Non-owning references

## Dependencies

External libraries:

- **fmt** - Formatting library
- **nlohmann/json** - JSON parsing
- **half** - IEEE 754 half-precision floats
- **STB** - stb_image, stb_image_resize, stb_image_write
- **cgltf** - Minimal glTF 2.0 parser
- **RTree** - Spatial indexing
- **entt** - ECS framework (optional)

## PCG Quick Example

```cpp
#include <nvk_pcg.h>

using namespace nv;

// Create a closed-loop square path
auto pos = PointAttribute::create<Vec3d>(pt_position_attr, {
    {0.0, 0.0, 0.0},
    {2.0, 0.0, 0.0},
    {2.0, 2.0, 0.0},
    {0.0, 2.0, 0.0},
});
auto points = PointArray::create({pos});
points->set_closed_loop(true);

// Add custom attribute
auto density = points->add_attribute<F64>("$Density", 1.0);
density[0] = 0.5; density[1] = 0.7; density[2] = 0.9; density[3] = 0.3;

// Set up PCG context
auto ctx = PCGContext::create();
ctx->inputs().set("In", PointArrayVector{points});
ctx->inputs().set("Distance", 0.25);

// Compute offsets
pcg_compute_path_offsets(*ctx);
auto contours = ctx->outputs().get("Out");

// Process result
for (const auto& contour : contours) {
    F64 area = contour->compute_area();
    // contour is a closed loop of offset positions
}
```

## Build

```bash
cmake -S . -B build -DCMAKE_BUILD_TYPE=Release
cmake --build build
```

Requirements:
- C++17 compatible compiler
- CMake 3.15+

## License

MIT License

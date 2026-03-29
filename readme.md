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

Complete PCG system for planar graph generation:

- **PCGContext**: Main context holding the planar graph
- **PCGGraph**: Planar graph with nodes and edges
- **PCGNode**: Graph nodes with position, data, and connectivity
- **PCGPoint**: Point coordinates and attributes
- **PCGPointRef**: Reference to points in the array
- **PointArray**: Storage container with fast indexing
- **PointAttribute**: Custom typed data per point

**Operations**:
- `pcg_set_data_id()` - Assign unique IDs to paths
- `pcg_find_path_2d_intersections()` - Detect crossing points
- `pcg_build_intersection_contours()` - Generate closed loops
- `pcg_resample_paths()` - Equal-interval resampling
- `pcg_compute_path_offsets()` - Perpendicular offset generation

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

## Usage Example

```cpp
#include <nvk_base.h>
#include <nvk_math.h>
#include <nvk/pcg/PCGContext.h>
#include <nvk/gltf/Asset.h>

using namespace nv;

// Math
Vec3f pos(1.0f, 2.0f, 3.0f);
Vec3f normalized = pos.normalized();

// PCG
PCGContext ctx;
ctx.add_node(0, Vec2f(0, 0));
ctx.add_node(1, Vec2f(1, 1));
ctx.add_edge(0, 1);
pcg_set_data_id(ctx);

// GLTF
GLTFAsset asset = GLTFAsset::load("model.gltf");
GLTFScene* scene = asset.getScene(0);

// Task
auto promise = make_promise([](Defer d) {
    // async work
    d.resolve(42);
});

promise.then([](const Any& value) {
    return value.get<int>();
});
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

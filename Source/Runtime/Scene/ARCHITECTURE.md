# Scene Subsystem Architecture

> **Priority 9** | Est. 2,000+ files | 200K+ LOC | ⭐⭐⭐ Complexity

---

## Directory Structure

```
Scene/
├── Private/
├── Public/
│
├── ECS/                        # Entity Component System
│   ├── Entity/
│   │   ├── Handle/
│   │   ├── Pool/
│   │   └── Generation/
│   ├── Component/
│   │   ├── Storage/
│   │   │   ├── Dense/
│   │   │   ├── Sparse/
│   │   │   └── Tag/
│   │   ├── Registry/
│   │   └── Reflection/
│   ├── Archetype/
│   │   ├── Storage/
│   │   ├── Migration/
│   │   └── Query/
│   ├── System/
│   │   ├── Scheduler/
│   │   ├── Dependency/
│   │   └── Phases/
│   └── Query/
│       ├── Builder/
│       ├── Filter/
│       └── Cache/
│
├── Hierarchy/                  # Scene hierarchy
│   ├── Transform/
│   │   ├── Local/
│   │   ├── World/
│   │   └── Dirty/
│   ├── Parent/
│   ├── Children/
│   └── Order/
│
├── Graph/                      # Scene graph
│   ├── Node/
│   ├── Traversal/
│   ├── Update/
│   └── Events/
│
├── Streaming/                  # Level streaming
│   ├── Chunks/
│   ├── Loading/
│   ├── Unloading/
│   └── Seamless/
│
├── Octree/                     # Spatial partitioning
│   ├── Build/
│   ├── Query/
│   └── Dynamic/
│
├── Visibility/                 # Visibility system
│   ├── Culling/
│   ├── Occlusion/
│   └── LOD/
│
└── Serialization/              # Scene serialization
    ├── Binary/
    ├── JSON/
    ├── Prefab/
    └── Instantiate/
```

---

## Current Code Mapping

| New Location | Current Location | Files |
|--------------|------------------|-------|
| Scene/ECS/ | src/Runtime/ECS/ | ~17 |
| Scene/Hierarchy/ | src/Runtime/Scene/ | ~20 |

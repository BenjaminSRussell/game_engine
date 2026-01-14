# Core Subsystem Architecture

> **Priority 6** | Est. 3,000+ files | 300K+ LOC | ⭐⭐⭐ Complexity

The foundation layer with zero external dependencies (except platform APIs).

---

## Directory Structure

```
Core/
├── Private/
├── Public/
│
├── Platform/                   # Platform abstraction
│   ├── Windows/
│   │   ├── Memory/
│   │   ├── Threading/
│   │   ├── FileSystem/
│   │   └── Window/
│   ├── macOS/
│   │   ├── Memory/
│   │   ├── Threading/
│   │   ├── FileSystem/
│   │   └── Window/
│   ├── Linux/
│   │   ├── Memory/
│   │   ├── Threading/
│   │   ├── FileSystem/
│   │   └── Window/
│   ├── iOS/
│   └── Android/
│
├── Memory/                     # Memory management
│   ├── Allocators/
│   │   ├── System/
│   │   ├── Linear/
│   │   ├── Stack/
│   │   ├── Pool/
│   │   ├── Buddy/
│   │   ├── Slab/
│   │   └── TLSF/
│   ├── Tracking/
│   │   ├── Leaks/
│   │   ├── Corruption/
│   │   └── Statistics/
│   └── Virtual/
│       ├── Reserve/
│       ├── Commit/
│       └── Pages/
│
├── Math/                       # Math library
│   ├── Vector/
│   │   ├── Vec2/
│   │   ├── Vec3/
│   │   ├── Vec4/
│   │   └── SIMD/
│   ├── Matrix/
│   │   ├── Mat3/
│   │   ├── Mat4/
│   │   └── Transform/
│   ├── Quaternion/
│   ├── Geometry/
│   │   ├── AABB/
│   │   ├── OBB/
│   │   ├── Sphere/
│   │   ├── Plane/
│   │   ├── Frustum/
│   │   └── Ray/
│   ├── Random/
│   │   ├── PCG/
│   │   ├── Mersenne/
│   │   └── Noise/
│   └── Interpolation/
│       ├── Lerp/
│       ├── Slerp/
│       ├── Bezier/
│       └── Spline/
│
├── Threading/                  # Threading primitives
│   ├── Thread/
│   ├── Mutex/
│   │   ├── Mutex/
│   │   ├── SpinLock/
│   │   └── RWLock/
│   ├── Atomic/
│   ├── Semaphore/
│   ├── ConditionVar/
│   ├── ThreadLocal/
│   ├── Fiber/
│   └── ThreadPool/
│
├── Jobs/                       # Job system
│   ├── Scheduler/
│   ├── Queue/
│   │   ├── WorkStealing/
│   │   └── MPMC/
│   ├── Graph/
│   ├── Parallel/
│   │   ├── For/
│   │   ├── Sort/
│   │   └── Reduce/
│   └── Async/
│
├── Containers/                 # Data structures
│   ├── Array/
│   │   ├── Dynamic/
│   │   ├── Small/
│   │   └── Span/
│   ├── HashMap/
│   │   ├── Open/
│   │   ├── Robin/
│   │   └── Swiss/
│   ├── Set/
│   ├── Queue/
│   │   ├── FIFO/
│   │   ├── Priority/
│   │   └── Ring/
│   ├── Tree/
│   │   ├── BST/
│   │   ├── RBTree/
│   │   └── BTree/
│   ├── Graph/
│   ├── Pool/
│   │   ├── Object/
│   │   ├── Slot/
│   │   └── Handle/
│   └── Bitset/
│
├── String/                     # String utilities
│   ├── String/
│   ├── StringBuilder/
│   ├── StringView/
│   ├── Formatting/
│   ├── Parsing/
│   └── Unicode/
│
├── Logging/                    # Logging system
│   ├── Logger/
│   ├── Sinks/
│   │   ├── Console/
│   │   ├── File/
│   │   └── Network/
│   ├── Formatting/
│   └── Categories/
│
├── Serialization/              # Serialization
│   ├── Binary/
│   ├── JSON/
│   ├── Archive/
│   └── Versioning/
│
├── Config/                     # Configuration
│   ├── INI/
│   ├── TOML/
│   ├── CommandLine/
│   └── CVars/
│
├── Time/                       # Time utilities
│   ├── Clock/
│   ├── Timer/
│   ├── Stopwatch/
│   └── DateTime/
│
├── Profiling/                  # Profiling
│   ├── CPU/
│   ├── Memory/
│   ├── Scopes/
│   └── Remote/
│
├── Error/                      # Error handling
│   ├── Assert/
│   ├── Result/
│   ├── Exception/
│   └── StackTrace/
│
└── Delegates/                  # Callbacks
    ├── Delegate/
    ├── MulticastDelegate/
    └── Event/
```

---

## Current Code Mapping

| New Location | Current Location | Files |
|--------------|------------------|-------|
| Core/Memory/ | src/core/memory/ | ~20 |
| Core/Math/ | src/core/math/ | ~35 |
| Core/Threading/ | src/core/sync/ | ~30 |
| Core/Containers/ | src/core/containers/ | ~30 |
| Core/Logging/ | src/core/logging/ | ~10 |
| Core/Platform/ | src/core/platform/ | ~120 |

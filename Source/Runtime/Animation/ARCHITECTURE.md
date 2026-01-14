# Animation Subsystem Architecture

> **Priority 4** | Est. 4,000+ files | 400K+ LOC | ⭐⭐⭐⭐ Complexity

---

## Directory Structure

```
Animation/
├── Private/
├── Public/
│
├── Core/                       # Animation core
│   ├── Time/
│   ├── Curves/
│   │   ├── Bezier/
│   │   ├── Hermite/
│   │   └── Linear/
│   └── Events/
│
├── Skeletal/                   # Skeletal animation
│   ├── Skeleton/
│   │   ├── Bones/
│   │   ├── Sockets/
│   │   └── LOD/
│   ├── Pose/
│   │   ├── Local/
│   │   ├── Global/
│   │   └── Retarget/
│   ├── Clip/
│   │   ├── Sampling/
│   │   ├── Compression/
│   │   └── Streaming/
│   ├── Blending/
│   │   ├── Lerp/
│   │   ├── Additive/
│   │   ├── Layered/
│   │   └── Masked/
│   └── Montage/
│
├── IK/                         # Inverse Kinematics
│   ├── FABRIK/
│   ├── CCD/
│   ├── TwoBone/
│   ├── LookAt/
│   ├── FootIK/
│   │   ├── Ground/
│   │   ├── Slope/
│   │   └── Steps/
│   └── FullBody/
│
├── StateMachine/               # Animation state machines
│   ├── States/
│   ├── Transitions/
│   │   ├── Blend/
│   │   ├── Crossfade/
│   │   └── Sync/
│   ├── Conditions/
│   └── Layers/
│
├── BlendSpace/                 # Blend spaces
│   ├── 1D/
│   ├── 2D/
│   └── Additive/
│
├── Procedural/                 # Procedural animation
│   ├── SpringBones/
│   ├── Jiggle/
│   ├── Twist/
│   └── Aim/
│
├── Physics/                    # Physics animation
│   ├── Ragdoll/
│   ├── Powered/
│   └── Blend/
│
├── MotionMatching/             # Motion matching
│   ├── Database/
│   ├── Features/
│   ├── Search/
│   └── Trajectory/
│
├── Morph/                      # Morph targets
│   ├── Targets/
│   ├── Blending/
│   └── Streaming/
│
└── Compression/                # Animation compression
    ├── Quantization/
    ├── KeyReduction/
    └── ACL/                    # Animation Compression Library
```

---

## Current Code Mapping

| New Location | Current Location | Files |
|--------------|------------------|-------|
| Animation/Skeletal/ | src/Runtime/Animation/Skeletal/ | ~50 |
| Animation/IK/ | src/Runtime/Animation/IK/ | ~20 |
| Animation/StateMachine/ | src/Runtime/Animation/StateMachine/ | ~10 |

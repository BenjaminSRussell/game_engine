# Runtime Subsystems Architecture

> Core engine systems required for runtime execution (no editor dependencies)

---

## Module Hierarchy

```
Runtime/
├── Core/           # Foundation layer (no deps except platform)
├── RHI/            # Hardware abstraction (depends: Core)
├── Renderer/       # High-level rendering (depends: RHI, Core)
├── Physics/        # Simulation (depends: Core)
├── Animation/      # Skeletal & morphs (depends: Core)
├── Audio/          # Sound systems (depends: Core)
├── AI/             # Intelligence (depends: Core, Navigation)
├── Networking/     # Network layer (depends: Core)
├── Scene/          # World management (depends: Core)
└── UI/             # User interface (depends: Core, Renderer)
```

---

## Dependency Graph (Build Order)

```
Level 0: Core/Platform
    ↓
Level 1: Core/Math, Core/Memory, Core/Threading
    ↓
Level 2: Core/Containers, Core/Serialization
    ↓
Level 3: RHI (Vulkan, Metal, D3D12)
    ↓
Level 4: Renderer, Physics, Audio, Animation, AI, Networking
    ↓
Level 5: Scene, UI
```

---

## Subdocuments

| Module | Documentation | Est. Files |
|--------|---------------|------------|
| [Core](Core/ARCHITECTURE.md) | Foundation systems | 3,000+ |
| [RHI](RHI/ARCHITECTURE.md) | Graphics abstraction | 5,000+ |
| [Renderer](Renderer/ARCHITECTURE.md) | Rendering pipeline | 12,000+ |
| [Physics](Physics/ARCHITECTURE.md) | Physical simulation | 8,000+ |
| [Animation](Animation/ARCHITECTURE.md) | Animation systems | 4,000+ |
| [Audio](Audio/ARCHITECTURE.md) | Audio engine | 2,500+ |
| [AI](AI/ARCHITECTURE.md) | AI systems | 4,000+ |
| [Networking](Networking/ARCHITECTURE.md) | Network layer | 2,500+ |
| [Scene](Scene/ARCHITECTURE.md) | Scene management | 2,000+ |
| [UI](UI/ARCHITECTURE.md) | UI framework | 2,000+ |

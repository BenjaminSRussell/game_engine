# AI Subsystem Architecture

> **Priority 5** | Est. 4,000+ files | 400K+ LOC | ⭐⭐⭐⭐ Complexity

---

## Directory Structure

```
AI/
├── Private/
├── Public/
│
├── Core/                       # AI core systems
│   ├── Controller/
│   ├── Blackboard/
│   └── World/
│
├── BehaviorTree/               # Behavior trees
│   ├── Tree/
│   │   ├── Parser/
│   │   ├── Compiler/
│   │   └── Instance/
│   ├── Nodes/
│   │   ├── Composite/
│   │   │   ├── Selector/
│   │   │   ├── Sequence/
│   │   │   └── Parallel/
│   │   ├── Decorator/
│   │   │   ├── Repeater/
│   │   │   ├── Inverter/
│   │   │   └── Cooldown/
│   │   ├── Task/
│   │   │   ├── MoveTo/
│   │   │   ├── Wait/
│   │   │   └── Custom/
│   │   └── Service/
│   └── Execution/
│
├── Navigation/                 # Navigation system
│   ├── NavMesh/
│   │   ├── Generation/
│   │   ├── Tiles/
│   │   ├── Obstacles/
│   │   └── Links/
│   ├── Pathfinding/
│   │   ├── AStar/
│   │   ├── JPS/
│   │   ├── HPA/
│   │   └── Theta/
│   ├── Steering/
│   │   ├── Seek/
│   │   ├── Flee/
│   │   ├── Arrive/
│   │   ├── Wander/
│   │   └── Pursue/
│   ├── Avoidance/
│   │   ├── RVO/
│   │   ├── ORCA/
│   │   └── Local/
│   └── Crowd/
│       ├── Simulation/
│       ├── Flow/
│       └── Groups/
│
├── Perception/                 # Perception system
│   ├── Sight/
│   │   ├── Raycast/
│   │   ├── Cone/
│   │   └── LOS/
│   ├── Hearing/
│   ├── Damage/
│   ├── Team/
│   └── Memory/
│
├── GOAP/                       # Goal-Oriented Action Planning
│   ├── Goals/
│   ├── Actions/
│   ├── Planner/
│   └── WorldState/
│
├── UtilityAI/                  # Utility AI
│   ├── Considerations/
│   ├── Curves/
│   ├── Actions/
│   └── Reasoner/
│
├── HTN/                        # Hierarchical Task Network
│   ├── Domain/
│   ├── Tasks/
│   ├── Methods/
│   └── Planner/
│
├── ML/                         # Machine Learning
│   ├── Inference/
│   │   ├── ONNX/
│   │   ├── CoreML/
│   │   └── TensorRT/
│   ├── Training/
│   ├── Imitation/
│   └── Reinforcement/
│
├── Dialog/                     # Dialog system
│   ├── Graph/
│   ├── Conditions/
│   └── Localization/
│
└── Debug/
    ├── Visualization/
    └── Recording/
```

---

## Current Code Mapping

| New Location | Current Location | Files |
|--------------|------------------|-------|
| AI/BehaviorTree/ | src/Runtime/AI/BehaviorTree/ | ~60 |
| AI/Navigation/ | src/Runtime/AI/Navigation/ | ~50 |
| AI/Perception/ | src/Runtime/AI/Perception/ | ~20 |
| AI/GOAP/ | src/Runtime/AI/GOAP/ | ~15 |

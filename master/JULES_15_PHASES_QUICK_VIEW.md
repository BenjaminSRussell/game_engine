# Jules 15-Phase System - Quick View

**All 15 phases, one per agent, at a glance.**

---

## Critical Path (Sequential)
```
Phase 1: Engine-Core
    ↓
Phase 2: Rendering  
    ↓
Phase 3: Physics
    ↓
Phases 4-14: ALL CAN RUN IN PARALLEL
    ↓
Phase 15: Tools (Final)
```

---

## Phase Breakdown

### TIER 1: Foundation (Must be sequential)

#### Phase 1: Jules-Engine-Core
- **Files:** 198 (src/engine/core/)
- **Tasks:** 6 (CORE-001 through CORE-006)
- **Duration:** Weeks 1-2
- **Success:** Engine initializes, runs 10 frames clean, zero memory leaks
- **Read:** `master/JULIUS_AGENT_TASK_PROMPTS.md` → Agent 1

#### Phase 2: Jules-Rendering
- **Files:** 361 (src/engine/rendering/)
- **Tasks:** 7 (RENDER-001 through RENDER-007)
- **Duration:** Weeks 3-4 (after Phase 1)
- **Success:** Triangle renders at 60 FPS on all platforms
- **Read:** `master/JULIUS_AGENT_TASK_PROMPTS.md` → Agent 2

#### Phase 3: Jules-Physics
- **Files:** 224 (src/engine/physics/)
- **Tasks:** 6 (PHYS-001 through PHYS-006)
- **Duration:** Weeks 5-6 (after Phase 2)
- **Success:** 1000+ physics objects at 60 FPS
- **Read:** `master/JULIUS_AGENT_TASK_PROMPTS.md` → Agent 3

---

### TIER 2: Parallel Execution (Start after Phase 3)

#### Phase 4: Jules-Animation
- **Files:** 55
- **Tasks:** 5 (ANIM-001 through ANIM-005)
- **Duration:** Weeks 7-8
- **Depends On:** Phase 3
- **Read:** `master/JULIUS_AGENT_TASK_PROMPTS.md` → Agent 4

#### Phase 5: Jules-Audio
- **Files:** 67
- **Tasks:** 4 (AUDIO-001 through AUDIO-004)
- **Duration:** Weeks 9-10
- **Depends On:** Phase 3
- **Read:** `master/JULIUS_AGENT_TASK_PROMPTS.md` → Agent 5

#### Phase 6: Jules-AI
- **Files:** 108
- **Tasks:** 4 (AI-001 through AI-004)
- **Duration:** Weeks 11-12
- **Depends On:** Phase 3
- **Read:** `master/JULIUS_AGENT_TASK_PROMPTS.md` → Agent 6

#### Phase 7: Jules-Assets
- **Files:** 140
- **Tasks:** 4 (ASSET-001 through ASSET-004)
- **Duration:** Weeks 13-14
- **Depends On:** Phase 3
- **Read:** `master/JULIUS_AGENT_TASK_PROMPTS.md` → Agent 7

#### Phase 8: Jules-Networking
- **Files:** 44
- **Tasks:** 4 (NET-001 through NET-004)
- **Duration:** Weeks 15-16
- **Depends On:** Phase 3
- **Read:** `master/JULIUS_AGENT_TASK_PROMPTS.md` → Agent 8

#### Phase 9: Jules-Gameplay
- **Files:** 22
- **Tasks:** 5 (GAME-001 through GAME-005)
- **Duration:** Weeks 17-18
- **Depends On:** Phase 3
- **Read:** `master/JULIUS_AGENT_TASK_PROMPTS.md` → Agent 9

#### Phase 10: Jules-Character
- **Files:** 130
- **Tasks:** 4 (CHAR-001 through CHAR-004)
- **Duration:** Weeks 19-20
- **Depends On:** Phase 4 (Animation)
- **Read:** `master/JULIUS_AGENT_TASK_PROMPTS.md` → Agent 10

#### Phase 11: Jules-Geometry
- **Files:** 134
- **Tasks:** 4 (GEO-001 through GEO-004)
- **Duration:** Weeks 21-22
- **Depends On:** Phase 3
- **Read:** `master/JULIUS_AGENT_TASK_PROMPTS.md` → Agent 11

#### Phase 12: Jules-Materials
- **Files:** 89
- **Tasks:** 4 (MAT-001 through MAT-004)
- **Duration:** Weeks 23-24
- **Depends On:** Phase 2
- **Read:** `master/JULIUS_AGENT_TASK_PROMPTS.md` → Agent 12

#### Phase 13: Jules-Lighting
- **Files:** 131
- **Tasks:** 4 (LIGHT-001 through LIGHT-004)
- **Duration:** Weeks 25-26
- **Depends On:** Phase 2
- **Read:** `master/JULIUS_AGENT_TASK_PROMPTS.md` → Agent 13

#### Phase 14: Jules-Effects
- **Files:** 79
- **Tasks:** 4 (FX-001 through FX-004)
- **Duration:** Weeks 27-28
- **Depends On:** Phase 2
- **Read:** `master/JULIUS_AGENT_TASK_PROMPTS.md` → Agent 14

---

### TIER 4: Final (Start only after all phases 1-14)

#### Phase 15: Jules-Tools
- **Files:** 363 (src/engine/editor/, src/engine/tools/, src/engine/debug/)
- **Tasks:** 6 (TOOLS-001 through TOOLS-006)
- **Duration:** Weeks 29-30+
- **Depends On:** ALL phases 1-14
- **Success:** Full game playable in editor
- **Read:** `master/JULIUS_AGENT_TASK_PROMPTS.md` → Agent 15

---

## Summary Table

| Phase | Agent | Tasks | Start Week | Files | Parallel? |
|-------|-------|-------|-----------|-------|-----------|
| 1 | Engine-Core | 6 | 1 | 198 | ❌ |
| 2 | Rendering | 7 | 3 | 361 | ❌ |
| 3 | Physics | 6 | 5 | 224 | ❌ |
| 4 | Animation | 5 | 7 | 55 | ✅ |
| 5 | Audio | 4 | 9 | 67 | ✅ |
| 6 | AI | 4 | 11 | 108 | ✅ |
| 7 | Assets | 4 | 13 | 140 | ✅ |
| 8 | Networking | 4 | 15 | 44 | ✅ |
| 9 | Gameplay | 5 | 17 | 22 | ✅ |
| 10 | Character | 4 | 19 | 130 | ✅ |
| 11 | Geometry | 4 | 21 | 134 | ✅ |
| 12 | Materials | 4 | 23 | 89 | ✅ |
| 13 | Lighting | 4 | 25 | 131 | ✅ |
| 14 | Effects | 4 | 27 | 79 | ✅ |
| 15 | Tools | 6 | 29 | 363 | ❌ |

**Total:** 65 tasks, 2,385 files, 30+ weeks

---

## Where to Find Everything

### Start Here
👉 `master/JULES_15_PHASE_SYSTEM.md` - Complete phase documentation

### Your Phase Details
👉 `master/JULIUS_AGENT_TASK_PROMPTS.md` - Specific task prompts for your agent

### Task Tracking
👉 `master/todo.csv` - Live status of all 65 tasks

### Full Agent Info
👉 `master/JULIUS_15_AGENTS_ASSIGNMENTS.md` - Deep specs for all agents

### Quick Lookup
👉 `JULIUS_15_QUICK_REFERENCE.md` - 1-page agent matrix

---

## How to Use This Document

**For your phase:** Find it above, read "Read: ..." line to know where task prompts are

**For status:** Check `master/todo.csv` for your phase tasks (e.g., grep "^ANIM-" for Phase 4)

**For blocking:** See "Depends On" line to understand prerequisites

**For execution:** Go to referenced document, find your agent, read task prompts

---

**Ready to execute. Phase 1 starts now.**

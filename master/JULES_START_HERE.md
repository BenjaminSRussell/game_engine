# 🚀 Jules Agent Organization - START HERE

**Last Updated:** 2026-01-12  
**Status:** Ready for assignment and execution

---

## What Is This?

You now have **15 specialized Jules agents**, each owning a distinct engine subsystem. This is the largest coordinated effort for the Minecraft v2 engine.

**Total Scope:** 2,385 files, 65+ critical tasks, ~300 hours of implementation work

---

## Three Key Documents

### 1. **JULES_15_AGENTS_ASSIGNMENTS.md** (Detailed)
📄 `master/JULES_15_AGENTS_ASSIGNMENTS.md` - 30KB comprehensive guide

**Contains:**
- Full agent profiles (6-9 pages each)
- Detailed mission statements
- File ownership lists
- 4-6 critical tasks per agent with:
  - Specific file locations and line numbers
  - Current state analysis (what's broken)
  - Acceptance criteria (what done looks like)
  - Dependency relationships
- Complete dependency matrix
- 4-phase execution strategy (8+ weeks)
- Success criteria and checkpoints

**Use:** Reference when planning your agent's work in detail

---

### 2. **JULES_15_QUICK_REFERENCE.md** (Fast Lookup)
📄 `JULES_15_QUICK_REFERENCE.md` - 9.6KB at-a-glance guide

**Contains:**
- Agent matrix (all 15 at a glance)
- Directory ownership table
- Phase-by-phase timeline
- Success checkpoints
- Agent boundaries lookup
- Key rules (do's and don'ts)
- Example: How to start a task

**Use:** Quick answers, phase planning, ownership lookup

---

### 3. **master/todo.md** (Status Tracking)
📄 `master/todo.md` - Updated master TODO list

**Contains:**
- Checklist of all 15 agents
- Link to detailed assignments doc
- Integration with existing rules

**Use:** Daily status reference, see which agents active

---

## The 15 Agents Explained

### PHASE 1: Foundation (P0) - Weeks 1-2
These three agents build the core engine before anything else starts.

| Agent | Role | Files | Tasks | Goal |
|-------|------|-------|-------|------|
| **Jules-Engine-Core** | Engine backbone | 198 | 6 | Lifecycle, memory, threading working |
| **Jules-Rendering** | Graphics pipeline | 361 | 7 | Triangle on screen at 60 FPS |
| **Jules-Physics** | Physics simulation | 224 | 6 | 1000+ objects @ 60 FPS |

**Start here.** These must work before Phase 2.

---

### PHASE 2: Systems (P1) - Weeks 3-4
These five agents build major engine systems in parallel (once Phase 1 is solid).

| Agent | Role | Files | Tasks | Depends On |
|-------|------|-------|-------|-----------|
| **Jules-Animation** | Character motion | 55 | 5 | Engine-Core |
| **Jules-Audio** | Sound & music | 67 | 4 | Engine-Core |
| **Jules-AI** | NPC behavior | 108 | 4 | Physics |
| **Jules-Assets** | Content pipeline | 140 | 4 | Engine-Core |
| **Jules-Networking** | Multiplayer | 44 | 4 | Physics |

**Parallelize:** Can start independently after Phase 1 foundation solid.

---

### PHASE 3: Integration (P1) - Weeks 5-6
These six agents build content and polish layers in parallel.

| Agent | Role | Files | Tasks | Depends On |
|-------|------|-------|-------|-----------|
| **Jules-Gameplay** | Game mechanics | 22 | 5 | Physics, Animation |
| **Jules-Character** | Character systems | 130 | 4 | Animation |
| **Jules-Geometry** | Mesh systems | 134 | 4 | Rendering |
| **Jules-Materials** | Material system | 89 | 4 | Rendering, Shading |
| **Jules-Lighting** | Light & shadows | 131 | 4 | Rendering |
| **Jules-Effects** | VFX & particles | 79 | 4 | Rendering |

**Parallelize:** Can start independently after Phase 2 systems ready.

---

### PHASE 4: Polish (P2) - Weeks 7+
This one agent builds tools and polish after all systems exist.

| Agent | Role | Files | Tasks | Depends On |
|-------|------|-------|-------|-----------|
| **Jules-Tools** | Editor & dev tools | 308 | 6 | All others |

**Last:** Needs all other systems functional.

---

## How to Use This Structure

### If You Are An Agent Lead:
1. Read `master/JULES_15_AGENTS_ASSIGNMENTS.md` - your full section
2. Understand your 4-6 critical tasks
3. Verify file ownership (no overlap with others)
4. Plan implementation order (respect dependencies)
5. Update `master/todo.csv` as you start/complete tasks

### If You're Coordinating All 15 Agents:
1. Use `JULES_15_QUICK_REFERENCE.md` for status overview
2. Check phase readiness before advancing
3. Flag blockers that affect other agents
4. Hold weekly sync-ups on cross-agent issues
5. Verify no file ownership conflicts

### If You're New to the Project:
1. Read `JULES_15_QUICK_REFERENCE.md` (10 min)
2. Find your agent in the matrix
3. Read full assignment in `JULES_15_AGENTS_ASSIGNMENTS.md`
4. Start with first critical task
5. Ask for help if blocked

---

## Key Principles

### Zero Overlap Guarantee
Each agent owns exclusive file subsystems. Two agents never edit the same code.

**Ownership is by directory:**
```
src/engine/rendering/     → Jules-Rendering only
src/engine/physics/       → Jules-Physics only
src/engine/animation/     → Jules-Animation only
... etc
```

See `JULES_15_QUICK_REFERENCE.md` → "Directory Ownership" for complete list.

### Clear Dependencies
If Agent A blocks Agent B, it's documented. Check before starting a task.

```
Jules-Rendering MUST finish before Jules-Materials can start
Jules-Physics MUST finish before Jules-AI can pathfind
```

See dependency matrix in assignments doc.

### Phased Execution
Don't start Phase 2 until Phase 1 is solid. Phases are ~2 weeks each.

### No Task Duplication
Every critical task is assigned exactly once. No overlap, no guessing.

---

## Getting Started

### Step 1: Identify Your Agent
Which subsystem are you working on?
- Core engine? → Jules-Engine-Core
- Graphics? → Jules-Rendering  
- Physics? → Jules-Physics
- Animation? → Jules-Animation
... etc (15 total options)

### Step 2: Read Your Assignment
Open `master/JULES_15_AGENTS_ASSIGNMENTS.md`  
Find your agent section (1-15 in document)  
Read your mission statement and task list

### Step 3: Pick Your First Task
Each agent has 4-6 critical tasks.  
Start with task #1 (they're sequenced for dependencies).  
Read the full spec - it has everything you need.

### Step 4: Understand Dependencies
Check "Depends On" section - are prerequisites complete?  
If blocking on another agent, track their progress.

### Step 5: Plan Implementation
Review "Acceptance Test" - this is your definition of done.  
List all functions you need to implement.  
Verify all files you need to edit belong to your agent.

### Step 6: Update todo.csv
Add entry: `AGENT-###,implementation,Category,Jules-YourAgent,Task description,in_progress`

### Step 7: Implement & Test
Code to spec.  
Run acceptance tests.  
Commit with task ID reference.

### Step 8: Mark Complete
Update todo.csv: `status = completed`  
Add completion note to todo.md with test results.  
Move to next task.

---

## Estimated Timeline

| Phase | Duration | Agents | Status |
|-------|----------|--------|--------|
| PHASE 1: Foundation | 2 weeks | Engine-Core, Rendering, Physics | Ready to start |
| PHASE 2: Systems | 2 weeks | Animation, Audio, AI, Assets, Networking | Wait for P1 |
| PHASE 3: Integration | 2 weeks | Gameplay, Character, Geometry, Materials, Lighting, Effects | Wait for P2 |
| PHASE 4: Polish | 2+ weeks | Tools | Wait for P3 |
| **TOTAL** | **8+ weeks** | **All 15** | **Estimated completion** |

Each agent works in parallel within their phase.

---

## Success Criteria

### Engine Running ✅
- [ ] Phase 1 complete: Engine starts, runs, stops clean
- [ ] Phase 2 complete: All systems initialized and basic operations work
- [ ] Phase 3 complete: Game systems integrated and working together
- [ ] Phase 4 complete: Full editor functional, game playable

### Quality Standards ✅
- [ ] Zero memory leaks (use ASAN)
- [ ] No race conditions (use TSan)
- [ ] All acceptance tests pass for each task
- [ ] Code compiles with zero warnings
- [ ] Performance meets targets (60 FPS, 16.67ms budget)

### Organization ✅
- [ ] No file ownership overlap
- [ ] All tasks recorded in master/todo.csv
- [ ] Clear commit messages with task IDs
- [ ] Weekly status updates
- [ ] No blocked agents for extended periods

---

## Coordination Checklist

### Before Phase 1 Starts:
- [ ] All 3 Phase 1 agents understand their tasks
- [ ] Verify no file conflicts between agents
- [ ] Set up weekly sync schedule
- [ ] Confirm test plan and success criteria

### End of Phase 1 (Week 2):
- [ ] All 3 agents report ready for Phase 2
- [ ] Engine core systems working
- [ ] Phase 2 agents can start unblocked
- [ ] No critical bugs blocking progress

### End of Phase 2 (Week 4):
- [ ] All 5 Phase 2 agents complete core tasks
- [ ] Phase 3 agents can start unblocked
- [ ] Cross-system integration tests pass
- [ ] No critical blockers

### End of Phase 3 (Week 6):
- [ ] All 6 Phase 3 agents complete core tasks
- [ ] Full game loop functional (explore → collect → craft → fight)
- [ ] Phase 4 (Tools) can start
- [ ] Performance profiling done

### End of Phase 4+ (Week 8+):
- [ ] All 15 agents complete
- [ ] Full game playable
- [ ] Editor functional
- [ ] Codebase stable and maintainable

---

## Quick Links

| Document | Purpose | Read Time |
|----------|---------|-----------|
| `master/JULES_15_AGENTS_ASSIGNMENTS.md` | Full agent specifications | 60 min |
| `JULES_15_QUICK_REFERENCE.md` | At-a-glance reference | 10 min |
| `master/todo.md` | Status tracking | 5 min |
| `master/master.md` | Workflow rules | 10 min |
| `master/expectations.md` | Quality bar | 5 min |

---

## FAQ

**Q: What if I finish my agent's tasks early?**  
A: Cross-system testing, optimization, or help unblock other agents if possible.

**Q: What if another agent's task blocks me?**  
A: Flag it in the weekly sync. If critical, escalate for prioritization.

**Q: Can I edit files outside my agent's subsystem?**  
A: No. Only edit your assigned directory. Coordinate with other agents if needed.

**Q: When should I consolidate duplicate code?**  
A: After build is GREEN and your agent's core tasks done. Consolidation is polish, not blocking.

**Q: How do I report a bug in another agent's subsystem?**  
A: Open issue in master/ folder with details. Reference their agent. They'll prioritize.

---

## Ready?

1. **Read:** `JULES_15_QUICK_REFERENCE.md` (10 min overview)
2. **Locate:** Your agent in the matrix
3. **Deep Dive:** Your section in `JULES_15_AGENTS_ASSIGNMENTS.md`
4. **Plan:** Your first critical task
5. **Start:** Begin implementation
6. **Track:** Update `master/todo.csv` with progress

Let's build this engine! 🚀

---

**Questions?** Check `master/JULES_15_AGENTS_ASSIGNMENTS.md` for full details of your agent's mission.

End of START HERE

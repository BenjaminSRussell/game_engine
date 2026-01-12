# Jules 15 Agents - Phase-by-Phase Execution Guide

**Created:** 2026-01-12
**Updated:** 2026-01-12 (Now 15 Phases - One Per Agent)
**Status:** ✅ READY FOR EXECUTION
**Total Duration:** 30+ weeks, 15 phases, 65 tasks across 2,385 files

---

## 🎯 What This Is

You have **everything you need** to execute the Minecraft v2 engine development across 15 specialized agents, organized in 15 dedicated phases with clear data locations for every piece of information.

**NEW:** Each agent now has a dedicated phase. Phases 1-3 are sequential (critical path). Phases 4-14 can overlap after Phase 3. Phase 15 is the final editor/tools phase.

---

## ⭐ NEW: 15-Phase System (One Agent Per Phase)

**READ FIRST:** `master/JULES_15_PHASE_SYSTEM.md`

Instead of 4 phases with multiple agents per phase, we now have:
- **Phase 1:** Jules-Engine-Core (6 tasks)
- **Phase 2:** Jules-Rendering (7 tasks)
- **Phase 3:** Jules-Physics (6 tasks)
- **Phases 4-14:** Each dedicated to one agent (4-5 tasks each, can overlap)
- **Phase 15:** Jules-Tools (6 tasks, starts after all others complete)

**Total:** 15 phases, 65 tasks, 30+ weeks

---

## 📚 The 7 Core Documents

| Document | Location | Size | Purpose | Read Time |
|----------|----------|------|---------|-----------|
| **Phase Prompts** | `master/JULES_PHASE_PROMPTS.md` | 24 KB | Phase-by-phase execution prompts + data locations | 30 min |
| **Complete Index** | `master/JULIUS_COMPLETE_INDEX.md` | 11 KB | Navigation hub for all documents | 10 min |
| **Agent Assignments** | `master/JULIUS_15_AGENTS_ASSIGNMENTS.md` | 30 KB | Full specs for all 15 agents | 60 min |
| **Task Prompts** | `master/JULIUS_AGENT_TASK_PROMPTS.md` | 14 KB | Specific task instructions + CSV format | 30 min |
| **Quick Reference** | `JULIUS_15_QUICK_REFERENCE.md` | 10 KB | Matrix, ownership table, phase timeline | 10 min |
| **Status Tracking** | `master/todo.csv` | Variable | Live task database (in_progress/completed) | N/A |

---

## 🚀 PHASE 1: Foundation (Weeks 1-2)

### Quick Summary
Get the engine running with pixels on screen and physics objects simulating.

**Agents:** Jules-Engine-Core, Jules-Rendering, Jules-Physics (3 agents, 19 tasks)

**Where to Find Phase 1 Data:**

1. **Phase 1 Objectives & Prompts**
   - File: `master/JULIUS_PHASE_PROMPTS.md`
   - Section: "PHASE 1: Foundation"
   - Contains: Phase prompt, success criteria, blocking info

2. **Agent Task Lists**
   - File: `master/JULIUS_AGENT_TASK_PROMPTS.md`
   - Sections: "Agent 1: Jules-Engine-Core", "Agent 2: Jules-Rendering", "Agent 3: Jules-Physics"
   - Contains: Each agent's 4-7 tasks with prompts

3. **Detailed Agent Specs**
   - File: `master/JULIUS_15_AGENTS_ASSIGNMENTS.md`
   - Section: "TIER 1: Critical Foundation"
   - Contains: Full mission, ownership, task details with file locations

4. **Task Status**
   - File: `master/todo.csv`
   - Search: `grep "^CORE-\|^RENDER-\|^PHYS-" master/todo.csv`
   - Contains: Task ID, status (pending/in_progress/completed)

5. **File Ownership**
   - File: `JULIUS_15_QUICK_REFERENCE.md`
   - Section: "Directory Ownership"
   - Contains: Which agent owns which src/engine/* directories

### How to Start Phase 1

```bash
# Step 1: Read Phase 1 objective
cat master/JULIUS_PHASE_PROMPTS.md | grep -A 30 "PHASE 1: Foundation"

# Step 2: Read your agent's tasks
grep -A 200 "Agent 1: Jules-Engine-Core" master/JULIUS_AGENT_TASK_PROMPTS.md

# Step 3: Track progress
grep "^CORE-\|^RENDER-\|^PHYS-" master/todo.csv

# Step 4: Update status as you work
# Edit master/todo.csv, change status from pending → in_progress → completed
```

### Phase 1 Success
- [ ] Engine-Core: 6/6 tasks completed
- [ ] Rendering: 7/7 tasks completed
- [ ] Physics: 6/6 tasks completed
- [ ] Engine runs 10 frames clean
- [ ] Triangle renders at 60 FPS
- [ ] 1000+ physics objects at 60 FPS

---

## 📊 PHASE 2: Systems (Weeks 3-4)

### Quick Summary
Build animation, audio, AI, assets, and networking systems in parallel.

**Agents:** Jules-Animation, Jules-Audio, Jules-AI, Jules-Assets, Jules-Networking (5 agents, 20 tasks)

**Where to Find Phase 2 Data:**

1. **Phase 2 Objectives & Prompts**
   - File: `master/JULIUS_PHASE_PROMPTS.md`
   - Section: "PHASE 2: Systems"
   - Contains: Phase prompt, dependency checklist, success criteria

2. **Agent Task Lists**
   - File: `master/JULIUS_AGENT_TASK_PROMPTS.md`
   - Sections: Agent 4-8 (Animation, Audio, AI, Assets, Networking)
   - Contains: Each agent's 4-5 tasks with prompts

3. **Detailed Agent Specs**
   - File: `master/JULIUS_15_AGENTS_ASSIGNMENTS.md`
   - Section: "TIER 2: Major Systems"
   - Contains: Full mission, ownership, task details

4. **Task Status**
   - File: `master/todo.csv`
   - Search: `grep "^ANIM-\|^AUDIO-\|^AI-\|^ASSET-\|^NET-" master/todo.csv`
   - Contains: Task ID, status

5. **Phase 1 Dependency Check**
   - File: `master/JULIUS_PHASE_PROMPTS.md`
   - Section: "PHASE 2 → How Phase 2 Agents Use This → Pre-Phase 2 Checklist"
   - Verify: Phase 1 complete before starting Phase 2

### How to Start Phase 2

```bash
# Step 1: Verify Phase 1 complete
grep "^CORE-\|^RENDER-\|^PHYS-" master/todo.csv | grep -v "completed" | wc -l
# Should output: 0 (all tasks completed)

# Step 2: Read Phase 2 objective
cat master/JULIUS_PHASE_PROMPTS.md | grep -A 30 "PHASE 2: Systems"

# Step 3: Read your agent's tasks (e.g., Jules-Animation)
grep -A 200 "Agent 4: Jules-Animation" master/JULIUS_AGENT_TASK_PROMPTS.md

# Step 4: Track progress
grep "^ANIM-\|^AUDIO-\|^AI-\|^ASSET-\|^NET-" master/todo.csv
```

### Phase 2 Success
- [ ] Animation: 5/5 tasks completed
- [ ] Audio: 4/4 tasks completed
- [ ] AI: 4/4 tasks completed
- [ ] Assets: 4/4 tasks completed
- [ ] Networking: 4/4 tasks completed
- [ ] All systems integrate with Phase 1

---

## 🎨 PHASE 3: Integration (Weeks 5-6)

### Quick Summary
Build gameplay, character, geometry, materials, lighting, and effects in parallel.

**Agents:** Jules-Gameplay, Jules-Character, Jules-Geometry, Jules-Materials, Jules-Lighting, Jules-Effects (6 agents, 24 tasks)

**Where to Find Phase 3 Data:**

1. **Phase 3 Objectives & Prompts**
   - File: `master/JULIUS_PHASE_PROMPTS.md`
   - Section: "PHASE 3: Integration"
   - Contains: Phase prompt, dependencies, success criteria

2. **Agent Task Lists**
   - File: `master/JULIUS_AGENT_TASK_PROMPTS.md`
   - Sections: Agent 9-14 (Gameplay, Character, Geometry, Materials, Lighting, Effects)
   - Contains: Each agent's 4-5 tasks with prompts

3. **Detailed Agent Specs**
   - File: `master/JULIUS_15_AGENTS_ASSIGNMENTS.md`
   - Sections: Agent descriptions for each Phase 3 agent
   - Contains: Full mission, ownership, task details

4. **Task Status**
   - File: `master/todo.csv`
   - Search: `grep "^GAME-\|^CHAR-\|^GEO-\|^MAT-\|^LIGHT-\|^FX-" master/todo.csv`
   - Contains: Task ID, status

5. **Phase 1 + 2 Dependency Check**
   - File: `master/JULIUS_PHASE_PROMPTS.md`
   - Section: "PHASE 3 → How Phase 3 Agents Use This → Pre-Phase 3 Checklist"
   - Verify: Phase 1 (19 tasks) + Phase 2 (20 tasks) complete

### How to Start Phase 3

```bash
# Step 1: Verify Phase 1 + 2 complete
grep "^CORE-\|^RENDER-\|^PHYS-\|^ANIM-\|^AUDIO-\|^AI-\|^ASSET-\|^NET-" master/todo.csv | grep -v "completed" | wc -l
# Should output: 0 (all Phase 1 + 2 tasks completed)

# Step 2: Read Phase 3 objective
cat master/JULIUS_PHASE_PROMPTS.md | grep -A 40 "PHASE 3: Integration"

# Step 3: Read your agent's tasks (e.g., Jules-Gameplay)
grep -A 200 "Agent 9: Jules-Gameplay" master/JULIUS_AGENT_TASK_PROMPTS.md

# Step 4: Track progress
grep "^GAME-\|^CHAR-\|^GEO-\|^MAT-\|^LIGHT-\|^FX-" master/todo.csv
```

### Phase 3 Success
- [ ] Gameplay: 5/5 tasks completed
- [ ] Character: 4/4 tasks completed
- [ ] Geometry: 4/4 tasks completed
- [ ] Materials: 4/4 tasks completed
- [ ] Lighting: 4/4 tasks completed
- [ ] Effects: 4/4 tasks completed
- [ ] Core gameplay loop working (explore → craft → fight → progress)

---

## 🛠️ PHASE 4: Polish (Weeks 7+)

### Quick Summary
Build editor and development tools.

**Agents:** Jules-Tools (1 agent, 6 tasks)

**Where to Find Phase 4 Data:**

1. **Phase 4 Objectives & Prompts**
   - File: `master/JULIUS_PHASE_PROMPTS.md`
   - Section: "PHASE 4: Polish"
   - Contains: Phase prompt, dependencies, success criteria

2. **Agent Task List**
   - File: `master/JULIUS_AGENT_TASK_PROMPTS.md`
   - Section: "Agent 15: Jules-Tools"
   - Contains: 6 tasks with prompts

3. **Detailed Agent Spec**
   - File: `master/JULIUS_15_AGENTS_ASSIGNMENTS.md`
   - Section: "Agent 15: Jules-Tools"
   - Contains: Full mission, ownership, 6 task details

4. **Task Status**
   - File: `master/todo.csv`
   - Search: `grep "^TOOLS-" master/todo.csv`
   - Contains: Task ID, status

5. **All Prior Phases Dependency Check**
   - Verify: Phase 1 (19) + Phase 2 (20) + Phase 3 (24) = 63 tasks complete
   - Before: Jules-Tools cannot start until all other agents done

### How to Start Phase 4

```bash
# Step 1: Verify ALL prior phases complete
grep -v "completed" master/todo.csv | grep "CORE-\|RENDER-\|PHYS-\|ANIM-\|AUDIO-\|AI-\|ASSET-\|NET-\|GAME-\|CHAR-\|GEO-\|MAT-\|LIGHT-\|FX-" | wc -l
# Should output: 0 (all 63 tasks completed)

# Step 2: Read Phase 4 objective
cat master/JULIUS_PHASE_PROMPTS.md | grep -A 30 "PHASE 4: Polish"

# Step 3: Read Jules-Tools agent tasks
grep -A 200 "Agent 15: Jules-Tools" master/JULIUS_AGENT_TASK_PROMPTS.md

# Step 4: Track progress
grep "^TOOLS-" master/todo.csv
```

### Phase 4 Success
- [ ] Tools: 6/6 tasks completed
- [ ] Level editor working
- [ ] Gizmo system working
- [ ] Property editor working
- [ ] Asset browser working
- [ ] Debug visualization working
- [ ] Developer console working
- [ ] Full game playable

---

## 📍 Quick Data Lookup Reference

### "I need the Phase X prompt"
→ `master/JULIUS_PHASE_PROMPTS.md` → Section "PHASE X: [Name]"

### "I need the task list for my agent"
→ `master/JULIUS_AGENT_TASK_PROMPTS.md` → Section "Agent N: Jules-[YourAgent]"

### "I need complete specs for my agent"
→ `master/JULIUS_15_AGENTS_ASSIGNMENTS.md` → Section "Agent N: Jules-[YourAgent]"

### "What's my current task status?"
→ `master/todo.csv` → `grep "Jules-[YourAgent]" master/todo.csv`

### "Which files do I own?"
→ `JULIUS_15_QUICK_REFERENCE.md` → Section "Directory Ownership"

### "What are the phase dependencies?"
→ `master/JULIUS_PHASE_PROMPTS.md` → Your phase section → "Where to Find Data" → "Dependencies"

### "How do I update CSV when I start/complete a task?"
→ `master/JULIUS_AGENT_TASK_PROMPTS.md` → Section "CSV Integration - How It Works"

### "What's the complete task list for a phase?"
→ `master/JULIUS_PHASE_PROMPTS.md` → Your phase → "Phase X Task Details (N total)"

### "How do I navigate all these documents?"
→ `master/JULIUS_COMPLETE_INDEX.md` → Full navigation guide

---

## 🔄 Daily Workflow for Any Agent

### Morning:
1. Open `master/JULIUS_AGENT_TASK_PROMPTS.md`
2. Find your agent section
3. Check which task is next (your agent lead or CSV status)
4. Search CSV: `grep "TASK-ID" master/todo.csv`
5. If status = pending, update to in_progress
6. Read the **Your Prompt** section
7. Start implementing

### During Work:
- Keep CSV updated (status = in_progress)
- Implement according to prompt
- Test against acceptance criteria
- Commit frequently with task ID reference

### When Done:
- Update CSV: status = completed
- Add note to `master/todo.md`
- Commit final changes
- Move to next task in your agent section

### Weekly:
- Count tasks completed: `grep "Jules-[YourAgent]" master/todo.csv | grep "completed" | wc -l`
- Report % complete to team
- Flag blockers or dependencies needed
- Sync on cross-agent issues

---

## 🚨 Critical Rules

### ✅ DO:
- Read the phase prompt before starting
- Verify previous phase complete before starting yours
- Update CSV status (pending → in_progress → completed)
- Test against acceptance criteria
- Commit with task ID reference
- Report blockers immediately

### 🚫 DON'T:
- Skip phase prerequisites
- Edit files you don't own
- Disable/backup files (fix in-place)
- Create duplicate implementations
- Commit without updating CSV
- Overlap work with other agents

---

## 📊 Phase Verification Commands

### Verify Phase 1 Complete:
```bash
grep "^CORE-\|^RENDER-\|^PHYS-" master/todo.csv | grep -v "completed" | wc -l
# Output should be: 0 (means all 19 tasks completed)
```

### Verify Phase 2 Complete:
```bash
grep "^ANIM-\|^AUDIO-\|^AI-\|^ASSET-\|^NET-" master/todo.csv | grep -v "completed" | wc -l
# Output should be: 0 (means all 20 tasks completed)
```

### Verify Phase 3 Complete:
```bash
grep "^GAME-\|^CHAR-\|^GEO-\|^MAT-\|^LIGHT-\|^FX-" master/todo.csv | grep -v "completed" | wc -l
# Output should be: 0 (means all 24 tasks completed)
```

### Verify Phase 4 Complete:
```bash
grep "^TOOLS-" master/todo.csv | grep -v "completed" | wc -l
# Output should be: 0 (means all 6 tasks completed)
```

### Verify ALL Complete:
```bash
grep -v "completed" master/todo.csv | grep "CORE-\|RENDER-\|PHYS-\|ANIM-\|AUDIO-\|AI-\|ASSET-\|NET-\|GAME-\|CHAR-\|GEO-\|MAT-\|LIGHT-\|FX-\|TOOLS-" | wc -l
# Output should be: 0 (means all 65+ tasks completed)
```

---

## 📈 Progress Dashboard

### Check Overall Progress:
```bash
# Total tasks
echo "Total completed tasks:"
grep "completed" master/todo.csv | grep "CORE-\|RENDER-\|PHYS-\|ANIM-\|AUDIO-\|AI-\|ASSET-\|NET-\|GAME-\|CHAR-\|GEO-\|MAT-\|LIGHT-\|FX-\|TOOLS-" | wc -l
# Should show incremental progress toward 65

# Phase 1 progress
echo "Phase 1 (should be 0 or 19):"
grep "^CORE-\|^RENDER-\|^PHYS-" master/todo.csv | grep "completed" | wc -l

# Phase 2 progress
echo "Phase 2 (should be 0 or 20):"
grep "^ANIM-\|^AUDIO-\|^AI-\|^ASSET-\|^NET-" master/todo.csv | grep "completed" | wc -l

# Phase 3 progress
echo "Phase 3 (should be 0 or 24):"
grep "^GAME-\|^CHAR-\|^GEO-\|^MAT-\|^LIGHT-\|^FX-" master/todo.csv | grep "completed" | wc -l

# Phase 4 progress
echo "Phase 4 (should be 0 or 6):"
grep "^TOOLS-" master/todo.csv | grep "completed" | wc -l
```

---

## 🎓 Learning Path (45 minutes)

1. **5 min:** Read `master/JULIUS_PHASE_PROMPTS.md` - your current phase
2. **10 min:** Read `JULIUS_15_QUICK_REFERENCE.md` - agent matrix & ownership
3. **15 min:** Read `master/JULIUS_15_AGENTS_ASSIGNMENTS.md` - your agent section
4. **5 min:** Open `master/todo.csv`, find your agent tasks
5. **10 min:** Read `master/JULIUS_AGENT_TASK_PROMPTS.md` - your first task prompt
6. **Ready:** Start implementing per prompt

---

## 🏁 Ready to Go?

1. Identify which phase you're in
2. Read phase prompt from `master/JULIUS_PHASE_PROMPTS.md`
3. Verify previous phase complete (run verification commands above)
4. Find your agent in `master/JULIUS_AGENT_TASK_PROMPTS.md`
5. Read first task prompt
6. Update `master/todo.csv` to in_progress
7. Implement according to prompt
8. Test, commit, mark complete
9. Move to next task
10. Report progress weekly

**Everything you need is in these documents. Let's build this engine!** 🚀

---

End of Execution Guide

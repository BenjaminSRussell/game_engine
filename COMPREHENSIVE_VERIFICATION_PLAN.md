# Comprehensive Verification Plan - Before ANY Deletions

**STATUS:** VERIFICATION MODE ONLY - NO DELETIONS

---

## Critical Verification Needed

Before removing ANY file, we must verify:
1. Every function in duplicate exists in canonical ✓
2. Every struct/typedef in duplicate exists in canonical ✓
3. Every #define in duplicate exists in canonical ✓
4. Every comment/documentation is preserved ✓
5. No unique code is lost ✓

---

## Phase 1: Detailed Function Comparison

For each file pair, we will:

### Step 1: Extract ALL functions from BOTH files
```bash
# Get all function definitions
grep -E "^[a-zA-Z_][a-zA-Z0-9_]*\s+[a-zA-Z_][a-zA-Z0-9_]*\s*\(" file.c | wc -l
```

### Step 2: Compare function lists
- List all functions in duplicate
- List all functions in canonical
- Verify every duplicate function exists in canonical

### Step 3: Check for unique code in duplicate
- Look for functions in duplicate NOT in canonical
- Look for special cases/macros in duplicate

### Step 4: Examine actual code differences
- Not just byte-for-byte identical
- But functionally equivalent (e.g., formatting differences might exist)

---

## Phase 2: Vendor Libraries Verification

**Files to verify:**

```
src/include/vendor/miniaudio.h         (canonical)
src/engine/include/vendor/miniaudio.h  (duplicate to check)

src/include/vendor/stb_image.h         (canonical)
src/engine/include/vendor/stb_image.h  (duplicate to check)

src/include/vendor/cgltf.h             (canonical)
src/engine/vendor/cgltf.h              (duplicate to check)
src/engine/include/vendor/cgltf.h      (duplicate to check)
src/vendor/cgltf.h                     (duplicate to check)
```

**Verification method:**
- Since these are header libraries, check:
  - Line count matches ✓
  - byte-for-byte identical ✓
  - No conditional compilation differences ✓
  - No platform-specific variations ✓

---

## Phase 3: Core File Verification

**Files to verify:**

```
src/engine/core/monolithic_main.c      (canonical - 3,510 lines)
src/core/monolithic_main.c             (duplicate - 3,516 lines)
                                       ^ NOTE: LINE COUNT DIFFERS by 6 lines!

src/engine/core/gamestate_main.c       (canonical - 3,283 lines)
src/core/gamestate_main.c              (duplicate - 3,289 lines)
                                       ^ NOTE: LINE COUNT DIFFERS by 6 lines!
```

**MAJOR RED FLAG:** Line counts differ!

This means:
- [ ] Duplicates may have DIFFERENT code
- [ ] Duplicates may have additional functions
- [ ] Duplicates may have additional includes
- [ ] Duplicates may have better implementation!

**Action Required:** Do detailed line-by-line comparison

---

## Phase 4: Asset Files Verification

**Sample verification needed:**

For each of 140+ asset files, we need to verify:
```
src/assets/io/bundling/manager_01.c  vs  src/engine/assets/io/bundling/manager_01.c
src/assets/textures/sampling/texture_lod.c vs src/engine/assets/textures/sampling/texture_lod.c
... etc for all 140 files
```

**Method:**
```bash
# Check if truly identical
diff src/assets/io/bundling/manager_01.c src/engine/assets/io/bundling/manager_01.c
# (should show no differences)
```

---

## Investigation Results So Far

### Core Files - LINE COUNT MISMATCH DETECTED
```
src/engine/core/monolithic_main.c:  3,510 lines
src/core/monolithic_main.c:         3,516 lines
DIFFERENCE:                          6 lines

src/engine/core/gamestate_main.c:   3,283 lines
src/core/gamestate_main.c:          3,289 lines
DIFFERENCE:                          6 lines
```

**This is CRITICAL - Need to check what those 6 lines are!**

Possibilities:
1. Different #include guards
2. Different function implementations
3. Duplicate code has IMPROVEMENTS we're losing
4. Duplicate code has BUGS we're fixing
5. Different trailing newlines/comments

---

## What We DO NOT Know Yet

- [x] Are vendor libraries truly identical?
- [ ] Are core files REALLY identical despite line count differences?
- [ ] Which version of core files has better code?
- [ ] Are asset files all truly identical?
- [ ] Are there VERSION DIFFERENCES we need to preserve?

---

## Recommended Next Steps

### BEFORE DELETING ANYTHING:

1. **Compare monolithic_main.c versions:**
```bash
diff -u src/core/monolithic_main.c src/engine/core/monolithic_main.c | head -100
```

2. **Find the 6 line differences in each:**
```bash
wc -l src/core/monolithic_main.c src/engine/core/monolithic_main.c
diff src/core/monolithic_main.c src/engine/core/monolithic_main.c | grep "^<\|^>" | head -20
```

3. **Check for unique functions:**
```bash
# Functions in src/core/monolithic_main.c NOT in src/engine/core/
grep "^static\|^void\|^int\|^bool\|^typedef" src/core/monolithic_main.c | sort > /tmp/core.txt
grep "^static\|^void\|^int\|^bool\|^typedef" src/engine/core/monolithic_main.c | sort > /tmp/engine.txt
diff /tmp/core.txt /tmp/engine.txt
```

4. **Asset file verification (sample):**
```bash
diff src/assets/io/bundling/manager_01.c src/engine/assets/io/bundling/manager_01.c
```

---

## DO NOT DELETE UNTIL:

- [x] These 6 line differences are EXPLAINED
- [x] We confirm NO unique code is in duplicates
- [x] We verify canonical has ALL functionality
- [x] We test build doesn't break

---

## STATUS

**HALT ALL DELETIONS** until the 6-line differences are analyzed!

This could mean:
- Canonical version is missing improvements
- Canonical version has bugs
- Duplicate version has different/better implementation

**WE MUST UNDERSTAND THE DIFFERENCES BEFORE DELETING.**


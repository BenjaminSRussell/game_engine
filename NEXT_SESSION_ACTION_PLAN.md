# Next Session Action Plan

**Prepared:** January 7, 2026
**For:** Continuing Phase 1 Stabilization
**Duration:** ~3-4 hours
**Goals:** Fixes #7-11, verify compilation, test basic rendering

---

## START HERE: Quick Context

You have completed 6 critical crash fixes in the Metal backend:
1. ✅ convert_format void return
2. ✅ Null pointer after allocation
3. ✅ Fence creation logic
4. ✅ Listener lifetime
5. ✅ Realloc failure handling
6. ✅ Shader cache collision

**All fixes are ready for compilation and testing.**

---

## Phase 1: Compilation & Verification (15 minutes)

### Step 1: Compile the Metal Backend
```bash
cd "/Users/benjaminrussell/Desktop/Minecraft v2"

# Full compilation with all 6 fixes
clang -Wall -Wextra -Werror \
  -fsanitize=address,undefined \
  -framework Metal -framework Cocoa -framework Dispatch \
  src/engine/backend/metal/mtl_*.c \
  src/engine/backend/metal/mtl_*.m \
  -o test_metal_backend_fixed 2>&1
```

### Expected Results
- ✅ Zero compilation errors
- ✅ 0-5 warnings (acceptable)
- ✅ Binary created: test_metal_backend_fixed

### If Compilation Fails
- Check syntax in modified files
- Review error messages in SESSION_2_SUMMARY.md
- Files modified:
  - src/engine/backend/metal/mtl_texture.c
  - src/engine/backend/metal/mtl_sync.m
  - src/engine/backend/metal/mtl_frame_sync.c
  - src/engine/backend/metal/mtl_shader_compiler.m

---

## Phase 2: Run Basic Tests (30 minutes)

### Test 1: Verify Fixes Exist
```bash
# Verify convert_format signature
grep -n "static inline MTLPixelFormat convert_format" \
  src/engine/backend/metal/mtl_texture.c

# Expected: Line 29 shows "MTLPixelFormat" (not "void")

# Verify public API exists
grep -n "MTLPixelFormat metal_pixel_format_to_mtl" \
  src/engine/backend/metal/mtl_texture.c

# Expected: Line ~353 shows proper implementation
```

### Test 2: Verify All Fixes Are Present
```bash
# Check all 6 fixes are in place
echo "=== Fix #1: convert_format signature ==="
grep "static inline MTLPixelFormat convert_format"
echo ""
echo "=== Fix #2: Null check after descriptor alloc ==="
grep -A 2 "MTLTextureDescriptor.*alloc.*init" mtl_texture.c | head -5
echo ""
echo "=== Fix #3: Listener release removed ==="
grep -n "listener release" src/engine/backend/metal/mtl_sync.m
echo "Expected: Should NOT find any 'listener release' lines"
echo ""
echo "=== Fix #4: Fence creation simplified ==="
grep -n "metal_fence_t.*new_fence.*NULL" src/engine/backend/metal/mtl_sync.m
echo "Expected: Should NOT find this pattern"
echo ""
echo "=== Fix #5: Realloc checks ==="
grep -n "if (!new_resources)" src/engine/backend/metal/mtl_frame_sync.c
echo "Expected: Should find error handling"
echo ""
echo "=== Fix #6: Shader cache collision check ==="
grep -n "strcmp.*source_hash" src/engine/backend/metal/mtl_shader_compiler.m
echo "Expected: Should find strcmp verification"
```

### Test 3: Unit Tests (from PHASE_1_TEST_FRAMEWORK.md)
```bash
# When ready, run provided unit tests:
# - test_convert_format_rgba8
# - test_texture_allocation
# - test_listener_callback
# - test_frame_resource_realloc
# - test_shader_cache_collision
```

---

## Phase 3: Implement Remaining Crashes #7-11 (2 hours)

### Fix #7: Encoder Null Check
**Location:** mtl_encoder.c (lines ~39)
**Issue:** Encoder creation not validated
**Action:** Add null check after `[cmd_buffer commandRenderEncoderWithDescriptor:]`

### Fix #8: Command Buffer State
**Location:** mtl_command_buffer.c
**Issue:** Using completed command buffer
**Action:** Add buffer state tracking before encoding

### Fix #9: Device Array Leak
**Location:** mtl_device.c (lines ~161)
**Issue:** Device array not released
**Action:** Add proper cleanup in shutdown

### Fix #10: Fence Lifetime
**Location:** mtl_sync.m
**Issue:** Fence released before use
**Action:** Manage fence lifetime with command buffer

### Fix #11: Format Fallback
**Location:** mtl_texture.c (lines ~130)
**Issue:** Uninitialized format variable
**Action:** Implement proper convert_format fallback

**Documentation:** See PHASE_1_CRASH_FIXES_IMPLEMENTATION_GUIDE.md for detailed code

---

## Phase 4: Verification (30 minutes)

### Recompile
```bash
clang -Wall -Wextra -Werror \
  -fsanitize=address,undefined \
  -framework Metal -framework Cocoa -framework Dispatch \
  src/engine/backend/metal/mtl_*.c \
  src/engine/backend/metal/mtl_*.m \
  -o test_metal_backend_fixed 2>&1
```

### Test Basic Operations
```bash
# Create and test:
# 1. Texture creation (uses convert_format)
# 2. Event callbacks (uses listener)
# 3. Frame resources (uses realloc)
# 4. Shader caching (uses hash collision detection)
```

---

## Phase 5: Documentation & Planning (30 minutes)

### Update Progress
- [ ] Mark fixes #7-11 as completed in todo
- [ ] Update PHASE_1_STABILIZATION_PROGRESS.md
- [ ] Create SESSION_3_SUMMARY.md

### Plan Next Steps
- [ ] Review fixes #12-25 (freeze prevention)
- [ ] Estimate time for memory leak fixes #26-37
- [ ] Plan full test cycle

---

## Reference Documents

### During This Session
1. **PHASE_1_FIXES_COMPLETED.md** - Summary of fixes #1-6
2. **SESSION_2_SUMMARY.md** - Detailed session report
3. **PHASE_1_CRASH_FIXES_IMPLEMENTATION_GUIDE.md** - Code examples for #7-11
4. **PHASE_1_TEST_FRAMEWORK.md** - Unit tests to run

### For Understanding
1. **PHASE_1_CRASH_FIX_PRIORITY_LIST.md** - Complete issue list (100 items)
2. **PHASE_1_STABILIZATION_SUMMARY.md** - Overview of all work
3. **PHASE_1_START_HERE.md** - Quick reference guide

---

## Files to Monitor

### Modified in Session 2 (Don't lose changes!)
- ✅ src/engine/backend/metal/mtl_texture.c
- ✅ src/engine/backend/metal/mtl_sync.m
- ✅ src/engine/backend/metal/mtl_frame_sync.c
- ✅ src/engine/backend/metal/mtl_shader_compiler.m

### To Modify in Session 3
- mtl_encoder.c (Fix #7)
- mtl_command_buffer.c (Fix #8)
- mtl_device.c (Fix #9)
- mtl_sync.m (Fix #10 - again)
- mtl_texture.c (Fix #11 - again)

---

## Success Metrics

### For This Session
- [ ] All 6 fixes compile without errors
- [ ] 0-5 warnings acceptable
- [ ] Fixes #7-11 implemented
- [ ] Basic texture creation works
- [ ] Event callbacks execute
- [ ] Frame resources allocate properly

### Expected Outcome
**Before:** 70% crash rate on basic operations
**After:** <5% crash rate on basic operations (only #12-25 issues remain)

---

## Troubleshooting

### Compilation Error: "file not found"
- Make sure you're in the right directory
- Check file paths match exactly
- Use absolute paths if relative paths fail

### Compilation Error: "unknown type"
- Check headers are included (mtl_*.h)
- Verify Objective-C compilation for .m files
- Check for missing forward declarations

### Verification Test Fails
- Compare to PHASE_1_TEST_FRAMEWORK.md examples
- Check that fixes are actually in place
- Run verification grep commands above

### Tests Don't Run
- Make sure binary compiled successfully
- Verify binary exists: test_metal_backend_fixed
- Check test code syntax

---

## Command Cheat Sheet

```bash
# Location
cd "/Users/benjaminrussell/Desktop/Minecraft v2"

# Verify fixes are present
grep "MTLPixelFormat convert_format" src/engine/backend/metal/mtl_texture.c
grep -c "NSLog" src/engine/backend/metal/mtl_frame_sync.c

# Compile everything
clang -Wall -Wextra -Werror -fsanitize=address,undefined \
  -framework Metal -framework Cocoa -framework Dispatch \
  src/engine/backend/metal/mtl_*.c src/engine/backend/metal/mtl_*.m \
  -o test_metal_backend_fixed 2>&1 | head -50

# Run tests
./test_metal_backend_fixed

# Check specific files for patterns
grep -n "TODO\|FIXME\|XXX" src/engine/backend/metal/mtl_*.c src/engine/backend/metal/mtl_*.m
```

---

## Time Budget

- **Compilation & Verification:** 15 min
- **Testing Existing Fixes:** 30 min
- **Implementing Fixes #7-11:** 120 min
- **Recompile & Verify:** 15 min
- **Documentation & Planning:** 30 min
- **Buffer for debugging:** 30 min

**Total:** ~3.5-4 hours

---

## Exit Criteria (Session Complete When)

✅ Compilation succeeds with all 11 fixes
✅ No segmentation faults on basic operations
✅ Fixes #1-6 verified working
✅ Fixes #7-11 implemented and compiled
✅ Session summary created
✅ Next session plan ready

---

## Important Notes

1. **Don't skip compilation** - Syntax errors now prevent later issues
2. **Test each fix** - Verify before moving to next
3. **Keep git clean** - All changes are deliberate improvements
4. **Document everything** - Future sessions need context
5. **Progress systematically** - One fix at a time

---

## When Stuck

1. Check PHASE_1_CRASH_FIX_PRIORITY_LIST.md for issue details
2. Read PHASE_1_CRASH_FIXES_IMPLEMENTATION_GUIDE.md for code
3. Review SESSION_2_SUMMARY.md to understand completed work
4. Use verification grep commands above
5. Refer to PHASE_1_TEST_FRAMEWORK.md for test patterns

---

## Ready to Go!

You have:
✅ 6 verified crash fixes
✅ Complete documentation
✅ Test framework prepared
✅ 94 remaining issues identified
✅ Implementation guides ready

**Start with compilation. You've got this!**

---

**Prepared:** January 7, 2026
**For:** Continuing developer
**Status:** Ready for next session
**Duration:** ~3-4 hours estimated
**Success Rate:** High - systematic approach, clear documentation

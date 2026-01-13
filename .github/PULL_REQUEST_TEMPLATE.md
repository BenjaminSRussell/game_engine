# Pull Request Checklist

## Pre-Merge Verification

- [ ] Ran duplicate detection script: `python3 tools/check_duplicate_features.py <branch>`
- [ ] Searched for similar features in main branch
- [ ] Verified no overlapping TODOs being implemented
- [ ] Checked subsystem ownership list

## Code Quality

- [ ] Code compiles without errors
- [ ] All tests pass
- [ ] Added tests for new features
- [ ] Updated documentation

## Subsystem Impact

Which subsystems does this PR affect?
- [ ] Physics
- [ ] Rendering
- [ ] Animation
- [ ] UI
- [ ] Audio
- [ ] Networking
- [ ] AI
- [ ] Core Systems

Subsystem Owner Approval: @___________

## Related Work

- Related branches: 
- Depends on:
- Blocks:

## Description

Brief description of changes and rationale.

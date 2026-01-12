# Engine Expectations

## Purpose
Set the quality and depth bar for agent reviews and implementations across the entire codebase, targeting Unreal Engine-level standards.

## Review Scope (Full Codebase)
- Cover the entire codebase across assigned categories; do not skip subsystems due to size.
- Verify feature parity across platforms/backends where applicable.
- Identify missing or stubbed implementations, broken integrations, and unreachable code.
- Compare the system design and feature completeness against Unreal Engine-level expectations.

## Quality Bar (Unreal Engine Standard)
- Builds must be green before any consolidation work begins.
- No disabled files or `.disabled`/backup variants; fix or integrate in-place.
- No duplicate or conflicting implementations left unaccounted for.
- Every TODO must be specific: file/function, expected behavior, and acceptance test.
- Match Unreal Engine-level robustness: stable runtime, predictable performance, and clear editor workflows.

## Depth Expectations
- Trace the full data flow for each major system (init → update → render → shutdown).
- Document coupling points (shared data, ownership, and lifetimes).
- Require end-to-end tests for gameplay-critical paths.
- Validate parity with Unreal-grade tooling expectations (profiling, diagnostics, and editor reliability).

## Evidence Required
- Cite files and functions for each finding.
- Add a TODO for each gap with a direct action and validation step.
- Link any new TODO to its subsystem and priority in `master/todo.csv`.

## Output Format
- Add findings as TODOs in `master/todo.csv` and `master/todo.md`.
- Keep tasks small and actionable (1-3 days of work each).
- Include explicit acceptance criteria and a test/verification method.

## Plans And Sequencing
- Prioritize build correctness and integration stability before optimization.
- Consolidation is the final step after build is green.
- Avoid overlapping changes between agents; claim category in `master/todo.md`.

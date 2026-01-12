# Master Docs Hub

This folder centralizes all non-README markdown documentation and TODO tracking.

## Key Files
- `master/todo.md`: Human-readable active TODOs (from docs + in-code TODOs).
- `master/todo.csv`: Canonical machine-readable TODO list; remove items when done.
- `master/summary.md`: Inventory of every file in this folder with purpose and agent rules.

## Workflow
- Use `master/todo.csv` as the source of truth.
- Keep `master/todo.md` in sync for human review.
- Do not add new TODO markdown files outside this folder.
- Only one README is allowed at repo root (`README.md`).
- Consolidation is the final step after build is green; do not consolidate before a successful build.
- Read `master/master.md` before starting any work to confirm rules and workflow.
- Use `master/expectations.md` for quality and depth requirements.

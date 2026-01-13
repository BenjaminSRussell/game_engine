# UI Layout Profiling Tests

This directory contains tests for the UI layout engine performance profiling.

## Files

- `test_layout_profiling.c`: The test source code.
- `run_profiling_test.sh`: Script to compile and run the test.

## Usage

To run the test:

```bash
./run_profiling_test.sh
```

## Dependencies

The test links against:
- `src/engine/ui/layout/flexbox_layout.c`
- `src/engine/ui/layout/grid_layout.c`
- `src/engine/platform/time_system.c`

And requires the engine headers.

# Deployment Validation Tool

A comprehensive validation system for the Minecraft v2 Engine deployment checklist, ensuring code quality, performance standards, and cross-platform compatibility are met before release.

## Overview

The Deployment Validation Tool provides automated checking of all critical deployment requirements specified in the ENGINE_ARCHITECTURE_MASTER_AUDIT.md checklist. It performs comprehensive analysis across three main categories:

- **Code Quality**: TODO/FIXME detection, error condition testing, memory optimization, thread safety verification
- **Performance**: Frame time stability, memory leak detection, GPU/CPU utilization monitoring
- **Compatibility**: Windows Vulkan, macOS Metal, and Linux Vulkan backend verification

## Features

### Code Quality Validation
- Scans source code for TODO and FIXME comments in critical paths
- Validates error condition test coverage
- Analyzes memory usage patterns for optimization opportunities
- Verifies thread safety across all systems using static analysis
- Counts lines of code and provides quality metrics

### Performance Monitoring
- Monitors frame time stability with spike detection (>20% threshold)
- Runs 10-minute memory leak detection tests
- Tracks GPU utilization (target: 60-80%)
- Monitors CPU utilization (target: 50-70%)
- Provides detailed performance metrics and variance analysis

### Compatibility Testing
- Validates Windows 10+ Vulkan backend support
- Checks macOS 10.15+ Metal backend compatibility
- Verifies Linux Vulkan backend functionality
- Reports system information and graphics capabilities

## Building

### Prerequisites
- CMake 3.16 or higher
- C99-compatible compiler (GCC, Clang, or MSVC)
- pthread library (Unix/Linux systems)

### Build Instructions

```bash
# Create build directory
mkdir build && cd build

# Configure with CMake
cmake ..

# Build the tool
make

# Install (optional)
make install
```

### Build Targets
- `deployment_validator` - Main validation executable
- `run_validation` - Custom target to run all validation checks
- `generate_report` - Generate JSON validation report

## Usage

### Basic Usage

```bash
# Run all validation checks
./deployment_validator

# Run specific category
./deployment_validator -c performance

# Save report to file
./deployment_validator -o report.json -f json

# Verbose output
./deployment_validator -v

# Quiet mode (errors only)
./deployment_validator -q
```

### Command Line Options

- `-h, --help` - Show help message
- `-c, --category CAT` - Run specific category (code, performance, compatibility)
- `-o, --output FILE` - Save report to file
- `-f, --format FORMAT` - Output format (text, json, xml)
- `-v, --verbose` - Enable verbose output
- `-q, --quiet` - Suppress non-error output
- `--list-checks` - List all available checks

### Examples

```bash
# Run performance checks and save JSON report
./deployment_validator -c performance -o perf_report.json -f json

# Run all checks with verbose output
./deployment_validator -v

# List all available checks
./deployment_validator --list-checks

# Generate comprehensive report for CI/CD
./deployment_validator -o deployment_report.json -f json -q
```

## Validation Checks

### Code Quality Checks

| ID | Check | Severity | Description |
|----|-------|----------|-------------|
| TODO-0060 | No TODOs/FIXMEs in main code paths | High | Ensure no TODO or FIXME comments remain in critical code paths |
| TODO-0061 | All error conditions tested | High | Verify all error conditions have corresponding tests |
| TODO-0062 | Memory usage profiled + optimized | Medium | Profile and optimize memory usage patterns |
| TODO-0063 | Thread safety verified | Critical | Verify thread safety across all systems |

### Performance Checks

| ID | Check | Severity | Description |
|----|-------|----------|-------------|
| TODO-0064 | Frame time stable (no spikes >20%) | High | Ensure frame times are stable with no spikes >20% |
| TODO-0065 | Memory usage stable (no leaks over 10m runtime) | High | Run 10-minute memory leak detection test |
| TODO-0066 | GPU utilization 60-80% (not bottlenecked) | Medium | Monitor GPU utilization (target 60-80%) |
| TODO-0067 | CPU utilization 50-70% (not throttled) | Medium | Monitor CPU utilization (target 50-70%) |

### Compatibility Checks

| ID | Check | Severity | Description |
|----|-------|----------|-------------|
| TODO-0068 | Windows 10+ (Vulkan backend) | Medium | Verify Windows 10+ Vulkan backend compatibility |
| TODO-0069 | macOS 10.15+ (Metal backend) | Medium | Verify macOS 10.15+ Metal backend compatibility |
| TODO-0070 | Linux (Vulkan backend) | Medium | Verify Linux Vulkan backend compatibility |

## Output Formats

### Text Output
Human-readable console output with detailed progress and results.

### JSON Output
Structured JSON format suitable for CI/CD integration and automated processing:

```json
{
  "validation_report": {
    "timestamp": "Mon Jan 13 18:25:00 2025",
    "summary": {
      "total_checks": 11,
      "passed": 8,
      "failed": 2,
      "warnings": 1,
      "skipped": 0
    },
    "code_quality": {
      "todo_count": 25,
      "fixme_count": 8,
      "total_lines_of_code": 150000,
      "thread_safety_violations": 0,
      "memory_safety_issues": 2
    },
    "performance": {
      "frame_time_ms": 16.67,
      "frame_time_variance": 0.25,
      "frame_spikes_20_percent": 12,
      "memory_usage_mb": 518.0,
      "memory_leak_rate_mb_per_min": 0.6,
      "gpu_utilization_percent": 75.0,
      "cpu_utilization_percent": 65.0
    },
    "compatibility": {
      "windows_vulkan_supported": true,
      "macos_metal_supported": true,
      "linux_vulkan_supported": true
    }
  }
}
```

## Integration

### CI/CD Pipeline Integration

The tool is designed for easy integration into CI/CD pipelines:

```bash
#!/bin/bash
# Example CI script

# Run validation checks
./deployment_validator -o validation_report.json -f json -q

# Check exit code
if [ $? -eq 0 ]; then
    echo "✅ All validation checks passed"
    exit 0
else
    echo "❌ Validation checks failed"
    cat validation_report.json
    exit 1
fi
```

### GitHub Actions Example

```yaml
name: Deployment Validation
on: [push, pull_request]

jobs:
  validate:
    runs-on: ubuntu-latest
    steps:
    - uses: actions/checkout@v2
    - name: Build validation tool
      run: |
        cd src/tools/deployment
        mkdir build && cd build
        cmake ..
        make
    - name: Run validation checks
      run: |
        cd src/tools/deployment/build
        ./deployment_validator -o validation_report.json -f json
    - name: Upload validation report
      uses: actions/upload-artifact@v2
      with:
        name: validation-report
        path: src/tools/deployment/build/validation_report.json
```

## Configuration

The tool can be configured through environment variables:

- `DEPLOYMENT_VALIDATOR_SOURCE_DIR` - Override source directory path
- `DEPLOYMENT_VALIDATOR_OUTPUT_DIR` - Override output directory
- `DEPLOYMENT_VALIDATOR_THREADS` - Set number of threads for parallel analysis

## Troubleshooting

### Common Issues

1. **Permission Denied**: Ensure the executable has proper permissions
   ```bash
   chmod +x deployment_validator
   ```

2. **Source Directory Not Found**: Set the correct working directory or use environment variable
   ```bash
   export DEPLOYMENT_VALIDATOR_SOURCE_DIR="/path/to/minecraft/v2"
   ```

3. **Memory Analysis Fails**: Ensure sufficient system memory for large codebases

### Debug Mode

Compile with debug flags for detailed error reporting:
```bash
cmake -DCMAKE_BUILD_TYPE=Debug ..
make
```

## Contributing

When adding new validation checks:

1. Add the check to the appropriate category in `deployment_validator.h`
2. Implement the validation function in `deployment_validator.c`
3. Update the command-line interface to support the new check
4. Add tests to verify the functionality
5. Update this documentation

## License

This tool is part of the Minecraft v2 Engine project and follows the same licensing terms.

## Support

For issues, questions, or contributions, please refer to the main project repository or contact the development team.

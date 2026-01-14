# Minecraft v2 Engine Test Framework

A comprehensive testing framework designed to achieve >80% code coverage across all major engine systems with unit tests, integration tests, and stress testing capabilities.

## Overview

The test framework provides:
- **Unit Tests**: Individual component testing with comprehensive coverage
- **Integration Tests**: Cross-system interaction validation
- **Stress Tests**: Performance and stability testing under load
- **Coverage Analysis**: Detailed code coverage reporting
- **Memory Leak Detection**: Automatic memory leak tracking
- **Thread Safety Testing**: Multi-threaded operation validation
- **Performance Profiling**: Execution time and resource usage tracking

## Features

### Test Suites

#### Core Systems
- **Memory Tests**: Allocation, deallocation, fragmentation, thread safety
- **Audio Tests**: System initialization, source management, mixing, performance
- **Physics Tests**: World simulation, rigid bodies, collision detection, response
- **Math Tests**: Vector operations, matrix operations, trigonometric functions
- **Rendering Tests**: Graphics pipeline, shader management, frame buffer operations
- **AI Tests**: Behavior trees, pathfinding, decision making
- **Animation Tests**: Skeletal animation, morph targets, blending
- **Input Tests**: Event handling, device management, mapping
- **Network Tests**: Protocol handling, data serialization, connection management
- **Filesystem Tests**: File operations, path handling, I/O performance

#### Integration Tests
- **Audio-Physics Integration**: Sound propagation, physics-based audio effects
- **Rendering-Physics Integration**: Physics visualization, debug rendering
- **AI-Animation Integration**: Character animation driven by AI decisions
- **Input-Audio Integration**: Real-time audio parameter control

#### Stress Tests
- **Entity Stress Tests**: 1000+ entities for 5+ minutes
- **Memory Stress Tests**: Large allocation patterns and fragmentation
- **Physics Stress Tests**: Complex collision scenarios under load
- **Rendering Stress Tests**: High-poly scenes and shader complexity

### Advanced Features

#### Coverage Analysis
- Line-by-line coverage tracking
- Function coverage reporting
- Branch coverage analysis
- HTML and JSON report generation
- Minimum coverage threshold enforcement

#### Memory Management
- Automatic leak detection
- Allocation tracking
- Usage pattern analysis
- Fragmentation monitoring
- Peak memory usage reporting

#### Performance Monitoring
- Execution time measurement
- Memory usage tracking
- CPU utilization monitoring
- Cache hit/miss ratios
- GPU utilization (where applicable)

#### Thread Safety
- Multi-threaded test execution
- Race condition detection
- Deadlock prevention validation
- Atomic operation testing
- Synchronization primitive testing

## Building

### Prerequisites
- CMake 3.16 or higher
- C99-compatible compiler (GCC, Clang, or MSVC)
- pthread library
- Math library (libm)

### Build Instructions

```bash
# Create build directory
mkdir build && cd build

# Configure with CMake
cmake ..

# Build the test framework
make -j$(nproc)

# Run all tests
make run_tests

# Or run the test executable directly
./bin/engine_test_runner --help
```

## Usage

### Command Line Options

```bash
# Run all tests with coverage
./engine_test_runner --all --coverage --verbose

# Run specific test suites
./engine_test_runner --memory --physics --audio

# Run integration tests
./engine_test_runner --integration --verbose

# Run stress tests
./engine_test_runner --stress --verbose

# Generate coverage report with custom threshold
./engine_test_runner --all --coverage --threshold 85

# Custom output directory and log file
./engine_test_runner --all --output ./reports --log ./test.log
```

### Options Reference

| Option | Long Option | Description |
|---------|-------------|-------------|
| `-a` | `--all` | Run all test suites (default) |
| `-m` | `--memory` | Run memory tests |
| `-A` | `--audio` | Run audio tests |
| `-p` | `--physics` | Run physics tests |
| `-r` | `--rendering` | Run rendering tests |
| `-i` | `--ai` | Run AI tests |
| `-n` | `--animation` | Run animation tests |
| `-I` | `--input` | Run input tests |
| `-N` | `--network` | Run network tests |
| `-f` | `--filesystem` | Run filesystem tests |
| `-M` | `--math` | Run math tests |
| `-g` | `--integration` | Run integration tests |
| `-s` | `--stress` | Run stress tests |
| `-v` | `--verbose` | Enable verbose output |
| `-c` | `--coverage` | Generate coverage report |
| `-t FLOAT` | `--threshold FLOAT` | Minimum coverage threshold |
| `-o DIR` | `--output DIR` | Output directory for reports |
| `-l FILE` | `--log FILE` | Log file path |
| `-h` | `--help` | Show help message |

### Examples

#### Basic Usage
```bash
# Run all tests with default settings
./engine_test_runner

# Run with verbose output and coverage
./engine_test_runner --all --verbose --coverage
```

#### Targeted Testing
```bash
# Test only memory and physics systems
./engine_test_runner --memory --physics --verbose

# Run integration tests only
./engine_test_runner --integration --coverage
```

#### Stress Testing
```bash
# Run all stress tests
./engine_test_runner --stress --verbose

# Run stress tests with coverage analysis
./engine_test_runner --stress --coverage --threshold 75
```

#### Continuous Integration
```bash
# CI-friendly command with coverage requirements
./engine_test_runner --all --coverage --threshold 80 --output ./reports --log ./ci.log
```

## Test Reports

### Console Output
The framework provides detailed console output including:
- Individual test results
- Suite summaries
- Overall statistics
- Coverage percentages
- Performance metrics
- Error messages and stack traces

### JSON Reports
Structured JSON reports include:
- Test execution metadata
- Individual test results
- Coverage statistics
- Performance metrics
- Error details
- Timestamp information

### Coverage Reports
Coverage analysis provides:
- Line coverage percentages
- Function coverage statistics
- Branch coverage data
- Uncovered code identification
- Coverage trend analysis

## Architecture

### Framework Components

#### Core Framework
- **Test Runner**: Main execution engine
- **Assertion System**: Test validation macros
- **Result Collection**: Test result aggregation
- **Report Generation**: Output formatting

#### Test Infrastructure
- **Memory Tracking**: Allocation monitoring
- **Performance Timing**: High-precision timing
- **Thread Management**: Multi-threading support
- **Coverage Analysis**: Code coverage tracking

#### Test Utilities
- **Mock Systems**: Test doubles for dependencies
- **Data Generators**: Test data creation
- **Validation Helpers**: Common test patterns
- **Performance Benchmarks**: Standardized performance tests

### Test Structure

Each test follows this pattern:
```c
static bool test_function_name(void) {
    // Setup test conditions
    ASSERT_TRUE(condition);
    
    // Execute test logic
    result = function_under_test();
    
    // Validate results
    ASSERT_EQ(expected, result);
    
    // Cleanup
    ASSERT_NO_MEMORY_LEAKS();
    
    return true;
}
```

### Assertion Macros

#### Basic Assertions
- `ASSERT_TRUE(condition)` - Verify condition is true
- `ASSERT_FALSE(condition)` - Verify condition is false
- `ASSERT_EQ(expected, actual)` - Verify equality
- `ASSERT_NEQ(expected, actual)` - Verify inequality
- `ASSERT_NULL(ptr)` - Verify pointer is NULL
- `ASSERT_NOT_NULL(ptr)` - Verify pointer is not NULL

#### Floating Point Assertions
- `ASSERT_FLOAT_EQ(expected, actual, tolerance)` - Verify float equality with tolerance

#### Memory Assertions
- `ASSERT_NO_MEMORY_LEAKS()` - Verify no memory leaks

#### Performance Assertions
- `MEASURE_PERFORMANCE_START()` - Start performance measurement
- `MEASURE_PERFORMANCE_END(result_ns)` - End performance measurement

## Integration with CI/CD

### GitHub Actions Example
```yaml
name: Engine Tests
on: [push, pull_request]

jobs:
  test:
    runs-on: ubuntu-latest
    steps:
    - uses: actions/checkout@v2
    - name: Build Tests
      run: |
        mkdir build && cd build
        cmake ..
        make -j$(nproc)
    - name: Run Tests
      run: |
        cd build
        ./bin/engine_test_runner --all --coverage --threshold 80
    - name: Upload Reports
      uses: actions/upload-artifact@v2
      with:
        name: test-reports
        path: build/test_reports/
```

### Jenkins Pipeline Example
```groovy
pipeline {
    agent any
    stages {
        stage('Build') {
            steps {
                sh 'mkdir build && cd build'
                sh 'cmake ..'
                sh 'make -j$(nproc)'
            }
        }
        stage('Test') {
            steps {
                sh 'cd build'
                sh './bin/engine_test_runner --all --coverage --threshold 80 --output ./reports'
            }
            post {
                always {
                    publishHTML([
                        allowMissing: false,
                        alwaysLinkToLastBuild: true,
                        keepAll: true,
                        reportDir: 'build/test_reports',
                        reportFiles: 'coverage_report.html',
                        reportName: 'Coverage Report'
                    ])
                }
            }
        }
    }
}
```

## Contributing

### Adding New Tests

1. **Create Test Function**: Follow the naming convention `test_module_feature()`
2. **Add to Suite**: Include in appropriate test suite function
3. **Register Suite**: Add to main test runner
4. **Update Documentation**: Document test purpose and coverage

### Test Guidelines

- **Descriptive Names**: Use clear, descriptive test function names
- **Single Responsibility**: Each test should verify one specific behavior
- **Isolation**: Tests should not depend on each other
- **Cleanup**: Always clean up resources and verify no leaks
- **Documentation**: Comment complex test logic and edge cases

### Coverage Requirements

- **New Code**: All new code must have >80% test coverage
- **Critical Paths**: Critical code paths must have >95% coverage
- **Error Handling**: All error paths must be tested
- **Edge Cases**: Boundary conditions and edge cases must be covered

## Troubleshooting

### Common Issues

#### Build Failures
- Ensure CMake 3.16+ is installed
- Check compiler compatibility
- Verify pthread library availability

#### Test Failures
- Check test logs for detailed error messages
- Verify test environment setup
- Check for missing dependencies

#### Coverage Issues
- Ensure debug symbols are enabled
- Check compiler flags for coverage support
- Verify source file paths are correct

#### Memory Leaks
- Check test cleanup code
- Verify all allocations are freed
- Look for hidden allocations in dependencies

### Debug Mode

Enable debug mode for detailed information:
```bash
./engine_test_runner --all --verbose --coverage --threshold 0
```

This provides maximum output without failing on coverage thresholds.

## License

This test framework is part of the Minecraft v2 Engine project and follows the same licensing terms.

## Support

For questions, issues, or contributions:
- Create an issue in the project repository
- Check existing documentation and test examples
- Review test code for similar patterns and best practices

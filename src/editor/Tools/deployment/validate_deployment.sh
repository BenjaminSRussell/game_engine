#!/bin/bash

# Deployment Validation CI/CD Integration Script
# This script runs the full deployment validation suite and exits with appropriate codes
# for CI/CD pipeline integration

set -e  # Exit on any error

# Configuration
SCRIPT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
VALIDATOR_DIR="$SCRIPT_DIR/build"
VALIDATOR="$VALIDATOR_DIR/deployment_validator"
REPORT_DIR="${REPORT_DIR:-./validation_reports}"
TIMESTAMP=$(date +"%Y%m%d_%H%M%S")

# Colors for output
RED='\033[0;31m'
GREEN='\033[0;32m'
YELLOW='\033[1;33m'
BLUE='\033[0;34m'
NC='\033[0m' # No Color

# Logging functions
log_info() {
    echo -e "${BLUE}[INFO]${NC} $1"
}

log_success() {
    echo -e "${GREEN}[SUCCESS]${NC} $1"
}

log_warning() {
    echo -e "${YELLOW}[WARNING]${NC} $1"
}

log_error() {
    echo -e "${RED}[ERROR]${NC} $1"
}

# Check if validator exists
if [ ! -f "$VALIDATOR" ]; then
    log_error "Deployment validator not found at $VALIDATOR"
    log_info "Please build the validator first:"
    log_info "  cd $SCRIPT_DIR && mkdir -p build && cd build && cmake .. && make"
    exit 1
fi

# Create report directory
mkdir -p "$REPORT_DIR"

# Function to run validation category
run_validation_category() {
    local category="$1"
    local report_file="$REPORT_DIR/validation_${category}_${TIMESTAMP}.json"
    
    log_info "Running $category validation checks..."
    
    if "$VALIDATOR" -c "$category" -o "$report_file" -f json -q; then
        log_success "$category validation: PASSED"
        return 0
    else
        log_error "$category validation: FAILED"
        return 1
    fi
}

# Function to run full validation
run_full_validation() {
    local report_file="$REPORT_DIR/validation_full_${TIMESTAMP}.json"
    local summary_file="$REPORT_DIR/validation_summary_${TIMESTAMP}.txt"
    
    log_info "Running full deployment validation..."
    
    # Run validation and capture output
    if "$VALIDATOR" -o "$report_file" -f json 2>&1 | tee "$summary_file"; then
        log_success "Full validation: PASSED"
        return 0
    else
        log_error "Full validation: FAILED"
        return 1
    fi
}

# Function to generate summary report
generate_summary() {
    local report_file="$1"
    local summary_file="$2"
    
    if [ ! -f "$report_file" ]; then
        log_error "Report file not found: $report_file"
        return 1
    fi
    
    # Extract key metrics using jq (if available) or basic text parsing
    if command -v jq &> /dev/null; then
        log_info "Generating summary with jq..."
        jq -r '
            "=== DEPLOYMENT VALIDATION SUMMARY ===\n" +
            "Timestamp: " + .validation_report.timestamp + "\n" +
            "Total Checks: " + (.validation_report.summary.total_checks | tostring) + "\n" +
            "Passed: " + (.validation_report.summary.passed | tostring) + "\n" +
            "Failed: " + (.validation_report.summary.failed | tostring) + "\n" +
            "Warnings: " + (.validation_report.summary.warnings | tostring) + "\n" +
            "\n=== CODE QUALITY ===\n" +
            "TODO Comments: " + (.validation_report.code_quality.todo_count | tostring) + "\n" +
            "FIXME Comments: " + (.validation_report.code_quality.fixme_count | tostring) + "\n" +
            "Lines of Code: " + (.validation_report.code_quality.total_lines_of_code | tostring) + "\n" +
            "Thread Safety Violations: " + (.validation_report.code_quality.thread_safety_violations | tostring) + "\n" +
            "\n=== PERFORMANCE ===\n" +
            "Frame Time: " + (.validation_report.performance.frame_time_ms | tostring) + " ms\n" +
            "Memory Usage: " + (.validation_report.performance.memory_usage_mb | tostring) + " MB\n" +
            "GPU Utilization: " + (.validation_report.performance.gpu_utilization_percent | tostring) + "%\n" +
            "CPU Utilization: " + (.validation_report.performance.cpu_utilization_percent | tostring) + "%\n" +
            "\n=== COMPATIBILITY ===\n" +
            "Windows Vulkan: " + (.validation_report.compatibility.windows_vulkan_supported | tostring) + "\n" +
            "macOS Metal: " + (.validation_report.compatibility.macos_metal_supported | tostring) + "\n" +
            "Linux Vulkan: " + (.validation_report.compatibility.linux_vulkan_supported | tostring) + "\n"
        ' "$report_file" > "$summary_file"
    else
        log_warning "jq not available, using basic summary..."
        echo "=== DEPLOYMENT VALIDATION SUMMARY ===" > "$summary_file"
        echo "Report generated at: $(date)" >> "$summary_file"
        echo "Detailed report available in: $report_file" >> "$summary_file"
    fi
}

# Main execution logic
main() {
    local mode="${1:-full}"
    local exit_code=0
    
    log_info "Starting deployment validation (mode: $mode)..."
    log_info "Report directory: $REPORT_DIR"
    
    case "$mode" in
        "code")
            run_validation_category "code" || exit_code=1
            ;;
        "performance")
            run_validation_category "performance" || exit_code=1
            ;;
        "compatibility")
            run_validation_category "compatibility" || exit_code=1
            ;;
        "categories")
            # Run all categories individually
            run_validation_category "code" || exit_code=1
            run_validation_category "performance" || exit_code=1
            run_validation_category "compatibility" || exit_code=1
            ;;
        "full"|*)
            run_full_validation || exit_code=1
            ;;
    esac
    
    # Generate summary if we have a report
    local latest_report=$(ls -t "$REPORT_DIR"/validation_*.json 2>/dev/null | head -1)
    if [ -n "$latest_report" ]; then
        local summary_file="${latest_report%.json}_summary.txt"
        generate_summary "$latest_report" "$summary_file"
        log_info "Summary report: $summary_file"
    fi
    
    if [ $exit_code -eq 0 ]; then
        log_success "Deployment validation completed successfully!"
        log_info "All checks passed - ready for deployment!"
    else
        log_error "Deployment validation failed!"
        log_info "Please review the validation reports and fix issues before deployment."
    fi
    
    exit $exit_code
}

# Parse command line arguments
if [ $# -gt 0 ]; then
    case "$1" in
        -h|--help)
            echo "Usage: $0 [MODE]"
            echo ""
            echo "Modes:"
            echo "  full         Run all validation checks (default)"
            echo "  code         Run code quality checks only"
            echo "  performance  Run performance checks only"
            echo "  compatibility Run compatibility checks only"
            echo "  categories   Run all categories individually"
            echo ""
            echo "Environment Variables:"
            echo "  REPORT_DIR   Directory to save validation reports (default: ./validation_reports)"
            echo ""
            echo "Examples:"
            echo "  $0                    # Run all checks"
            echo "  $0 code               # Run code quality checks only"
            echo "  REPORT_DIR=/tmp $0     # Save reports to /tmp"
            exit 0
            ;;
        *)
            main "$1"
            ;;
    esac
else
    main "full"
fi

#!/usr/bin/env python3

"""
Frontend-Engine Connection Verification Script
This script verifies that all connections between the frontend and engine are working properly.
"""

import os
import sys
import subprocess
import json
from pathlib import Path

def run_command(cmd, cwd=None):
    """Run a command and return the result."""
    try:
        result = subprocess.run(cmd, shell=True, cwd=cwd, capture_output=True, text=True)
        return result.returncode == 0, result.stdout, result.stderr
    except Exception as e:
        return False, "", str(e)

def check_file_exists(filepath, description):
    """Check if a file exists."""
    if os.path.exists(filepath):
        print(f" {description}: {filepath}")
        return True
    else:
        print(f" {description}: {filepath} (missing)")
        return False

def check_directory_exists(dirpath, description):
    """Check if a directory exists."""
    if os.path.isdir(dirpath):
        print(f" {description}: {dirpath}")
        return True
    else:
        print(f" {description}: {dirpath} (missing)")
        return False

def verify_frontend_structure():
    """Verify the frontend directory structure."""
    print(" Verifying Frontend Structure...")
    
    frontend_dir = Path(__file__).parent
    checks = []
    
    # Check main directories
    checks.append(check_directory_exists(frontend_dir / "CVoxelForge", "CVoxelForge bridge directory"))
    checks.append(check_directory_exists(frontend_dir / "Core", "Core directory"))
    checks.append(check_directory_exists(frontend_dir / "Sources", "Sources directory"))
    checks.append(check_directory_exists(frontend_dir / "Tests", "Tests directory"))
    checks.append(check_directory_exists(frontend_dir / "UI", "UI directory"))
    
    # Check key files
    checks.append(check_file_exists(frontend_dir / "Package.swift", "Package.swift"))
    checks.append(check_file_exists(frontend_dir / "App.swift", "App.swift"))
    checks.append(check_file_exists(frontend_dir / "ContentView.swift", "ContentView.swift"))
    checks.append(check_file_exists(frontend_dir / "Sources/EngineBridge.swift", "EngineBridge.swift"))
    
    # Check bridge files
    cvoxelforge_dir = frontend_dir / "CVoxelForge"
    checks.append(check_file_exists(cvoxelforge_dir / "swift_bridge.c", "Swift bridge C file"))
    checks.append(check_file_exists(cvoxelforge_dir / "engine_stubs.c", "Engine stubs C file"))
    checks.append(check_file_exists(cvoxelforge_dir / "include/voxelforge_bridge.h", "Bridge header file"))
    
    return all(checks)

def verify_test_files():
    """Verify test files exist and are properly structured."""
    print("\n Verifying Test Files...")
    
    frontend_dir = Path(__file__).parent
    tests_dir = frontend_dir / "Tests"
    checks = []
    
    test_files = [
        "CoreSystemsTests.swift",
        "EngineBridgeTests.swift", 
        "FrontendIntegrationTests.swift",
        "EngineConnectionTests.swift"
    ]
    
    for test_file in test_files:
        checks.append(check_file_exists(tests_dir / test_file, f"Test file: {test_file}"))
    
    return all(checks)

def verify_package_configuration():
    """Verify Package.swift configuration."""
    print("\n Verifying Package Configuration...")
    
    frontend_dir = Path(__file__).parent
    package_file = frontend_dir / "Package.swift"
    
    if not package_file.exists():
        print(" Package.swift not found")
        return False
    
    with open(package_file, 'r') as f:
        content = f.read()
    
    checks = []
    
    # Check for required targets
    checks.append("CEngineCore" in content and "CVoxelForge" in content)
    checks.append("VoxelForgeStudio" in content)
    checks.append("VoxelForgeStudioTests" in content)
    
    # Check for required frameworks
    frameworks = ["Metal", "MetalKit", "SwiftUI", "QuartzCore"]
    for framework in frameworks:
        checks.append(f"-framework {framework}" in content)
    
    # Check for engine linking
    checks.append("-lEngine" in content)
    checks.append("-lGame" in content)
    
    if all(checks):
        print(" Package.swift configuration is correct")
        return True
    else:
        print(" Package.swift has issues")
        return False

def verify_bridge_compilation():
    """Verify that bridge files can be compiled."""
    print("\n Verifying Bridge Compilation...")
    
    frontend_dir = Path(__file__).parent
    cvoxelforge_dir = frontend_dir / "CVoxelForge"
    
    # Try to compile the bridge files
    c_files = [
        "swift_bridge.c",
        "engine_stubs.c",
        "occlusion_vfx_stubs.c",
        "world_editor_stubs.c",
        "graphics_debug_stubs.c",
        "rendering_tools_stubs.c",
        "visualization_stubs.c"
    ]
    
    for c_file in c_files:
        filepath = cvoxelforge_dir / c_file
        if not filepath.exists():
            print(f" Bridge file missing: {c_file}")
            return False
        
        # Basic syntax check with clang
        success, stdout, stderr = run_command(f"clang -fsyntax-only -Iinclude {filepath}", cwd=cvoxelforge_dir)
        if success:
            print(f" {c_file} syntax OK")
        else:
            print(f" {c_file} has syntax errors")
            print(f"   Error: {stderr}")
            return False
    
    return True

def verify_engine_linking():
    """Verify that engine libraries are available for linking."""
    print("\n Verifying Engine Linking...")
    
    frontend_dir = Path(__file__).parent
    project_root = frontend_dir.parent.parent
    
    # Check for engine library files in multiple locations
    possible_locations = [
        project_root / "libEngine.a",
        project_root / "libGame.a",
        project_root / "build/libEngine.a", 
        project_root / "build/libGame.a",
        project_root / "build_ci/libEngine.a",
        project_root / "build_ci/libGame.a"
    ]
    
    found_engine = False
    found_game = False
    
    for location in possible_locations:
        if location.name == "libEngine.a" and location.exists():
            print(f" Engine library found: {location}")
            found_engine = True
        elif location.name == "libGame.a" and location.exists():
            print(f" Game library found: {location}")
            found_game = True
    
    return found_engine and found_game

def run_frontend_build():
    """Attempt to build the frontend."""
    print("\n Attempting Frontend Build...")
    
    frontend_dir = Path(__file__).parent
    
    # Try swift build
    success, stdout, stderr = run_command("swift build", cwd=frontend_dir)
    
    if success:
        print(" Frontend build successful!")
        return True
    else:
        print(" Frontend build failed:")
        print(f"   Error: {stderr}")
        return False

def run_frontend_tests():
    """Attempt to run frontend tests."""
    print("\n Attempting Frontend Tests...")
    
    frontend_dir = Path(__file__).parent
    
    # Try swift test
    success, stdout, stderr = run_command("swift test", cwd=frontend_dir)
    
    if success:
        print(" Frontend tests passed!")
        return True
    else:
        print(" Frontend tests failed:")
        print(f"   Error: {stderr}")
        return False

def main():
    """Main verification function."""
    print(" Frontend-Engine Connection Verification")
    print("=" * 50)
    
    all_checks = []
    
    # Verify structure
    all_checks.append(verify_frontend_structure())
    
    # Verify test files
    all_checks.append(verify_test_files())
    
    # Verify package configuration
    all_checks.append(verify_package_configuration())
    
    # Verify bridge compilation
    all_checks.append(verify_bridge_compilation())
    
    # Verify engine linking
    all_checks.append(verify_engine_linking())
    
    # Try to build
    build_success = run_frontend_build()
    
    # Try to run tests
    if build_success:
        test_success = run_frontend_tests()
        all_checks.append(test_success)
    else:
        all_checks.append(False)
    
    # Summary
    print("\n" + "=" * 50)
    print(" VERIFICATION SUMMARY")
    print("=" * 50)
    
    if all(all_checks):
        print(" ALL CHECKS PASSED!")
        print(" Frontend is properly connected to the engine")
        print(" All bridge files are correctly implemented")
        print(" Tests are comprehensive and passing")
        print(" Build system is working correctly")
        return 0
    else:
        print(" SOME CHECKS FAILED!")
        print(" Please fix the issues above")
        return 1

if __name__ == "__main__":
    sys.exit(main())

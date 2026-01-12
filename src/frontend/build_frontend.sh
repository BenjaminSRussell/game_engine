#!/bin/bash

# Frontend Build and Test Script
# This script builds the frontend and runs comprehensive tests

echo "🚀 Building VoxelForge Frontend..."

# Navigate to frontend directory
cd "$(dirname "$0")"

# Clean previous build
echo "🧹 Cleaning previous build..."
rm -rf .build
rm -rf DerivedData

# Build the frontend
echo "🔨 Building frontend..."
swift build

# Check if build was successful
if [ $? -eq 0 ]; then
    echo "✅ Frontend build successful!"
    
    # Run tests
    echo "🧪 Running frontend tests..."
    swift test
    
    if [ $? -eq 0 ]; then
        echo "✅ All tests passed!"
        
        # Run specific test suites
        echo "🔍 Running Engine Bridge tests..."
        swift test --filter EngineBridgeTests
        
        echo "🔍 Running Integration tests..."
        swift test --filter FrontendIntegrationTests
        
        echo "🔍 Running Connection tests..."
        swift test --filter EngineConnectionTests
        
        echo "🎉 Frontend build and tests completed successfully!"
    else
        echo "❌ Some tests failed!"
        exit 1
    fi
else
    echo "❌ Frontend build failed!"
    exit 1
fi

echo "📊 Build Summary:"
echo "  - Frontend: Built and tested"
echo "  - Engine Bridge: Connected and tested"
echo "  - Integration: Verified"
echo "  - Tests: All passed"

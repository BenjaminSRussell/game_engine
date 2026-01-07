#!/bin/bash
# Generate the Xcode project via xcodegen (installs if missing).

set -e

SCRIPT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
PROJECT_DIR="$SCRIPT_DIR/.."

echo "Setting up Xcode project for Minecraft iOS..."

# Check if xcodegen is installed
if ! command -v xcodegen &> /dev/null; then
    echo "Installing xcodegen..."
    brew install xcodegen
fi

cd "$PROJECT_DIR"

# Create xcodegen config if it doesn't exist
if [ ! -f "project.yml" ]; then
    cat > project.yml << 'EOF'
name: Minecraft

settings:
  IPHONEOS_DEPLOYMENT_TARGET: "14.0"
  CODE_SIGN_IDENTITY: "Apple Development"

targets:
  Minecraft:
    type: application
    platform: iOS
    sources:
      - path: App/Sources
        excludes:
          - "**/*Tests*"
      - path: src
      - path: include
      - path: include_private
    resources:
      - path: App/Resources
      - path: assets
    settings:
      PRODUCT_NAME: Minecraft
      PRODUCT_BUNDLE_IDENTIFIER: com.minecraft.ios
      INFOPLIST_FILE: App/Resources/Info.plist
      SWIFT_OBJC_BRIDGING_HEADER: App/Sources/Minecraft-Bridging-Header.h
      HEADER_SEARCH_PATHS: "$(SRCROOT)/include $(SRCROOT)/include_private"
      GCC_C_LANGUAGE_DIALECT: "c11"
    dependencies:
      - framework: Metal
      - framework: MetalKit
      - framework: UIKit
      - framework: Foundation

schemes:
  Minecraft:
    build:
      targets:
        Minecraft: all
    run:
      config: Debug
    test:
      config: Debug
      targets:
        - Minecraft
EOF
    echo "Created project.yml"
fi

# Generate Xcode project
xcodegen generate

echo "Xcode project setup complete!"
echo "Open with: open Minecraft.xcodeproj"

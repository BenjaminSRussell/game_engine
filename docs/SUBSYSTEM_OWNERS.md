# Subsystem Ownership

To prevent duplicate work, each subsystem has designated coordination points.

## Core Systems
- Memory: @memory-team
- Logger: @core-team
- Threading: @core-team

## Physics
- Collision: @physics-team
- Constraints: @physics-team
- Integration: @physics-team

## Rendering
- GPU Memory: @rendering-team
- Pipelines: @rendering-team
- Vulkan/Metal: @rendering-team

## Animation
- Skeletal: @animation-team
- Procedural: @animation-team
- State Machines: @animation-team

## UI
- Widgets: @ui-team
- Layout: @ui-team
- Rendering: @ui-team

## Before Creating a Branch

1. Check this file for subsystem owner
2. Search existing branches: `git branch -r | grep <subsystem>`
3. Post in team channel about planned work
4. Get acknowledgment from subsystem owner

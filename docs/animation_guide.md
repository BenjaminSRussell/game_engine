# Animation System User Guide

## Overview

The animation system provides professional tools for creating and editing animations:
- **F-Curve Editor**: Edit animation curves with bezier handles
- **Dope Sheet**: Overview of all keyframes across tracks
- **NLA Editor**: Non-linear animation with action strips
- **Camera Sequencer**: Cinematic camera shots and transitions

## Getting Started

### Opening the Animation Timeline

In the Studio application, open the Animation panel:
```swift
// The AnimationTimelinePanel view is available in the main UI
AnimationTimelinePanel(timeline: animationTimeline)
```

### Switching Between Views

Use the segmented control at the top to switch between:
- **Timeline**: Classic track-based timeline
- **Dope Sheet**: Keyframe overview
- **F-Curve**: Graph editor for curves

---

## F-Curve Editor

### Purpose
Edit the interpolation curves between keyframes for smooth, natural animation.

### Basic Usage

1. **Select a track** in the track list (left side)
2. **View mode** → "F-Curve"
3. **Click keyframes** to select them (they turn white)
4. **Drag keyframes** to adjust timing and value
5. **Drag handles** (pink circles) to adjust curve shape

### Interpolation Types

- **Linear**: Straight line between keyframes
- **Bezier**: Smooth curve with adjustable handles
- **Constant**: Hold value until next keyframe

### Keyboard Shortcuts

- **Scroll/Pinch**: Zoom in/out
- **Drag background**: Pan the view
- **Click keyframe**: Select
- **Drag keyframe**: Move

### Tips

- Use **Bezier** interpolation for smooth, natural motion
- Adjust **handles** to control acceleration/deceleration
- **Zoom in** when making fine adjustments
- Enable **Snap to Frames** for precise timing

---

## Dope Sheet

### Purpose
See all keyframes across all tracks at once, perfect for timing adjustments.

### Basic Usage

1. **View mode** → "Dope Sheet"
2. **Scroll horizontally** to navigate timeline
3. **Click keyframes** to select
4. **Drag keyframes** to adjust timing

### Track Controls

Each track has controls:
- **Speaker icon**: Mute/unmute track
- **Lock icon**: Lock/unlock track (prevents editing)
- **Track name**: Click to select

### Tips

- Use for **timing refinement** after rough animation
- **Multi-select** keyframes for batch operations (future feature)
- **Color coding** helps identify track types

---

## NLA Editor

### Purpose
Compose complex animations from reusable action clips, blend multiple animations.

### Basic Usage

1. **View mode** → "NLA Editor" (not in view picker yet - access via separate panel)
2. **Click "Add Strip"** to create a test strip
3. **Drag strips** to reposition in time
4. **Resize handles** (black bars) to adjust duration

### Action Strips

Each strip represents an animation action:
- **Name**: Displayed on the strip
- **Start Time**: When the strip begins
- **Duration**: Length of the strip
- **Blend Mode**: How it combines (Replace, Add, Multiply, Crossfade)

### Workflow

1. Create individual animations (actions)
2. Arrange them as strips on the NLA tracks
3. Use blend modes to layer animations
4. Adjust timing with time remapping

### Tips

- **Orange strips** are easy to see
- Use **Replace** mode for primary animation
- Use **Add** mode to layer secondary motion
- **Time Scale** can speed up or slow down actions

---

## Camera Sequencer

### Purpose
Create cinematic camera sequences with multiple cameras, shots, and transitions.

### C API Usage

```c
#include "editor/sequence_editor/timeline.h"
#include "editor/sequence_editor/camera_sequencer.h"

// Initialize timeline
timeline_init(30.0); // 30 second duration

// Get camera sequencer
CameraSequencer* cam_seq = timeline_get_camera_sequencer();

// Add a camera track
uint32_t track_id = camera_sequencer_add_track(cam_seq, "Main Camera Track");
CameraTrack* track = camera_sequencer_get_track(cam_seq, track_id);

// Add shots
uint32_t shot1 = camera_track_add_shot(track, camera1_id, 0.0, 5.0);
uint32_t shot2 = camera_track_add_shot(track, camera2_id, 5.0, 5.0);

// Configure shot effects
CameraShot* shot = camera_track_get_shot(track, shot1);
camera_shot_set_dof(shot, 10.0f, 2.8f);  // focal_distance, aperture
camera_shot_set_shake(shot, 0.5f, 2.0f); // intensity, frequency

// Set transitions
shot->transition_type = TRANSITION_DISSOLVE;
shot->transition_duration = 0.5;

// Play
timeline_play();

// Update each frame
timeline_update(delta_time);
```

### Camera Effects

- **Depth of Field**: Blur background/foreground
  - `focal_distance`: Where focus is sharp
  - `aperture`: Amount of blur (lower = more blur)
  
- **Camera Shake**: Add handheld feel
  - `intensity`: Shake strength
  - `frequency`: Shake speed

- **Motion Blur**: Blur during fast movement
  - `motion_blur_amount`: Blur intensity

### Transitions

- `TRANSITION_CUT`: Instant switch
- `TRANSITION_DISSOLVE`: Cross-fade
- `TRANSITION_FADE_BLACK`: Fade to black, then in
- `TRANSITION_FADE_WHITE`: Fade to white, then in

### Composition Guides

Enable in shot settings:
- `show_rule_of_thirds`: Classic composition grid
- `show_safe_area`: TV/video safe zones
- `show_center_cross`: Center marker

---

## Playback Controls

All views share these controls:

- **Play/Pause**: Start/stop animation playback
- **Stop**: Reset to start
- **Frame Forward/Back**: Step one frame
- **Time Display**: Current time / Total duration
- **FPS Selector**: 24, 30, 60, or 120 FPS
- **Snap**: Snap to frame boundaries

---

## Best Practices

### Animation Workflow

1. **Block out** rough animation in Timeline view
2. **Refine timing** in Dope Sheet
3. **Polish curves** in F-Curve Editor
4. **Compose sequences** in NLA Editor

### Camera Sequencing

1. Set up multiple camera positions
2. Create shots with appropriate timing
3. Add transitions between shots
4. Apply effects (DoF, shake) for polish
5. Enable composition guides for framing

### Performance

- Disable unused tracks (mute them)
- Lock tracks you're not editing
- Use lower FPS for preview, higher for final
- Limit number of visible curves in F-Curve editor

---

## Troubleshooting

**Keyframes not appearing**
- Check track is not muted
- Verify correct time range
- Ensure track type matches data

**Curves look wrong**
- Check interpolation type (should be Bezier)
- Adjust tangent handles
- Zoom in for precision

**Camera sequencer not updating**
- Verify timeline is playing
- Check shot timing (start/end)
- Ensure camera IDs are valid

**Strips not visible in NLA**
- Check track expansion
- Verify strip start time is in view
- Ensure strip has valid action ID

---

## Advanced Topics

### Keyframe Types

The system supports multiple value types:
- `float`: Single number (opacity, scale)
- `vector3`: 3D position/rotation
- `color`: RGB color values
- `bool`: On/off states
- `event`: Trigger events

### Bezier Math

Cubic bezier curves use 4 control points:
- P0: Start keyframe
- P1: Out tangent handle
- P2: In tangent handle of next keyframe
- P3: End keyframe

The curve is evaluated as: B(t) = (1-t)³P0 + 3(1-t)²tP1 + 3(1-t)t²P2 + t³P3

### Handle Modes

- **Free**: Independent control of each handle
- **Aligned**: Handles opposite each other
- **Vector**: No curve, straight to next point
- **Automatic**: System calculates smooth curve

---

## Conclusion

The animation system provides professional-grade tools for creating everything from simple property animations to complex cinematic sequences. Experiment with different views and workflows to find what works best for your project.

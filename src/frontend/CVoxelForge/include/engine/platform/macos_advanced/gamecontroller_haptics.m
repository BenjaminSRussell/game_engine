/**
 * =================================================================================================
 *                          GAME CONTROLLER & HAPTICS
 * =================================================================================================
 *
 * Handles gamepad input (Xbox, PS5, Switch) and advanced haptic feedback.
 *
 * =================================================================================================
 */

#import <GameController/GameController.h>

// TODO(AGENT_MACOS_2): Implement controller discovery
//   - Listen for GCControllerDidConnectNotification
//   - Support multiple controllers
//   - Difficulty: 3

// TODO(AGENT_MACOS_2): Create unified input mapping
//   - Map buttons/sticks to engine generic input events
//   - Handle deadzones and sensitivity
//   - Difficulty: 4

// TODO(AGENT_MACOS_2): Implement advanced haptics (DualSense)
//   - Provide CHHapticEngine support
//   - Play transient and continuous haptic events
//   - Difficulty: 6

// TODO(AGENT_MACOS_2): Create adaptive trigger support
//   - Support DualSense adaptive triggers functionality (if available on macOS)
//   - Map weapon state to trigger resistance
//   - Difficulty: 7

// TODO(AGENT_MACOS_2): Implement lightbar control
//   - Set controller RGB color based on player health/team
//   - Difficulty: 2

// TODO(AGENT_MACOS_2): Create motion sensor support
//   - Read accelerometer/gyroscope from controller
//   - Map to camera or gameplay mechanics
//   - Difficulty: 4

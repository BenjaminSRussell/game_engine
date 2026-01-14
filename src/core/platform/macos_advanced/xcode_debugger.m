/**
 * =================================================================================================
 *                          XCODE DEBUGGER INTEGRATION
 * =================================================================================================
 *
 * Helpers for better debugging experience in Xcode (LLDB).
 * Custom data formatters and debug helpers.
 *
 * =================================================================================================
 */

#import <Foundation/Foundation.h>

// TODO(AGENT_MACOS_2): Implement debug description helpers
//   - Provide `debugDescription` methods for opaque structs
//   - Format vectors and matrices as strings
//   - Difficulty: 3

// TODO(AGENT_MACOS_2): Create memory guard helpers
//   - Scribble on free (0x55)
//   - Verify heap integrity command
//   - Difficulty: 4

// TODO(AGENT_MACOS_2): Implement LLDB summary providers
//   - Python scripts to format engine types in variable view
//   - (Note: This is external to C code but hooks are needed)
//   - Difficulty: 5

// TODO(AGENT_MACOS_2): Create assertion failure hook
//   - Trigger breakpoint instruction (__builtin_trap())
//   - Print stack trace to console
//   - Difficulty: 3

// TODO(AGENT_MACOS_2): Implement log routing to Xcode console
//   - Redirect stdout/stderr to ASL (Apple System Log)
//   - Difficulty: 2

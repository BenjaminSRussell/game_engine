# Input Subsystem Architecture

> **Priority 11** | Est. 500+ files | 50K+ LOC | ⭐⭐ Complexity

Based on design document: Unified input abstraction for game and editor.

---

## Directory Structure

```
Input/
├── Private/
├── Public/
│
├── Core/                       # Input core
│   ├── Manager/                # InputManager - central state
│   ├── State/                  # Current input state
│   └── Events/                 # Input events
│
├── Devices/                    # Input devices
│   ├── Keyboard/
│   │   ├── KeyCodes/
│   │   ├── TextInput/
│   │   └── Modifiers/
│   ├── Mouse/
│   │   ├── Buttons/
│   │   ├── Position/
│   │   ├── Delta/
│   │   └── Scroll/
│   ├── Gamepad/
│   │   ├── Buttons/
│   │   ├── Axes/
│   │   ├── Rumble/
│   │   └── Polling/
│   ├── Touch/
│   │   ├── Gesture/
│   │   └── Multitouch/
│   └── VR/
│       ├── Controllers/
│       └── Haptics/
│
├── Mapping/                    # Input mappings
│   ├── Actions/                # Action definitions (MoveForward, Jump)
│   ├── Axes/                   # Axis mappings
│   ├── Contexts/               # Input contexts (game, menu, editor)
│   └── Rebinding/              # Runtime rebinding
│
├── Platform/                   # Platform integration
│   ├── Windows/                # WM_KEYDOWN, RawInput
│   ├── macOS/                  # NSEvent integration
│   ├── Linux/                  # X11/Wayland events
│   └── iOS/
│
└── Editor/                     # Editor input routing
    ├── Viewport/               # Scene manipulation input
    ├── PlayMode/               # Forward to game
    └── Shortcuts/              # Editor keyboard shortcuts
```

---

## Design Notes (from design doc)

> "The Input system might update states (which keys are down, mouse position, etc.) 
> and offer methods like isKeyPressed(KEY_W) or higher-level action mapping."

> "For the Swift editor, we'll need to forward events into the engine when the user 
> is controlling the game or manipulating the scene in the viewport."

---

## Key Files

- `input_manager.c/h` - Central state, query methods
- `key_codes.h` - Unified key enumeration
- `input_mappings.c/h` - Raw input → action mapping
- `platform_input_*.c` - Platform-specific event capture

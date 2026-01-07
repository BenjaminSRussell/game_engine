# =================================================================================================
#                          BLENDER ADDON REGISTRATION
# =================================================================================================
#
# Main entry point for the Blender integration plugin.
#
# =================================================================================================

import bpy

# TODO(AGENT_BLENDER_2): Implement AddonProperties
#   - Engine Path setting
#   - Export Directory setting
#   - Difficulty: 2

# TODO(AGENT_BLENDER_2): Register/Unregister classes
#   - Register all operators and panels
#   - Difficulty: 2

bl_info = {
    "name": "Game Engine Tools",
    "blender": (3, 0, 0),
    "category": "Game Engine",
}

def register():
    # TODO(AGENT_BLENDER_2): Registration logic
    pass

def unregister():
    # TODO(AGENT_BLENDER_2): Unregistration logic
    pass

if __name__ == "__main__":
    register()

# cmake/recovered_sources.cmake
# Recovered and integrated sources from the cleanup deletion

# ===========================================
# RECOVERED SCRIPTING SYSTEMS
# ===========================================
set(RECOVERED_SCRIPTING_SOURCES
    "src/engine/scripting/scripts/create_3d_models.py"
    "src/engine/scripting/scripts/create_block_textures.py"
    "src/engine/scripting/scripts/create_placeholder_assets.py"
    "src/engine/scripting/scripts/create_ui_assets.py"
    "src/engine/scripting/scripts/download_assets.py"
    "src/engine/scripting/scripts/download_production_assets.py"
    "src/engine/scripting/scripts/generate_asset_todos.py"
    "src/engine/scripting/scripts/generate_block_skins.py"
    "src/engine/scripting/scripts/replace_assets.py"
    "src/engine/scripting/scripts/validate_assets.py"
    "src/engine/scripts/fix_includes.py"
)

# ===========================================
# RECOVERED BLENDER INTEGRATION TOOLS
# ===========================================
set(RECOVERED_BLENDER_SOURCES
    "src/engine/core/integration/blender_addon/__init__.py"
    "src/engine/core/integration/blender_addon/collision_generator.py"
    "src/engine/core/integration/blender_addon/engine_panel.py"
    "src/engine/core/integration/blender_addon/export_operators.py"
    "src/engine/core/integration/blender_addon/live_link_client.py"
    "src/engine/core/integration/blender_addon/lod_generator.py"
    "src/engine/core/integration/blender_addon/material_baker.py"
    "src/engine/tools/blender_scripts/auto_rig_character.py"
    "src/engine/tools/blender_scripts/export_animations.py"
    "src/engine/tools/blender_scripts/generate_walk_cycle.py"
    "src/engine/tools/blender_scripts/import_image_as_mesh.py"
)

# ===========================================
# RECOVERED EXPERIMENTAL PHYSICS
# ===========================================
set(RECOVERED_EXPERIMENTAL_PHYSICS
    "src/engine/physics/experimental/advanced_physics_complete.c"
    "src/engine/physics/experimental/cloth_system.c"
    "src/engine/physics/experimental/complete_physics_batch.c"
    "src/engine/physics/experimental/final_systems_batch.c"
    "src/engine/physics/experimental/fluid_simulation.c"
    "src/engine/physics/experimental/physics_final_batch.c"
    "src/engine/physics/experimental/soft_body_solver.c"
    "src/engine/physics/experimental/vehicle_controller.c"
    "src/engine/physics/experimental/lift_drag.c"
    "src/engine/physics/experimental/wind_field.c"
)

# ===========================================
# RECOVERED FRONTEND TOOLS
# ===========================================
set(RECOVERED_FRONTEND_SOURCES
    "src/frontend/verify_connections.py"
)

list(LENGTH RECOVERED_SCRIPTING_SOURCES SCRIPTING_COUNT)
list(LENGTH RECOVERED_BLENDER_SOURCES BLENDER_COUNT)
list(LENGTH RECOVERED_EXPERIMENTAL_PHYSICS PHYSICS_COUNT)
list(LENGTH RECOVERED_FRONTEND_SOURCES FRONTEND_COUNT)

message(STATUS "Recovered ${SCRIPTING_COUNT} scripting files")
message(STATUS "Recovered ${BLENDER_COUNT} Blender integration files")
message(STATUS "Recovered ${PHYSICS_COUNT} experimental physics files")
message(STATUS "Recovered ${FRONTEND_COUNT} frontend tools")

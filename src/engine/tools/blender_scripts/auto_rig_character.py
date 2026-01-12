#!/usr/bin/env python3
"""
Blender Python Script: Auto-Rigging System
Purpose: Automatically rig character meshes with skeletal animation
Usage: blender -b -P auto_rig_character.py -- --input mesh.glb --output rigged.glb --rig_type human
"""

import bpy
import sys
import os
import math
import bmesh

def parse_arguments():
    """Parse command line arguments"""
    args = {}
    for i, arg in enumerate(sys.argv):
        if arg == "--" and i + 1 < len(sys.argv):
            # Parse arguments after --
            remaining = sys.argv[i + 1:]
            for j in range(0, len(remaining), 2):
                if j + 1 < len(remaining):
                    key = remaining[j].lstrip('-')
                    value = remaining[j + 1]
                    args[key] = value
            break
    return args

def clear_default_scene():
    """Clear default scene"""
    bpy.ops.object.select_all(action='SELECT')
    bpy.ops.object.delete()

def import_mesh(mesh_path):
    """TASK_020: Import mesh into Blender"""
    if not os.path.exists(mesh_path):
        raise FileNotFoundError(f"Mesh file not found: {mesh_path}")
    
    # Import GLB/GLTF file
    bpy.ops.import_scene.gltf(filepath=mesh_path)
    
    # Get the imported mesh object
    mesh_objects = [obj for obj in bpy.context.selected_objects if obj.type == 'MESH']
    if not mesh_objects:
        raise ValueError("No mesh objects found in imported file")
    
    return mesh_objects[0]

def analyze_mesh_bounds(mesh_obj):
    """TASK_021: Analyze mesh bounds and determine if humanoid"""
    # Get mesh dimensions
    bpy.context.view_layer.objects.active = mesh_obj
    
    # Calculate bounding box
    bbox_min = [float('inf')] * 3
    bbox_max = [float('-inf')] * 3
    
    for vert in mesh_obj.data.vertices:
        world_pos = mesh_obj.matrix_world @ vert.co
        for i in range(3):
            bbox_min[i] = min(bbox_min[i], world_pos[i])
            bbox_max[i] = max(bbox_max[i], world_pos[i])
    
    # Calculate dimensions
    width = bbox_max[0] - bbox_min[0]
    height = bbox_max[2] - bbox_min[2]  # Z is up in Blender
    depth = bbox_max[1] - bbox_min[1]
    
    # Calculate center
    center = [(bbox_min[i] + bbox_max[i]) / 2 for i in range(3)]
    
    # Determine if humanoid based on proportions
    # Humanoid typically: height > width, height > depth, and reasonable proportions
    aspect_ratio = height / max(width, depth) if max(width, depth) > 0 else 0
    
    is_humanoid = (aspect_ratio > 1.5 and aspect_ratio < 10.0 and 
                  height > 0.5 and height < 3.0)  # Assuming units are meters
    
    print(f"Mesh Analysis:")
    print(f"  Dimensions: W={width:.3f}, H={height:.3f}, D={depth:.3f}")
    print(f"  Aspect Ratio: {aspect_ratio:.2f}")
    print(f"  Humanoid: {is_humanoid}")
    
    return {
        'width': width,
        'height': height,
        'depth': depth,
        'center': center,
        'is_humanoid': is_humanoid,
        'aspect_ratio': aspect_ratio
    }

def create_metarig(rig_type="human", mesh_height=1.0):
    """TASK_022: Generate metarig using Rigify"""
    # Enable Rigify addon if not already enabled
    if "Rigify" not in bpy.context.preferences.addons:
        bpy.ops.preferences.addon_enable(module="Rigify")
    
    # Create metarig
    bpy.ops.armature.metarig_add()
    
    metarig = bpy.context.active_object
    metarig.name = "Metarig"
    
    # Select appropriate rig type based on analysis
    if rig_type == "human" or rig_type == "humanoid":
        # Use the default human metarig
        # The metarig_add() operator creates a human metarig by default
        pass
    elif rig_type == "quadruped":
        # For quadrupeds, we'd need a different metarig template
        # For now, we'll use the human metarig as a base
        print("Warning: Quadruped rigging not fully implemented, using human template")
    else:
        print(f"Unknown rig type: {rig_type}, using human template")
    
    return metarig

def scale_metarig_to_mesh(metarig, mesh_bounds):
    """TASK_023: Scale metarig to match mesh height"""
    mesh_height = mesh_bounds['height']
    metarig_height = 1.0  # Default metarig height
    
    # Calculate scale factor
    scale_factor = mesh_height / metarig_height
    
    # Apply scaling
    metarig.scale = (scale_factor, scale_factor, scale_factor)
    
    # Position metarig at mesh center
    metarig.location = mesh_bounds['center']
    
    # Apply transforms
    bpy.context.view_layer.objects.active = metarig
    bpy.ops.object.transform_apply(location=False, rotation=False, scale=True)
    
    print(f"Scaled metarig by factor: {scale_factor:.3f}")
    
    return metarig

def parent_mesh_to_rig(mesh_obj, rig_obj, automatic_weights=True):
    """TASK_024: Parent mesh to rig with automatic weights"""
    # Select both objects
    mesh_obj.select_set(True)
    rig_obj.select_set(True)
    rig_obj.select_set(True)  # Make rig the active object
    
    bpy.context.view_layer.objects.active = rig_obj
    
    if automatic_weights:
        # Parent with automatic weights
        bpy.ops.object.parent_set(type='ARMATURE_AUTO')
        print("Parented mesh to rig with automatic weights")
    else:
        # Parent without weights (manual weight painting required)
        bpy.ops.object.parent_set(type='ARMATURE_ENVELOPE')
        print("Parented mesh to rig with envelope weights")
    
    # Deselect objects
    mesh_obj.select_set(False)
    rig_obj.select_set(False)
    
    return rig_obj

def generate_rigify_rig(metarig):
    """Generate the final rig from metarig"""
    bpy.context.view_layer.objects.active = metarig
    
    # Generate the rig
    bpy.ops.pose.rigify_generate()
    
    # Find the generated rig (should be named "rig")
    rig_obj = None
    for obj in bpy.context.scene.objects:
        if obj.type == 'ARMATURE' and obj.name.startswith("rig"):
            rig_obj = obj
            break
    
    if not rig_obj:
        raise RuntimeError("Failed to generate rigify rig")
    
    # Hide the metarig
    metarig.hide_viewport = True
    metarig.hide_render = True
    
    return rig_obj

def export_rigged_mesh(output_path):
    """TASK_025: Export rig + mesh as GLB"""
    # Ensure the output directory exists
    output_dir = os.path.dirname(output_path)
    if output_dir and not os.path.exists(output_dir):
        os.makedirs(output_dir)
    
    # Select all objects we want to export
    bpy.ops.object.select_all(action='DESELECT')
    
    for obj in bpy.context.scene.objects:
        if obj.type in ['MESH', 'ARMATURE'] and not obj.hide_viewport:
            obj.select_set(True)
    
    # Export to GLB
    bpy.ops.export_scene.gltf(
        filepath=output_path,
        export_format='GLB',
        use_selection=True,
        export_texcoords=True,
        export_normals=True,
        export_materials='EXPORT',
        export_colors=True,
        export_animations=True,
        export_apply=True,
        export_yup=True
    )

def main():
    """Main auto-rigging process"""
    print("Starting auto-rigging process...")
    
    # Parse arguments
    args = parse_arguments()
    
    input_path = args.get('input', '')
    output_path = args.get('output', '')
    rig_type = args.get('rig_type', 'human')
    height_scale = float(args.get('height_scale', '1.0'))
    automatic_weights = args.get('automatic_weights', 'true').lower() == 'true'
    
    if not input_path or not output_path:
        print("Error: Both --input and --output arguments are required")
        sys.exit(1)
    
    try:
        # Clear scene
        clear_default_scene()
        print("Cleared default scene")
        
        # TASK_020: Import mesh
        mesh_obj = import_mesh(input_path)
        print(f"Imported mesh: {input_path}")
        
        # TASK_021: Analyze mesh bounds
        mesh_bounds = analyze_mesh_bounds(mesh_obj)
        
        # Override rig type if mesh is clearly not humanoid
        if rig_type == "human" and not mesh_bounds['is_humanoid']:
            print("Mesh doesn't appear humanoid, but proceeding with human rig as requested")
        
        # TASK_022: Create metarig
        metarig = create_metarig(rig_type, mesh_bounds['height'])
        print(f"Created {rig_type} metarig")
        
        # TASK_023: Scale metarig to match mesh
        metarig = scale_metarig_to_mesh(metarig, mesh_bounds)
        
        # Apply height scale if specified
        if height_scale != 1.0:
            metarig.scale = (height_scale, height_scale, height_scale)
            bpy.context.view_layer.objects.active = metarig
            bpy.ops.object.transform_apply(location=False, rotation=False, scale=True)
            print(f"Applied additional height scale: {height_scale}")
        
        # Generate the final rig
        rig_obj = generate_rigify_rig(metarig)
        print("Generated rigify rig")
        
        # TASK_024: Parent mesh to rig
        parent_mesh_to_rig(mesh_obj, rig_obj, automatic_weights)
        
        # TASK_025: Export rigged mesh
        export_rigged_mesh(output_path)
        print(f"Exported rigged mesh to: {output_path}")
        
        print("Auto-rigging completed successfully!")
        
    except Exception as e:
        print(f"Error during auto-rigging: {e}")
        sys.exit(1)

if __name__ == "__main__":
    main()

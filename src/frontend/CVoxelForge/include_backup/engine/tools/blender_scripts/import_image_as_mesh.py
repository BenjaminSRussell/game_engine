#!/usr/bin/env python3
"""
Blender Python Script: Image to 2.5D Mesh Conversion
Purpose: Convert 2D images to 3D meshes with thickness for 2.5D assets
Usage: blender -b -P import_image_as_mesh.py -- --input image.png --output mesh.glb
"""

import bpy
import sys
import os
import bmesh
import math

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
    """TASK_011: Clear default scene"""
    bpy.ops.object.select_all(action='SELECT')
    bpy.ops.object.delete()
    # Also clear materials
    bpy.ops.material.select_all(action='SELECT')
    bpy.ops.material.delete()

def import_image_as_plane(image_path, shadeless=False):
    """TASK_012: Import image as plane"""
    # Load the image
    if not os.path.exists(image_path):
        raise FileNotFoundError(f"Image file not found: {image_path}")
    
    # Import image as plane
    bpy.ops.import_image.to_plane(
        filepath=image_path,
        shadeless=shadeless,
        use_transparency=True,
        align='VIEW',
        size=1.0
    )
    
    # Get the imported plane object
    plane_obj = bpy.context.active_object
    return plane_obj

def apply_alpha_cutout(obj):
    """TASK_013: Apply alpha cutout using knife project"""
    # Get the material and texture
    if not obj.data.materials:
        return obj
    
    mat = obj.data.materials[0]
    if not mat.node_tree:
        return obj
    
    # Find the image texture node
    image_node = None
    for node in mat.node_tree.nodes:
        if node.type == 'TEX_IMAGE':
            image_node = node
            break
    
    if not image_node or not image_node.image:
        return obj
    
    image = image_node.image
    
    # Check if image has alpha channel
    if image.channels < 4:
        return obj
    
    # Create a new mesh with alpha cutout
    # For simplicity, we'll use a threshold-based approach
    # In a production system, you'd want more sophisticated alpha processing
    
    # Switch to edit mode
    bpy.context.view_layer.objects.active = obj
    bpy.ops.object.mode_set(mode='EDIT')
    
    # Get the mesh data
    bm = bmesh.from_edit_mesh(obj.data)
    
    # Create UV map if it doesn't exist
    if not obj.data.uv_layers:
        obj.data.uv_layers.new(name='UVMap')
    
    uv_layer = bm.loops.layers.uv.active
    
    # Simple alpha threshold - remove faces with low alpha
    threshold = 0.1
    faces_to_remove = []
    
    for face in bm.faces:
        # Sample alpha at face center
        total_alpha = 0
        sample_count = 0
        
        for loop in face.loops:
            uv = loop[uv_layer].uv
            # Convert UV to pixel coordinates
            px = int(uv.x * image.size[0])
            py = int(uv.y * image.size[1])
            
            # Clamp to image bounds
            px = max(0, min(px, image.size[0] - 1))
            py = max(0, min(py, image.size[1] - 1))
            
            # Get pixel alpha
            pixel = image.pixels[ (py * image.size[0] + px) * image.channels + 3]
            total_alpha += pixel
            sample_count += 1
        
        avg_alpha = total_alpha / sample_count if sample_count > 0 else 1.0
        
        if avg_alpha < threshold:
            faces_to_remove.append(face)
    
    # Remove low-alpha faces
    for face in faces_to_remove:
        bm.faces.remove(face)
    
    # Update mesh
    bmesh.update_edit_mesh(obj.data)
    
    # Return to object mode
    bpy.ops.object.mode_set(mode='OBJECT')
    
    return obj

def apply_solidify(obj, thickness=0.1):
    """TASK_014: Apply solidify modifier for thickness"""
    # Add solidify modifier
    solidify_mod = obj.modifiers.new(name="Solidify", type='SOLIDIFY')
    solidify_mod.thickness = thickness
    solidify_mod.offset = 1.0  # Expand outward
    solidify_mod.use_even_offset = True
    solidify_mod.use_quality_normals = True
    
    return obj

def setup_auto_uv(obj):
    """TASK_015: Setup automatic UV projection"""
    # Ensure we have UV coordinates
    if not obj.data.uv_layers:
        obj.data.uv_layers.new(name='UVMap')
    
    # Smart UV Project for better unwrapping
    bpy.context.view_layer.objects.active = obj
    bpy.ops.object.mode_set(mode='EDIT')
    
    # Select all faces
    bpy.ops.mesh.select_all(action='SELECT')
    
    # Smart UV project
    bpy.ops.uv.smart_project(
        angle_limit=66.0,
        island_margin=0.01,
        user_area_weight=0.0,
        use_aspect_normalization=True,
        stretch_to_bounds=False
    )
    
    # Return to object mode
    bpy.ops.object.mode_set(mode='OBJECT')
    
    return obj

def export_to_glb(output_path):
    """TASK_016: Export scene to GLB format"""
    # Ensure the output directory exists
    output_dir = os.path.dirname(output_path)
    if output_dir and not os.path.exists(output_dir):
        os.makedirs(output_dir)
    
    # Export to GLB
    bpy.ops.export_scene.gltf(
        filepath=output_path,
        export_format='GLB',
        use_selection=False,
        export_texcoords=True,
        export_normals=True,
        export_materials='EXPORT',
        export_colors=True,
        export_animations=True,
        export_apply=True,
        export_yup=True
    )

def main():
    """Main conversion process"""
    print("Starting image to mesh conversion...")
    
    # Parse arguments
    args = parse_arguments()
    
    input_path = args.get('input', '')
    output_path = args.get('output', '')
    thickness = float(args.get('thickness', '0.1'))
    alpha_cutout = args.get('alpha_cutout', 'true').lower() == 'true'
    auto_uv = args.get('auto_uv', 'true').lower() == 'true'
    shadeless = args.get('shadeless', 'false').lower() == 'true'
    
    if not input_path or not output_path:
        print("Error: Both --input and --output arguments are required")
        sys.exit(1)
    
    try:
        # TASK_011: Clear default scene
        clear_default_scene()
        print("Cleared default scene")
        
        # TASK_012: Import image as plane
        plane_obj = import_image_as_plane(input_path, shadeless=shadeless)
        print(f"Imported image: {input_path}")
        
        # TASK_013: Apply alpha cutout if requested
        if alpha_cutout:
            plane_obj = apply_alpha_cutout(plane_obj)
            print("Applied alpha cutout")
        
        # TASK_014: Apply solidify modifier
        plane_obj = apply_solidify(plane_obj, thickness)
        print(f"Applied solidify with thickness: {thickness}")
        
        # TASK_015: Setup automatic UV
        if auto_uv:
            plane_obj = setup_auto_uv(plane_obj)
            print("Setup automatic UV projection")
        
        # TASK_016: Export to GLB
        export_to_glb(output_path)
        print(f"Exported mesh to: {output_path}")
        
        print("Conversion completed successfully!")
        
    except Exception as e:
        print(f"Error during conversion: {e}")
        sys.exit(1)

if __name__ == "__main__":
    main()

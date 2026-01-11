import bpy
import argparse
import sys
import os

def convert_image_to_mesh(input_path, output_path, thickness, alpha_cutout, auto_uv, shadeless):
    # Clear existing data
    bpy.ops.wm.read_factory_settings(use_empty=True)
    
    # Import image as plane (requires 'Import-Export: Import Images as Planes' addon enabled essentially, 
    # but we can do it manually if addon logic is complex to invoke from headless. 
    # Actually, let's simple create a plane and assign material.)
    
    # Create plane
    bpy.ops.mesh.primitive_plane_add(size=2, enter_editmode=False, align='WORLD', location=(0, 0, 0))
    obj = bpy.context.active_object
    obj.name = "ImageMesh"
    
    # Create material
    mat = bpy.data.materials.new(name="ImageMaterial")
    mat.use_nodes = True
    bsdf = mat.node_tree.nodes["Principled BSDF"]
    
    # Load image
    try:
        img = bpy.data.images.load(input_path)
    except:
        print(f"Error loading image: {input_path}")
        return False
        
    # Connect texture
    tex_image = mat.node_tree.nodes.new('ShaderNodeTexImage')
    tex_image.image = img
    mat.node_tree.links.new(bsdf.inputs['Base Color'], tex_image.outputs['Color'])
    mat.node_tree.links.new(bsdf.inputs['Alpha'], tex_image.outputs['Alpha'])
    
    # Alpha settings
    if alpha_cutout:
        mat.blend_method = 'CLIP'
        mat.shadow_method = 'CLIP'
    
    # Assign material
    if obj.data.materials:
        obj.data.materials[0] = mat
    else:
        obj.data.materials.append(mat)
    
    # Solidify
    if thickness > 0:
        mod = obj.modifiers.new(name="Solidify", type='SOLIDIFY')
        mod.thickness = thickness
        mod.offset = 0
    
    # UV Map (Plane comes with UVs, but we can preserve or scale them)
    # Default plane UVs match the image 1:1 usually.
    
    # Export
    bpy.ops.export_scene.gltf(filepath=output_path, export_format='GLB')
    return True

if __name__ == "__main__":
    # Remove blender args
    if "--" in sys.argv:
        argv = sys.argv[sys.argv.index("--") + 1:]
    else:
        argv = []

    parser = argparse.ArgumentParser()
    parser.add_argument("--input", required=True)
    parser.add_argument("--output", required=True)
    parser.add_argument("--thickness", type=float, default=0.1)
    parser.add_argument("--alpha_cutout", type=str, default="true")
    parser.add_argument("--auto_uv", type=str, default="true")
    parser.add_argument("--shadeless", type=str, default="false")
    
    args = parser.parse_args(argv)
    
    success = convert_image_to_mesh(
        args.input,
        args.output,
        args.thickness,
        args.alpha_cutout.lower() == "true",
        args.auto_uv.lower() == "true",
        args.shadeless.lower() == "true"
    )
    
    if not success:
        sys.exit(1)

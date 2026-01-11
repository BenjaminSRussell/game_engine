import bpy
import argparse
import sys
import os

def auto_rig_character(input_path, output_path, rig_type, height_scale, auto_weights):
    bpy.ops.wm.read_factory_settings(use_empty=True)
    
    # Import input mesh
    ext = os.path.splitext(input_path)[1].lower()
    if ext == '.glb' or ext == '.gltf':
        bpy.ops.import_scene.gltf(filepath=input_path)
    elif ext == '.obj':
        bpy.ops.import_scene.obj(filepath=input_path)
    elif ext == '.fbx':
        bpy.ops.import_scene.fbx(filepath=input_path)
    else:
        print(f"Unsupported file format: {ext}")
        return False
        
    char_obj = None
    for obj in bpy.context.scene.objects:
        if obj.type == 'MESH':
            char_obj = obj
            break
            
    if not char_obj:
        print("No mesh found in input file")
        return False
        
    # Add Armature
    bpy.ops.object.armature_add(enter_editmode=False, location=(0, 0, 0))
    armature = bpy.context.active_object
    armature.name = "RootArmature"
    
    # Simple humanoid structure for "human" rig_type
    if rig_type == 'human':
        # This creates a very basic bone structure. 
        # In a real tool we'd use Rigify or append a template rig.
        bpy.ops.object.mode_set(mode='EDIT')
        bones = armature.data.edit_bones
        
        # Rename root bone to Hips
        root = bones['Bone']
        root.name = "Hips"
        root.head = (0, 0, height_scale * 1.0)
        root.tail = (0, 0, height_scale * 1.1)
        
        # Add basic spine and head
        spine = bones.new("Spine")
        spine.head = root.tail
        spine.tail = (0, 0, height_scale * 1.5)
        spine.parent = root
        
        head = bones.new("Head")
        head.head = spine.tail
        head.tail = (0, 0, height_scale * 1.7)
        head.parent = spine
        
        # Legs (simplified)
        l_leg = bones.new("Leg_L")
        l_leg.head = root.head
        l_leg.tail = (height_scale * 0.2, 0, height_scale * 0.5)
        l_leg.parent = root
        
        r_leg = bones.new("Leg_R")
        r_leg.head = root.head
        r_leg.tail = (height_scale * -0.2, 0, height_scale * 0.5)
        r_leg.parent = root
        
        bpy.ops.object.mode_set(mode='OBJECT')
    
    # Parent and Weight
    if auto_weights:
        bpy.ops.object.select_all(action='DESELECT')
        char_obj.select_set(True)
        armature.select_set(True)
        bpy.context.view_layer.objects.active = armature
        
        bpy.ops.object.parent_set(type='ARMATURE_AUTO')
    
    # Export
    bpy.ops.export_scene.gltf(filepath=output_path, export_format='GLB')
    return True

if __name__ == "__main__":
    if "--" in sys.argv:
        argv = sys.argv[sys.argv.index("--") + 1:]
    else:
        argv = []

    parser = argparse.ArgumentParser()
    parser.add_argument("--input", required=True)
    parser.add_argument("--output", required=True)
    parser.add_argument("--rig_type", type=str, default="human")
    parser.add_argument("--height_scale", type=float, default=1.0)
    parser.add_argument("--automatic_weights", type=str, default="true")
    
    args = parser.parse_args(argv)
    
    success = auto_rig_character(
        args.input,
        args.output,
        args.rig_type,
        args.height_scale,
        args.automatic_weights.lower() == "true"
    )
    
    if not success:
        sys.exit(1)

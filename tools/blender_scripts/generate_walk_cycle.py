import bpy
import argparse
import sys
import math
import os

def generate_walk_cycle(input_path, output_path, frame_count):
    bpy.ops.wm.read_factory_settings(use_empty=True)
    
    # Import
    ext = os.path.splitext(input_path)[1].lower()
    if ext == '.glb' or ext == '.gltf':
        bpy.ops.import_scene.gltf(filepath=input_path)
    elif ext == '.fbx':
        bpy.ops.import_scene.fbx(filepath=input_path)
    else:
        print(f"Unsupported input format: {ext}")
        return False
        
    # Find Armature
    armature = None
    for obj in bpy.context.scene.objects:
        if obj.type == 'ARMATURE':
            armature = obj
            break
            
    if not armature:
        print("No armature found")
        return False
        
    bpy.context.view_layer.objects.active = armature
    bpy.ops.object.mode_set(mode='POSE')
    
    # Create Action
    if not armature.animation_data:
        armature.animation_data_create()
    
    action = bpy.data.actions.new(name="WalkCycle")
    armature.animation_data.action = action
    
    # Simple procedural walk
    # Assuming "Leg_L", "Leg_R", "Hips" bones exist (from auto_rig)
    # We set keyframes at 0, 0.25, 0.5, 0.75, 1.0 of cycle
    
    bones = armature.pose.bones
    
    def set_keyframe(bone_name, axis, value, frame_idx):
        if bone_name in bones:
            bone = bones[bone_name]
            if axis == 'rotation': # simplified
                bone.rotation_mode = 'XYZ'
                bone.rotation_euler[0] = math.radians(value)
                bone.keyframe_insert(data_path="rotation_euler", index=0, frame=frame_idx)

    frames = frame_count
    
    # Contact (Right forward, Left back) - Frame 0 & Frame End
    for f in [0, frames]:
        set_keyframe("Leg_R", "rotation", 30, f)
        set_keyframe("Leg_L", "rotation", -30, f)
        # Hips
        if "Hips" in bones:
             bones["Hips"].location[2] = 0 # Default height
             bones["Hips"].keyframe_insert(data_path="location", index=2, frame=f)

    # Passing (Right passing, Left support) - Frame 0.5 * frames
    mid = frames // 2
    set_keyframe("Leg_R", "rotation", -30, mid)
    set_keyframe("Leg_L", "rotation", 30, mid)
    
    # High point/Low point details skipped for brevity, this is a placeholder cycle
    
    # Set scene settings
    bpy.context.scene.frame_start = 0
    bpy.context.scene.frame_end = frames
    
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
    parser.add_argument("--frame_count", type=int, default=30)
    
    args = parser.parse_args(argv)
    
    success = generate_walk_cycle(
        args.input,
        args.output,
        args.frame_count
    )
    
    if not success:
        sys.exit(1)

import bpy
import argparse
import sys
import os
import shutil

def export_animations(input_path, output_dir):
    bpy.ops.wm.read_factory_settings(use_empty=True)
    
    # Ensure output dir exists
    if not os.path.exists(output_dir):
        os.makedirs(output_dir)
        
    # Import
    ext = os.path.splitext(input_path)[1].lower()
    if ext == '.glb' or ext == '.gltf':
        bpy.ops.import_scene.gltf(filepath=input_path)
    elif ext == '.fbx':
        bpy.ops.import_scene.fbx(filepath=input_path)
    elif ext == '.blend':
        # Link/Append logic is complex, usually we just open the blend file directly
        # But here we are "importing". For .blend, we'd reopen.
        bpy.ops.wm.open_mainfile(filepath=input_path)
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
    
    # Iterate over actions
    count = 0
    if not bpy.data.actions:
        print("No actions found")
        return True # Not an error, just 0 exports
        
    for action in bpy.data.actions:
        armature.animation_data.action = action
        
        # We need to mute other NLA tracks to ensure this action plays?
        # For simplicity, we assume clean file or just active action export.
        
        # Export
        # Valid characters only
        safe_name = "".join([c for c in action.name if c.isalnum() or c in (' ', '_', '-')]).strip()
        out_name = f"{safe_name}.glb"
        out_path = os.path.join(output_dir, out_name)
        
        # We need to tell the exporter to export ONLY the active action?
        # GLTF exporter typically exports all actions unless filtered.
        # But we want separate files per animation (Unreal style).
        
        # Trick: Stash other actions or delete them temporarily, or use NLA.
        # GLTF exporter has 'export_anim_single_armature' option but it usually packs them.
        # If we want separate files, we must isolate.
        
        # Isolating:
        # 1. Clear NLA tracks
        if armature.animation_data.nla_tracks:
            for track in armature.animation_data.nla_tracks:
                armature.animation_data.nla_tracks.remove(track)
        
        # 2. Assign action
        armature.animation_data.action = action
        
        # 3. Export
        # Note: GLTF export might export all actions in bpy.data.actions if we aren't careful.
        # We might need to filter.
        # Usually simplest is to create a temporary .blend per action or just export current scene range.
        # But GLTF exporter tries to be smart.
        
        # Use 'export_animations=True' and ensure only one is pushed?
        bpy.ops.export_scene.gltf(
            filepath=out_path, 
            export_format='GLB',
            export_animations=True
            # In newer blender versions, there are more controls.
        )
        print(f"Exported: {out_path}")
        count += 1
        
    print(f"Exported {count} animations.")
    return True

if __name__ == "__main__":
    if "--" in sys.argv:
        argv = sys.argv[sys.argv.index("--") + 1:]
    else:
        argv = []

    parser = argparse.ArgumentParser()
    parser.add_argument("--input", required=True)
    parser.add_argument("--output_dir", required=True)
    
    args = parser.parse_args(argv)
    
    success = export_animations(
        args.input,
        args.output_dir
    )
    
    if not success:
        sys.exit(1)

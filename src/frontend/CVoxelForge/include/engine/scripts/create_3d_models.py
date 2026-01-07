#!/usr/bin/env python3

import os
import json
import math

def create_obj_model(vertices, faces, texture_coords=None, normals=None):
    """Create an OBJ file from vertices and faces"""
    obj_content = []
    
    # Add vertices
    for vertex in vertices:
        obj_content.append(f"v {vertex[0]} {vertex[1]} {vertex[2]}")
    
    # Add texture coordinates if provided
    if texture_coords:
        for uv in texture_coords:
            obj_content.append(f"vt {uv[0]} {uv[1]}")
    
    # Add normals if provided
    if normals:
        for normal in normals:
            obj_content.append(f"vn {normal[0]} {normal[1]} {normal[2]}")
    
    # Add faces
    for face in faces:
        if texture_coords and normals:
            # Face with vertex/texture/normal indices
            face_str = " ".join([f"{v}/{vt}/{vn}" for v, vt, vn in face])
        elif texture_coords:
            # Face with vertex/texture indices
            face_str = " ".join([f"{v}/{vt}" for v, vt in face])
        elif normals:
            # Face with vertex/normal indices
            face_str = " ".join([f"{v}//{vn}" for v, vn in face])
        else:
            # Face with vertex indices only
            if isinstance(face[0], tuple):
                # Extract vertex indices from tuples
                face_str = " ".join([str(v[0]) for v in face])
            else:
                face_str = " ".join([str(v) for v in face])
        obj_content.append(f"f {face_str}")
    
    return "\n".join(obj_content)

def create_player_model():
    """Create a simple player model (humanoid)"""
    vertices = []
    faces = []
    texture_coords = []
    normals = []
    
    # Player dimensions (Minecraft-style)
    body_width = 0.6
    body_height = 1.2
    body_depth = 0.3
    
    head_size = 0.4
    arm_width = 0.2
    arm_height = 0.8
    leg_width = 0.2
    leg_height = 0.8
    
    vertex_index = 1
    
    # Create body
    body_vertices, body_faces, body_uvs = create_box(
        -body_width/2, 0, -body_depth/2,
        body_width/2, body_height, body_depth/2,
        vertex_index
    )
    vertices.extend(body_vertices)
    faces.extend(body_faces)
    texture_coords.extend(body_uvs)
    vertex_index += len(body_vertices)
    
    # Create head
    head_vertices, head_faces, head_uvs = create_box(
        -head_size/2, body_height, -head_size/2,
        head_size/2, body_height + head_size, head_size/2,
        vertex_index
    )
    vertices.extend(head_vertices)
    faces.extend(head_faces)
    texture_coords.extend(head_uvs)
    vertex_index += len(head_vertices)
    
    # Create left arm
    arm_left_vertices, arm_left_faces, arm_left_uvs = create_box(
        -body_width/2 - arm_width, body_height - 0.2, -arm_width/2,
        -body_width/2, body_height - 0.2 + arm_height, arm_width/2,
        vertex_index
    )
    vertices.extend(arm_left_vertices)
    faces.extend(arm_left_faces)
    texture_coords.extend(arm_left_uvs)
    vertex_index += len(arm_left_vertices)
    
    # Create right arm
    arm_right_vertices, arm_right_faces, arm_right_uvs = create_box(
        body_width/2, body_height - 0.2, -arm_width/2,
        body_width/2 + arm_width, body_height - 0.2 + arm_height, arm_width/2,
        vertex_index
    )
    vertices.extend(arm_right_vertices)
    faces.extend(arm_right_faces)
    texture_coords.extend(arm_right_uvs)
    vertex_index += len(arm_right_vertices)
    
    # Create left leg
    leg_left_vertices, leg_left_faces, leg_left_uvs = create_box(
        -leg_width/2, 0, -leg_width/2,
        leg_width/2, leg_height, leg_width/2,
        vertex_index
    )
    vertices.extend(leg_left_vertices)
    faces.extend(leg_left_faces)
    texture_coords.extend(leg_left_uvs)
    vertex_index += len(leg_left_vertices)
    
    # Create right leg
    leg_right_vertices, leg_right_faces, leg_right_uvs = create_box(
        body_width/2 - leg_width, 0, -leg_width/2,
        body_width/2, leg_height, leg_width/2,
        vertex_index
    )
    vertices.extend(leg_right_vertices)
    faces.extend(leg_right_faces)
    texture_coords.extend(leg_right_uvs)
    
    return create_obj_model(vertices, faces, texture_coords)

def create_zombie_model():
    """Create a simple zombie model"""
    vertices = []
    faces = []
    texture_coords = []
    
    # Zombie dimensions (similar to player but slightly different)
    body_width = 0.6
    body_height = 1.4
    body_depth = 0.3
    
    head_size = 0.4
    arm_width = 0.25
    arm_height = 0.9
    leg_width = 0.25
    leg_height = 0.7
    
    vertex_index = 1
    
    # Create body (slightly taller)
    body_vertices, body_faces, body_uvs = create_box(
        -body_width/2, 0, -body_depth/2,
        body_width/2, body_height, body_depth/2,
        vertex_index
    )
    vertices.extend(body_vertices)
    faces.extend(body_faces)
    texture_coords.extend(body_uvs)
    vertex_index += len(body_vertices)
    
    # Create head
    head_vertices, head_faces, head_uvs = create_box(
        -head_size/2, body_height, -head_size/2,
        head_size/2, body_height + head_size, head_size/2,
        vertex_index
    )
    vertices.extend(head_vertices)
    faces.extend(head_faces)
    texture_coords.extend(head_uvs)
    vertex_index += len(head_vertices)
    
    # Create arms (straighter, zombie-like)
    arm_left_vertices, arm_left_faces, arm_left_uvs = create_box(
        -body_width/2 - arm_width, body_height - 0.1, -arm_width/2,
        -body_width/2, body_height - 0.1 + arm_height, arm_width/2,
        vertex_index
    )
    vertices.extend(arm_left_vertices)
    faces.extend(arm_left_faces)
    texture_coords.extend(arm_left_uvs)
    vertex_index += len(arm_left_vertices)
    
    arm_right_vertices, arm_right_faces, arm_right_uvs = create_box(
        body_width/2, body_height - 0.1, -arm_width/2,
        body_width/2 + arm_width, body_height - 0.1 + arm_height, arm_width/2,
        vertex_index
    )
    vertices.extend(arm_right_vertices)
    faces.extend(arm_right_faces)
    texture_coords.extend(arm_right_uvs)
    vertex_index += len(arm_right_vertices)
    
    # Create legs
    leg_left_vertices, leg_left_faces, leg_left_uvs = create_box(
        -leg_width/2, 0, -leg_width/2,
        leg_width/2, leg_height, leg_width/2,
        vertex_index
    )
    vertices.extend(leg_left_vertices)
    faces.extend(leg_left_faces)
    texture_coords.extend(leg_left_uvs)
    vertex_index += len(leg_left_vertices)
    
    leg_right_vertices, leg_right_faces, leg_right_uvs = create_box(
        body_width/2 - leg_width, 0, -leg_width/2,
        body_width/2, leg_height, leg_width/2,
        vertex_index
    )
    vertices.extend(leg_right_vertices)
    faces.extend(leg_right_faces)
    texture_coords.extend(leg_right_uvs)
    
    return create_obj_model(vertices, faces, texture_coords)

def create_creeper_model():
    """Create a simple creeper model"""
    vertices = []
    faces = []
    texture_coords = []
    
    # Creeper dimensions
    body_width = 0.6
    body_height = 1.7
    body_depth = 0.6
    
    head_size = 0.8
    leg_width = 0.3
    leg_height = 0.6
    
    vertex_index = 1
    
    # Create body (tall and rectangular)
    body_vertices, body_faces, body_uvs = create_box(
        -body_width/2, leg_height, -body_depth/2,
        body_width/2, leg_height + body_height, body_depth/2,
        vertex_index
    )
    vertices.extend(body_vertices)
    faces.extend(body_faces)
    texture_coords.extend(body_uvs)
    vertex_index += len(body_vertices)
    
    # Create head (cube-shaped)
    head_vertices, head_faces, head_uvs = create_box(
        -head_size/2, leg_height + body_height, -head_size/2,
        head_size/2, leg_height + body_height + head_size, head_size/2,
        vertex_index
    )
    vertices.extend(head_vertices)
    faces.extend(head_faces)
    texture_coords.extend(head_uvs)
    vertex_index += len(head_vertices)
    
    # Create four legs
    leg_positions = [
        (-body_width/2 + leg_width/2, 0, -body_depth/2 + leg_width/2),
        (body_width/2 - leg_width/2, 0, -body_depth/2 + leg_width/2),
        (-body_width/2 + leg_width/2, 0, body_depth/2 - leg_width/2),
        (body_width/2 - leg_width/2, 0, body_depth/2 - leg_width/2)
    ]
    
    for pos in leg_positions:
        leg_vertices, leg_faces, leg_uvs = create_box(
            pos[0] - leg_width/2, pos[1], pos[2] - leg_width/2,
            pos[0] + leg_width/2, pos[1] + leg_height, pos[2] + leg_width/2,
            vertex_index
        )
        vertices.extend(leg_vertices)
        faces.extend(leg_faces)
        texture_coords.extend(leg_uvs)
        vertex_index += len(leg_vertices)
    
    return create_obj_model(vertices, faces, texture_coords)

def create_pig_model():
    """Create a simple pig model"""
    vertices = []
    faces = []
    texture_coords = []
    
    # Pig dimensions
    body_width = 1.0
    body_height = 0.6
    body_depth = 0.8
    
    head_size = 0.5
    leg_width = 0.2
    leg_height = 0.4
    
    vertex_index = 1
    
    # Create body
    body_vertices, body_faces, body_uvs = create_box(
        -body_width/2, leg_height, -body_depth/2,
        body_width/2, leg_height + body_height, body_depth/2,
        vertex_index
    )
    vertices.extend(body_vertices)
    faces.extend(body_faces)
    texture_coords.extend(body_uvs)
    vertex_index += len(body_vertices)
    
    # Create head
    head_vertices, head_faces, head_uvs = create_box(
        -body_width/2 - head_size/2, leg_height + 0.2, -head_size/2,
        -body_width/2 + head_size/2, leg_height + 0.2 + head_size, head_size/2,
        vertex_index
    )
    vertices.extend(head_vertices)
    faces.extend(head_faces)
    texture_coords.extend(head_uvs)
    vertex_index += len(head_vertices)
    
    # Create four legs
    leg_positions = [
        (-body_width/2 + leg_width/2, 0, -body_depth/2 + leg_width/2),
        (body_width/2 - leg_width/2, 0, -body_depth/2 + leg_width/2),
        (-body_width/2 + leg_width/2, 0, body_depth/2 - leg_width/2),
        (body_width/2 - leg_width/2, 0, body_depth/2 - leg_width/2)
    ]
    
    for pos in leg_positions:
        leg_vertices, leg_faces, leg_uvs = create_box(
            pos[0] - leg_width/2, pos[1], pos[2] - leg_width/2,
            pos[0] + leg_width/2, pos[1] + leg_height, pos[2] + leg_width/2,
            vertex_index
        )
        vertices.extend(leg_vertices)
        faces.extend(leg_faces)
        texture_coords.extend(leg_uvs)
        vertex_index += len(leg_vertices)
    
    return create_obj_model(vertices, faces, texture_coords)

def create_box(x1, y1, z1, x2, y2, z2, start_index=1):
    """Create a box with vertices and faces"""
    vertices = [
        (x1, y1, z1),  # 0
        (x2, y1, z1),  # 1
        (x2, y2, z1),  # 2
        (x1, y2, z1),  # 3
        (x1, y1, z2),  # 4
        (x2, y1, z2),  # 5
        (x2, y2, z2),  # 6
        (x1, y2, z2),  # 7
    ]
    
    # Faces (vertex indices, starting from start_index)
    faces = [
        # Front
        [(start_index + 0, 1), (start_index + 1, 2), (start_index + 2, 3), (start_index + 3, 4)],
        # Back
        [(start_index + 4, 5), (start_index + 7, 8), (start_index + 6, 7), (start_index + 5, 6)],
        # Left
        [(start_index + 0, 9), (start_index + 3, 10), (start_index + 7, 11), (start_index + 4, 12)],
        # Right
        [(start_index + 1, 13), (start_index + 5, 14), (start_index + 6, 15), (start_index + 2, 16)],
        # Top
        [(start_index + 3, 17), (start_index + 2, 18), (start_index + 6, 19), (start_index + 7, 20)],
        # Bottom
        [(start_index + 0, 21), (start_index + 4, 22), (start_index + 5, 23), (start_index + 1, 24)],
    ]
    
    # Texture coordinates for each face
    texture_coords = [
        (0, 0), (1, 0), (1, 1), (0, 1),  # Front
        (0, 0), (1, 0), (1, 1), (0, 1),  # Back
        (0, 0), (1, 0), (1, 1), (0, 1),  # Left
        (0, 0), (1, 0), (1, 1), (0, 1),  # Right
        (0, 0), (1, 0), (1, 1), (0, 1),  # Top
        (0, 0), (1, 0), (1, 1), (0, 1),  # Bottom
    ]
    
    return vertices, faces, texture_coords

def create_3d_models():
    """Create 3D models for player and basic mobs"""
    
    # Create models directory
    models_dir = "assets/models/characters"
    os.makedirs(models_dir, exist_ok=True)
    
    # Define models to create
    models = {
        "player": create_player_model,
        "zombie": create_zombie_model,
        "creeper": create_creeper_model,
        "pig": create_pig_model,
    }
    
    created_assets = []
    
    for model_name, create_func in models.items():
        obj_content = create_func()
        model_path = os.path.join(models_dir, f"{model_name}.obj")
        
        with open(model_path, 'w') as f:
            f.write(obj_content)
        
        created_assets.append({
            "id": f"model_{model_name}",
            "type": "model",
            "path": f"assets/models/characters/{model_name}.obj",
            "format": "obj"
        })
        
        print(f"Created: {model_path}")
    
    return created_assets

def update_manifest(assets):
    """Update the manifest.json file with new models"""
    manifest_path = "assets/manifest.json"
    
    # Load existing manifest
    try:
        with open(manifest_path, 'r') as f:
            manifest = json.load(f)
    except FileNotFoundError:
        manifest = {"assets": []}
    
    # Add new assets
    manifest["assets"].extend(assets)
    
    # Save updated manifest
    with open(manifest_path, 'w') as f:
        json.dump(manifest, f, indent=2)
    
    print(f"Updated {manifest_path} with {len(assets)} new models")

def main():
    print("Creating 3D models for Minecraft v2...")
    
    # Change to the correct directory
    script_dir = os.path.dirname(os.path.abspath(__file__))
    project_root = os.path.dirname(script_dir)
    os.chdir(project_root)
    
    # Create 3D models
    created_assets = create_3d_models()
    
    # Update manifest
    update_manifest(created_assets)
    
    print(f"\nCreated {len(created_assets)} 3D models!")
    print("All models have been generated and added to the manifest.")

if __name__ == "__main__":
    main()

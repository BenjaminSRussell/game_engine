#!/usr/bin/env python3

import os
from PIL import Image, ImageDraw, ImageFont
import json
import random

def create_block_textures():
    """Create placeholder block textures for Minecraft v2"""
    
    # Create blocks directory
    blocks_dir = "assets/textures/blocks"
    os.makedirs(blocks_dir, exist_ok=True)
    
    # Define block types and their colors
    block_types = {
        "grass": [(34, 139, 34), (139, 69, 19), (34, 139, 34)],
        "dirt": [(139, 69, 19)],
        "stone": [(128, 128, 128)],
        "cobblestone": [(105, 105, 105)],
        "wood": [(139, 90, 43)],
        "leaves": [(34, 100, 34)],
        "sand": [(238, 203, 173)],
        "gravel": [(119, 136, 153)],
        "coal_ore": [(64, 64, 64), (32, 32, 32)],
        "iron_ore": [(192, 192, 192), (128, 128, 128)],
        "gold_ore": [(255, 215, 0), (218, 165, 32)],
        "diamond_ore": [(185, 242, 255), (0, 191, 255)],
        "water": [(64, 164, 223, 128)],
        "lava": [(255, 69, 0, 200)],
        "glass": [(255, 255, 255, 64)],
        "brick": [(178, 34, 34)],
        "ice": [(176, 224, 230, 180)],
        "snow": [(255, 250, 250)],
        "obsidian": [(47, 79, 79)],
        "bedrock": [(64, 64, 64)],
    }
    
    created_assets = []
    
    for block_name, colors in block_types.items():
        texture_path = os.path.join(blocks_dir, f"{block_name}.png")
        create_block_texture(texture_path, colors, block_name)
        
        created_assets.append({
            "id": f"block_{block_name}",
            "type": "texture",
            "path": f"assets/textures/blocks/{block_name}.png",
            "width": 16,
            "height": 16
        })
        
        print(f"Created: {texture_path}")
    
    return created_assets

def create_block_texture(filepath, colors, block_type):
    """Create a 16x16 block texture"""
    image = Image.new('RGBA', (16, 16), (0, 0, 0, 0))
    draw = ImageDraw.Draw(image)
    
    if block_type == "grass":
        # Grass block with top dirt and side grass
        # Top (grass)
        draw.rectangle([(0, 0), (15, 15)], fill=colors[0])
        # Add some texture variation
        for _ in range(5):
            x, y = random.randint(0, 15), random.randint(0, 15)
            draw.point((x, y), fill=(random.randint(20, 50), random.randint(120, 160), random.randint(20, 50)))
        
    elif block_type == "leaves":
        # Leaves with transparency
        draw.rectangle([(0, 0), (15, 15)], fill=colors[0])
        # Add leaf texture
        for _ in range(8):
            x, y = random.randint(0, 15), random.randint(0, 15)
            draw.point((x, y), fill=(random.randint(20, 50), random.randint(80, 120), random.randint(20, 50)))
        
    elif "ore" in block_type:
        # Ore blocks with ore spots
        draw.rectangle([(0, 0), (15, 15)], fill=colors[0])
        # Add ore spots
        for _ in range(3):
            x, y = random.randint(2, 13), random.randint(2, 13)
            draw.rectangle([(x, y), (x+2, y+2)], fill=colors[1])
        
    elif block_type == "water":
        # Water with transparency
        draw.rectangle([(0, 0), (15, 15)], fill=colors[0])
        # Add wave effect
        for y in range(0, 16, 2):
            draw.line([(0, y), (15, y)], fill=(64, 164, 223, 100))
        
    elif block_type == "lava":
        # Lava with transparency
        draw.rectangle([(0, 0), (15, 15)], fill=colors[0])
        # Add lava bubbles
        for _ in range(3):
            x, y = random.randint(1, 14), random.randint(1, 14)
            draw.point((x, y), fill=(255, 140, 0, 255))
        
    elif block_type == "glass":
        # Glass with transparency
        draw.rectangle([(0, 0), (15, 15)], fill=colors[0])
        # Add glass reflection
        draw.line([(0, 0), (7, 7)], fill=(255, 255, 255, 100))
        
    elif block_type == "ice":
        # Ice with transparency
        draw.rectangle([(0, 0), (15, 15)], fill=colors[0])
        # Add ice cracks
        draw.line([(0, 0), (15, 15)], fill=(200, 230, 255, 200))
        draw.line([(0, 8), (15, 8)], fill=(200, 230, 255, 150))
        
    else:
        # Solid blocks with texture
        draw.rectangle([(0, 0), (15, 15)], fill=colors[0])
        
        # Add texture variation for most blocks
        if block_type not in ["snow", "bedrock"]:
            for _ in range(3):
                x, y = random.randint(0, 15), random.randint(0, 15)
                variation = random.randint(-20, 20)
                r, g, b = colors[0]
                new_color = (
                    max(0, min(255, r + variation)),
                    max(0, min(255, g + variation)),
                    max(0, min(255, b + variation))
                )
                draw.point((x, y), fill=new_color)
    
    # Scale up to 32x32 for better visibility
    image = image.resize((32, 32), Image.NEAREST)
    image.save(filepath)

def create_item_textures():
    """Create placeholder item textures"""
    
    # Create items directory
    items_dir = "assets/textures/items"
    os.makedirs(items_dir, exist_ok=True)
    
    # Define item types
    item_types = {
        "sword": [(192, 192, 192)],
        "pickaxe": [(128, 128, 128)],
        "axe": [(139, 69, 19)],
        "shovel": [(105, 105, 105)],
        "apple": [(255, 0, 0)],
        "bread": [(255, 228, 196)],
        "coal": [(32, 32, 32)],
        "iron_ingot": [(192, 192, 192)],
        "gold_ingot": [(255, 215, 0)],
        "diamond": [(185, 242, 255)],
        "torch": [(255, 255, 0)],
        "bow": [(139, 69, 19)],
        "arrow": [(105, 105, 105)],
        "bucket": [(128, 128, 128)],
    }
    
    created_assets = []
    
    for item_name, colors in item_types.items():
        texture_path = os.path.join(items_dir, f"{item_name}.png")
        create_item_texture(texture_path, colors, item_name)
        
        created_assets.append({
            "id": f"item_{item_name}",
            "type": "texture",
            "path": f"assets/textures/items/{item_name}.png",
            "width": 16,
            "height": 16
        })
        
        print(f"Created: {texture_path}")
    
    return created_assets

def create_item_texture(filepath, colors, item_type):
    """Create a 16x16 item texture"""
    image = Image.new('RGBA', (16, 16), (0, 0, 0, 0))
    draw = ImageDraw.Draw(image)
    
    if item_type in ["sword", "pickaxe", "axe", "shovel"]:
        # Tools - simple shape
        draw.rectangle([(2, 8), (14, 14)], fill=colors[0])  # Handle
        draw.rectangle([(6, 2), (10, 8)], fill=colors[0])  # Head
        
    elif item_type in ["apple", "bread"]:
        # Food items - circular/rectangular
        if item_type == "apple":
            draw.ellipse([(3, 3), (13, 13)], fill=colors[0])
        else:
            draw.rectangle([(2, 4), (14, 12)], fill=colors[0])
            
    elif item_type in ["coal", "iron_ingot", "gold_ingot", "diamond"]:
        # Ingots and gems - rectangular
        draw.rectangle([(4, 4), (12, 12)], fill=colors[0])
        
    elif item_type == "torch":
        # Torch - stick and flame
        draw.rectangle([(7, 8), (9, 14)], fill=(139, 69, 19))  # Stick
        draw.ellipse([(5, 2), (11, 8)], fill=colors[0])  # Flame
        
    elif item_type == "bow":
        # Bow - curved shape
        draw.arc([(2, 2), (14, 14)], 0, 180, fill=colors[0], width=2)
        draw.line([(8, 8), (8, 12)], fill=(139, 69, 19), width=1)  # String
        
    elif item_type == "arrow":
        # Arrow - line with point
        draw.line([(8, 14), (8, 4)], fill=colors[0], width=2)
        draw.polygon([(8, 2), (6, 6), (10, 6)], fill=colors[0])
        
    elif item_type == "bucket":
        # Bucket - U-shape
        draw.rectangle([(4, 6), (12, 12)], fill=colors[0])
        draw.rectangle([(3, 12), (13, 14)], fill=colors[0])
    
    # Scale up to 32x32 for better visibility
    image = image.resize((32, 32), Image.NEAREST)
    image.save(filepath)

def update_manifest(assets):
    """Update the manifest.json file with new assets"""
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
    
    print(f"Updated {manifest_path} with {len(assets)} new assets")

def main():
    print("Creating block and item textures for Minecraft v2...")
    
    # Change to the correct directory
    script_dir = os.path.dirname(os.path.abspath(__file__))
    project_root = os.path.dirname(script_dir)
    os.chdir(project_root)
    
    # Create block textures
    block_assets = create_block_textures()
    
    # Create item textures
    item_assets = create_item_textures()
    
    # Update manifest
    all_assets = block_assets + item_assets
    update_manifest(all_assets)
    
    print(f"\nCreated {len(all_assets)} texture assets!")
    print(f"  - {len(block_assets)} block textures")
    print(f"  - {len(item_assets)} item textures")
    print("All textures have been generated and added to the manifest.")

if __name__ == "__main__":
    main()

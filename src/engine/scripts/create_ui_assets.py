#!/usr/bin/env python3

import os
from PIL import Image, ImageDraw, ImageFont
import json

def create_ui_assets():
    """Create placeholder UI texture assets for Minecraft v2"""
    
    # Create UI textures directory
    ui_dir = "assets/textures/ui"
    os.makedirs(ui_dir, exist_ok=True)
    
    # Define UI assets to create
    assets = {
        "button_normal": (200, 44, create_button_texture),
        "button_pressed": (200, 44, create_button_pressed_texture),
        "button_hover": (200, 44, create_button_hover_texture),
        "panel_background": (400, 300, create_panel_texture),
        "inventory_slot": (40, 40, create_inventory_slot_texture),
        "inventory_slot_selected": (40, 40, create_inventory_slot_selected_texture),
        "hotbar_slot": (40, 40, create_hotbar_slot_texture),
        "hotbar_slot_selected": (40, 40, create_hotbar_slot_selected_texture),
        "health_bar": (100, 8, create_health_bar_texture),
        "hunger_bar": (100, 8, create_hunger_bar_texture),
        "armor_bar": (100, 8, create_armor_bar_texture),
        "experience_bar": (200, 6, create_experience_bar_texture),
        "crosshair": (20, 20, create_crosshair_texture),
        "loading_bar": (300, 20, create_loading_bar_texture),
        "loading_bar_fill": (300, 20, create_loading_bar_fill_texture),
    }
    
    created_assets = []
    
    for asset_name, (width, height, create_func) in assets.items():
        image = create_func(width, height)
        filepath = os.path.join(ui_dir, f"{asset_name}.png")
        image.save(filepath)
        created_assets.append({
            "id": f"ui_{asset_name}",
            "type": "texture",
            "path": f"assets/textures/ui/{asset_name}.png",
            "width": width,
            "height": height
        })
        print(f"Created: {filepath}")
    
    return created_assets

def create_button_texture(width, height):
    """Create a normal button texture"""
    image = Image.new('RGBA', (width, height), (0, 0, 0, 0))
    draw = ImageDraw.Draw(image)
    
    # Draw button background
    draw.rounded_rectangle(
        [(0, 0), (width-1, height-1)],
        radius=8,
        fill=(60, 60, 60, 200),
        outline=(180, 180, 180, 255),
        width=2
    )
    
    return image

def create_button_pressed_texture(width, height):
    """Create a pressed button texture"""
    image = Image.new('RGBA', (width, height), (0, 0, 0, 0))
    draw = ImageDraw.Draw(image)
    
    # Draw pressed button background
    draw.rounded_rectangle(
        [(0, 0), (width-1, height-1)],
        radius=8,
        fill=(40, 40, 40, 200),
        outline=(120, 120, 120, 255),
        width=2
    )
    
    return image

def create_button_hover_texture(width, height):
    """Create a hover button texture"""
    image = Image.new('RGBA', (width, height), (0, 0, 0, 0))
    draw = ImageDraw.Draw(image)
    
    # Draw hover button background
    draw.rounded_rectangle(
        [(0, 0), (width-1, height-1)],
        radius=8,
        fill=(80, 80, 80, 200),
        outline=(200, 200, 200, 255),
        width=2
    )
    
    return image

def create_panel_texture(width, height):
    """Create a panel background texture"""
    image = Image.new('RGBA', (width, height), (0, 0, 0, 0))
    draw = ImageDraw.Draw(image)
    
    # Draw panel background with gradient effect
    for i in range(height):
        alpha = 180 + int(20 * (i / height))
        color = (40, 40, 40, alpha)
        draw.line([(0, i), (width, i)], fill=color)
    
    # Draw border
    draw.rectangle([(0, 0), (width-1, height-1)], outline=(100, 100, 100, 255), width=2)
    
    return image

def create_inventory_slot_texture(width, height):
    """Create an inventory slot texture"""
    image = Image.new('RGBA', (width, height), (0, 0, 0, 0))
    draw = ImageDraw.Draw(image)
    
    # Draw slot background
    draw.rectangle([(2, 2), (width-3, height-3)], fill=(20, 20, 20, 180))
    draw.rectangle([(0, 0), (width-1, height-1)], outline=(100, 100, 100, 255), width=1)
    
    return image

def create_inventory_slot_selected_texture(width, height):
    """Create a selected inventory slot texture"""
    image = Image.new('RGBA', (width, height), (0, 0, 0, 0))
    draw = ImageDraw.Draw(image)
    
    # Draw selected slot background
    draw.rectangle([(2, 2), (width-3, height-3)], fill=(40, 40, 40, 200))
    draw.rectangle([(0, 0), (width-1, height-1)], outline=(255, 255, 255, 255), width=2)
    
    return image

def create_hotbar_slot_texture(width, height):
    """Create a hotbar slot texture"""
    image = Image.new('RGBA', (width, height), (0, 0, 0, 0))
    draw = ImageDraw.Draw(image)
    
    # Draw hotbar slot background
    draw.rectangle([(1, 1), (width-2, height-2)], fill=(0, 0, 0, 160))
    draw.rectangle([(0, 0), (width-1, height-1)], outline=(255, 255, 255, 120), width=1)
    
    return image

def create_hotbar_slot_selected_texture(width, height):
    """Create a selected hotbar slot texture"""
    image = Image.new('RGBA', (width, height), (0, 0, 0, 0))
    draw = ImageDraw.Draw(image)
    
    # Draw selected hotbar slot background
    draw.rectangle([(1, 1), (width-2, height-2)], fill=(20, 20, 20, 200))
    draw.rectangle([(0, 0), (width-1, height-1)], outline=(255, 255, 255, 255), width=2)
    
    return image

def create_health_bar_texture(width, height):
    """Create a health bar texture"""
    image = Image.new('RGBA', (width, height), (0, 0, 0, 0))
    draw = ImageDraw.Draw(image)
    
    # Draw background
    draw.rectangle([(0, 0), (width-1, height-1)], fill=(40, 40, 40, 180))
    
    return image

def create_hunger_bar_texture(width, height):
    """Create a hunger bar texture"""
    image = Image.new('RGBA', (width, height), (0, 0, 0, 0))
    draw = ImageDraw.Draw(image)
    
    # Draw background
    draw.rectangle([(0, 0), (width-1, height-1)], fill=(40, 40, 40, 180))
    
    return image

def create_armor_bar_texture(width, height):
    """Create an armor bar texture"""
    image = Image.new('RGBA', (width, height), (0, 0, 0, 0))
    draw = ImageDraw.Draw(image)
    
    # Draw background
    draw.rectangle([(0, 0), (width-1, height-1)], fill=(40, 40, 40, 180))
    
    return image

def create_experience_bar_texture(width, height):
    """Create an experience bar texture"""
    image = Image.new('RGBA', (width, height), (0, 0, 0, 0))
    draw = ImageDraw.Draw(image)
    
    # Draw background
    draw.rectangle([(0, 0), (width-1, height-1)], fill=(40, 40, 40, 180))
    
    return image

def create_crosshair_texture(width, height):
    """Create a crosshair texture"""
    image = Image.new('RGBA', (width, height), (0, 0, 0, 0))
    draw = ImageDraw.Draw(image)
    
    center_x, center_y = width // 2, height // 2
    size = 10
    
    # Draw crosshair
    draw.line([(center_x - size, center_y), (center_x + size, center_y)], fill=(255, 255, 255, 255), width=2)
    draw.line([(center_x, center_y - size), (center_x, center_y + size)], fill=(255, 255, 255, 255), width=2)
    
    return image

def create_loading_bar_texture(width, height):
    """Create a loading bar background texture"""
    image = Image.new('RGBA', (width, height), (0, 0, 0, 0))
    draw = ImageDraw.Draw(image)
    
    # Draw background
    draw.rectangle([(0, 0), (width-1, height-1)], fill=(20, 20, 20, 200))
    draw.rectangle([(0, 0), (width-1, height-1)], outline=(100, 100, 100, 255), width=1)
    
    return image

def create_loading_bar_fill_texture(width, height):
    """Create a loading bar fill texture"""
    image = Image.new('RGBA', (width, height), (0, 0, 0, 0))
    draw = ImageDraw.Draw(image)
    
    # Draw fill with gradient
    for i in range(height):
        color = (50 + int(50 * (i / height)), 150 + int(50 * (i / height)), 50, 255)
        draw.line([(0, i), (width, i)], fill=color)
    
    return image

def update_manifest(assets):
    """Update the manifest.json file with new UI assets"""
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
    print("Creating UI texture assets for Minecraft v2...")
    
    # Change to the correct directory
    script_dir = os.path.dirname(os.path.abspath(__file__))
    project_root = os.path.dirname(script_dir)
    os.chdir(project_root)
    
    # Create UI assets
    created_assets = create_ui_assets()
    
    # Update manifest
    update_manifest(created_assets)
    
    print(f"\nCreated {len(created_assets)} UI texture assets!")
    print("All UI textures have been generated and added to the manifest.")

if __name__ == "__main__":
    main()

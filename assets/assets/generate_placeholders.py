#!/usr/bin/env python3
# TODO: Medium - Add more variety to the generated placeholder assets, such as more colors for NPC skins and more complex particle effects.
# TODO: High - Generate placeholder assets for items and blocks, not just entities.
"""
Generate simple colored placeholder NPC skins
Creates 64x32 PNG files for each NPC type
"""

from PIL import Image
import os

# Create output directory
os.makedirs('assets/textures/entities', exist_ok=True)

# NPC skin colors (RGB)
skins = {
    'villager': (139, 69, 19),      # Brown
    'zombie': (0, 170, 0),           # Green
    'skeleton': (238, 238, 238),     # White/Bone
    'creeper': (13, 167, 11),        # Creeper Green
    'cow': (160, 82, 45),            # Brown
    'pig': (255, 192, 203),          # Pink
    'chicken': (255, 255, 255),      # White
}

print("Creating NPC skin placeholders...")

for name, color in skins.items():
    # Create 64x32 image
    img = Image.new('RGB', (64, 32), color)
    
    # Save
    output_path = f'assets/textures/entities/{name}.png'
    img.save(output_path)
    print(f"✓ Created {output_path}")

# Create additional particle textures
print("\nCreating additional particle textures...")

particles_dir = 'assets/textures/particles/effects'
os.makedirs(particles_dir, exist_ok=True)

# Fire particle (orange-red gradient)
fire = Image.new('RGBA', (16, 16))
for y in range(16):
    for x in range(16):
        # Radial gradient from center
        dx = x - 8
        dy = y - 8
        dist = (dx*dx + dy*dy) ** 0.5
        if dist < 8:
            intensity = 1.0 - (dist / 8.0)
            r = int(255 * intensity)
            g = int(69 * intensity)
            b = 0
            a = int(255 * intensity)
            fire.putpixel((x, y), (r, g, b, a))
        else:
            fire.putpixel((x, y), (0, 0, 0, 0))

fire.save(f'{particles_dir}/fire.png')
print(f"✓ Created {particles_dir}/fire.png")

# Magic sparkle (cyan-white)
magic = Image.new('RGBA', (16, 16))
for y in range(16):
    for x in range(16):
        dx = x - 8
        dy = y - 8
        dist = (dx*dx + dy*dy) ** 0.5
        if dist < 6:
            intensity = 1.0 - (dist / 6.0)
            r = int(255 * intensity)
            g = int(255 * intensity)
            b = 255
            a = int(255 * intensity)
            magic.putpixel((x, y), (r, g, b, a))
        else:
            magic.putpixel((x, y), (0, 0, 0, 0))

magic.save(f'{particles_dir}/magic.png')
print(f"✓ Created {particles_dir}/magic.png")

# Damage indicator (red flash)
damage = Image.new('RGBA', (16, 16), (255, 0, 0, 200))
damage.save(f'{particles_dir}/damage.png')
print(f"✓ Created {particles_dir}/damage.png")

print("\n=== Asset Generation Complete ===")
print(f"Created {len(skins)} NPC skins")
print(f"Created 3 additional particle effects")
print("\nAll assets are ready to use!")

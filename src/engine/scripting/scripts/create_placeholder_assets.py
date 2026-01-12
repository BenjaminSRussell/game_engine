#!/usr/bin/env python3
"""
Create placeholder assets for testing the game engine.
Generates minimal WAV files and PNG textures.
"""

import os
import struct
import json
from pathlib import Path

BASE_DIR = Path(__file__).parent.parent
ASSETS_DIR = BASE_DIR / "assets"

def create_wav_file(filepath, duration_ms=100, frequency=440):
    """Create a simple WAV file with a sine wave."""
    sample_rate = 44100
    num_samples = int(sample_rate * duration_ms / 1000)
    
    # WAV file header
    wav_header = struct.pack('<4sI4s', b'RIFF', 36 + num_samples * 2, b'WAVE')
    fmt_chunk = struct.pack('<4sIHHIIHH', b'fmt ', 16, 1, 1, sample_rate, 
                            sample_rate * 2, 2, 16)
    data_header = struct.pack('<4sI', b'data', num_samples * 2)
    
    # Generate simple sine wave
    import math
    samples = []
    for i in range(num_samples):
        value = int(32767 * 0.1 * math.sin(2 * math.pi * frequency * i / sample_rate))
        samples.append(struct.pack('<h', value))
    
    filepath.parent.mkdir(parents=True, exist_ok=True)
    with open(filepath, 'wb') as f:
        f.write(wav_header)
        f.write(fmt_chunk)
        f.write(data_header)
        f.write(b''.join(samples))
    
    print(f"  Created {filepath.name}")

def create_png_texture(filepath, size=32, color=(128, 128, 128)):
    """Create a simple solid color PNG texture."""
    try:
        from PIL import Image
        img = Image.new('RGB', (size, size), color)
        filepath.parent.mkdir(parents=True, exist_ok=True)
        img.save(filepath)
        print(f"  Created {filepath.name}")
    except ImportError:
        # Fallback: create a minimal PNG manually
        print(f"  Skipped {filepath.name} (PIL not available)")

def main():
    print("Creating placeholder assets...")
    
    # Audio files
    print("\n[1/3] Creating audio files...")
    sounds_dir = ASSETS_DIR / "sounds"
    
    # Player footsteps
    for surface in ['grass', 'stone', 'wood', 'sand', 'water']:
        for i in range(1, 4):
            create_wav_file(sounds_dir / "player" / "footsteps" / f"{surface}_{i:02d}.wav", 
                          duration_ms=150, frequency=200 + i * 50)
    
    # Combat sounds
    for sound in ['sword_swing', 'sword_hit', 'arrow_shoot', 'arrow_hit', 
                  'hurt_01', 'hurt_02', 'hurt_03', 'death']:
        create_wav_file(sounds_dir / "player" / "combat" / f"{sound}.wav", 
                       duration_ms=200, frequency=300)
    
    # Interact sounds
    for sound in ['block_place', 'block_break', 'door_open', 'door_close',
                  'chest_open', 'chest_close', 'eat', 'drink']:
        create_wav_file(sounds_dir / "player" / "interact" / f"{sound}.wav",
                       duration_ms=150, frequency=400)
    
    # Block sounds
    for block in ['stone', 'grass', 'wood']:
        for action in ['break', 'place']:
            create_wav_file(sounds_dir / "blocks" / f"{block}_{action}.wav",
                           duration_ms=180, frequency=250)
    
    for sound in ['water_flow', 'water_splash']:
        create_wav_file(sounds_dir / "blocks" / f"{sound}.wav",
                       duration_ms=300, frequency=200)
    
    # Ambient sounds
    for sound in ['cave_01', 'cave_02', 'wind_light', 'wind_heavy',
                  'rain_light', 'rain_heavy', 'thunder_01', 'thunder_02']:
        create_wav_file(sounds_dir / "ambient" / f"{sound}.wav",
                       duration_ms=500, frequency=150)
    
    # Music (longer duration)
    music_dir = ASSETS_DIR / "music"
    for track in ['calm_01', 'calm_02', 'calm_03']:
        create_wav_file(music_dir / "overworld" / f"{track}.ogg",
                       duration_ms=2000, frequency=220)
    
    for track in ['battle_01', 'battle_02']:
        create_wav_file(music_dir / "combat" / f"{track}.ogg",
                       duration_ms=2000, frequency=330)
    
    create_wav_file(music_dir / "menu" / "menu_theme.ogg",
                   duration_ms=2000, frequency=260)
    
    # Textures
    print("\n[2/3] Creating texture files...")
    textures_dir = ASSETS_DIR / "textures" / "blocks"
    
    texture_colors = {
        'stone': (128, 128, 128),
        'dirt': (139, 69, 19),
        'grass_top': (34, 139, 34),
        'grass_side': (107, 142, 35),
        'wood': (139, 90, 43),
        'planks': (205, 133, 63),
        'sand': (238, 214, 175),
        'water': (65, 105, 225),
        'lava': (255, 69, 0),
        'glass': (173, 216, 230),
        'leaves': (0, 128, 0),
        'wool': (255, 255, 255),
    }
    
    for name, color in texture_colors.items():
        create_png_texture(textures_dir / f"{name}.png", size=32, color=color)
    
    # Atlas map (already created by previous script)
    print("\n[3/3] Verifying atlas configuration...")
    atlas_path = ASSETS_DIR / "textures" / "atlas" / "block_atlas_map.json"
    if atlas_path.exists():
        print(f"   Atlas map exists at {atlas_path}")
    else:
        print(f"  Creating atlas map...")
        atlas_map = {
            "version": 1,
            "tile_size": 32,
            "tiles_per_row": 16,
            "tiles": {name: i for i, name in enumerate(texture_colors.keys())}
        }
        atlas_path.parent.mkdir(parents=True, exist_ok=True)
        with open(atlas_path, 'w') as f:
            json.dump(atlas_map, f, indent=2)
        print(f"   Created atlas map")
    
    print("\n" + "=" * 60)
    print("Placeholder assets created successfully!")
    print("=" * 60)
    print(f"\nAssets location: {ASSETS_DIR}")
    print("\nNote: These are placeholder assets for testing.")
    print("For production, download proper assets from:")
    print("  - Kenney.nl (https://kenney.nl/assets)")
    print("  - Freesound.org (https://freesound.org)")
    print("  - OpenGameArt.org (https://opengameart.org)")

if __name__ == "__main__":
    main()

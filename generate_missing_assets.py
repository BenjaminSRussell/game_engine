import os
import random
import math
import struct
import wave
from PIL import Image, ImageDraw, ImageFont

def ensure_dir(path):
    if not os.path.exists(path):
        os.makedirs(path)

# --- Sound Generation ---

def generate_tone(filepath, frequency, duration, volume=0.5, fade_out=True):
    sample_rate = 44100
    n_frames = int(sample_rate * duration)
    
    with wave.open(filepath, 'w') as wav_file:
        wav_file.setnchannels(1)
        wav_file.setsampwidth(2)
        wav_file.setframerate(sample_rate)
        
        data = bytearray()
        for i in range(n_frames):
            t = float(i) / sample_rate
            val = math.sin(2.0 * math.pi * frequency * t)
            
            # Envelope
            env = 1.0
            if fade_out:
                env = 1.0 - (float(i) / n_frames)
            
            # Scale to 16-bit
            sample = int(val * volume * env * 32767.0)
            data.extend(struct.pack('<h', sample))
            
        wav_file.writeframes(data)
    print(f"Generated sound: {filepath}")

def generate_menu_sounds():
    ensure_dir("assets/sounds/ui")
    generate_tone("assets/sounds/ui/button_click.wav", 440, 0.1, 0.5)
    generate_tone("assets/sounds/ui/button_hover.wav", 220, 0.05, 0.3)
    generate_tone("assets/sounds/ui/menu_open.wav", 330, 0.3, 0.4)
    generate_tone("assets/sounds/ui/menu_close.wav", 165, 0.3, 0.4)

# --- Texture Generation ---

def create_solid_image(path, size, color, alpha=255):
    img = Image.new('RGBA', size, color + (alpha,))
    img.save(path)
    print(f"Generated texture: {path}")

def create_gradient_image(path, size, color1, color2):
    img = Image.new('RGBA', size)
    draw = ImageDraw.Draw(img)
    for y in range(size[1]):
        r = int(color1[0] + (color2[0] - color1[0]) * y / size[1])
        g = int(color1[1] + (color2[1] - color1[1]) * y / size[1])
        b = int(color1[2] + (color2[2] - color1[2]) * y / size[1])
        draw.line([(0, y), (size[0], y)], fill=(r, g, b, 255))
    img.save(path)
    print(f"Generated gradient: {path}")

def create_button(path, size, base_color, border_color):
    img = Image.new('RGBA', size, (0, 0, 0, 0))
    draw = ImageDraw.Draw(img)
    # Rounded rect manually or just rect
    draw.rectangle([(0,0), (size[0]-1, size[1]-1)], fill=base_color, outline=border_color, width=2)
    img.save(path)
    print(f"Generated button: {path}")

def generate_menu_textures():
    ensure_dir("assets/textures/ui")
    create_gradient_image("assets/textures/ui/menu_background.png", (1920, 1080), (20, 20, 30), (50, 50, 80))
    create_solid_image("assets/textures/ui/panel.png", (512, 512), (0, 0, 0), 180)
    
    # Logo
    img = Image.new('RGBA', (512, 128), (0,0,0,0))
    draw = ImageDraw.Draw(img)
    # Draw simple text proxy
    draw.text((50, 40), "MINECRAFT V2", fill=(255, 255, 255))
    img.save("assets/textures/ui/logo.png")
    print("Generated logo")

    # Buttons
    create_button("assets/textures/ui/button_normal.png", (200, 40), (100, 100, 100, 255), (200, 200, 200))
    create_button("assets/textures/ui/button_hover.png", (200, 40), (120, 120, 120, 255), (255, 255, 255))
    create_button("assets/textures/ui/button_pressed.png", (200, 40), (80, 80, 80, 255), (150, 150, 150))

# --- Icon Generation ---

def generate_icon(name, color, text):
    # Determine base size for @1x (we will generate @2x and @3x)
    base_size = 32
    
    scales = {
        '@2x': 2,
        '@3x': 3
    }
    
    for suffix, scale in scales.items():
        size = base_size * scale
        img = Image.new('RGBA', (size, size), (0, 0, 0, 0))
        draw = ImageDraw.Draw(img)
        
        # Circle or square background
        draw.ellipse([(2*scale, 2*scale), (size-2*scale, size-2*scale)], fill=color)
        
        # Simple symbol/text
        # Centering text is tricky properly without font file, we'll approximate
        draw.text((size//3, size//3), text, fill=(255,255,255))
        
        filename = f"assets/ui/icons/items/{name}{suffix}.png"
        ensure_dir(os.path.dirname(filename))
        img.save(filename)
        print(f"Generated icon: {filename}")

def generate_icons():
    icons = [
        ("icon_resource_wood", (139, 69, 19), "W"),
        ("icon_resource_stone", (128, 128, 128), "S"),
        ("icon_resource_iron_ingot", (192, 192, 192), "Fe"),
        ("icon_resource_gold_ingot", (255, 215, 0), "Au"),
        ("icon_resource_diamond", (0, 255, 255), "D"),
        ("icon_resource_coal", (30, 30, 30), "C"),
        ("icon_resource_redstone", (255, 0, 0), "R"),
        ("icon_resource_emerald", (0, 255, 0), "E"),
        ("icon_food_apple", (255, 50, 50), "Ap"),
        ("icon_food_bread", (200, 150, 50), "Br"),
        ("icon_misc_torch", (255, 200, 50), "T"),
        ("icon_misc_bucket", (150, 150, 180), "B"),
        ("icon_misc_compass", (200, 200, 200), "Co"),
        ("icon_misc_clock", (220, 220, 50), "Cl"),
    ]
    
    for name, color, text in icons:
        generate_icon(name, color, text)

if __name__ == "__main__":
    generate_menu_sounds()
    generate_menu_textures()
    generate_icons()

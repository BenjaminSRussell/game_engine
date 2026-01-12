#!/usr/bin/env python3
"""
Generate procedural 32x32 block and ore textures using the project palette.
Outputs are deterministic with a seed for repeatable results.
"""
from __future__ import annotations

import argparse
import json
import random
import re
from pathlib import Path
from typing import Dict, List, Tuple

try:
    from PIL import Image
except ImportError as exc:  # pragma: no cover - runtime dependency check
    raise SystemExit("Pillow is required: pip install pillow") from exc


RGB = Tuple[int, int, int]


def clamp(value: int) -> int:
    return max(0, min(255, value))


def hex_to_rgb(hex_color: str) -> RGB:
    hex_color = hex_color.lstrip("#")
    return tuple(int(hex_color[i : i + 2], 16) for i in (0, 2, 4))  # type: ignore[return-value]


def adjust_color(rgb: RGB, factor: float) -> RGB:
    return tuple(clamp(int(channel * factor)) for channel in rgb)  # type: ignore[return-value]


def mix(rgb_a: RGB, rgb_b: RGB, t: float) -> RGB:
    return tuple(clamp(int(a + (b - a) * t)) for a, b in zip(rgb_a, rgb_b))  # type: ignore[return-value]


def load_palette(palette_path: Path) -> Dict[str, List[str]]:
    group = None
    groups: Dict[str, List[str]] = {}
    group_re = re.compile(r"^##\s+(.+)")
    color_re = re.compile(r"-\s+[A-Z0-9]+\s+(#[0-9A-Fa-f]{6})")
    for line in palette_path.read_text(encoding="utf-8").splitlines():
        group_match = group_re.match(line)
        if group_match:
            group = group_match.group(1).strip().lower()
            groups.setdefault(group, [])
            continue
        color_match = color_re.search(line)
        if color_match and group:
            groups[group].append(color_match.group(1).upper())
    return groups


def apply_edge_shading(img: Image.Image, rng: random.Random) -> None:
    width, height = img.size
    pixels = img.load()
    for x in range(width):
        px = pixels[x, height - 1]
        pixels[x, height - 1] = adjust_color(px, 0.85)
    for y in range(height):
        px = pixels[width - 1, y]
        pixels[width - 1, y] = adjust_color(px, 0.85)
    for x in range(width):
        px = pixels[x, 0]
        pixels[x, 0] = adjust_color(px, 1.08 if rng.random() > 0.2 else 1.04)
    for y in range(height):
        px = pixels[0, y]
        pixels[0, y] = adjust_color(px, 1.08 if rng.random() > 0.2 else 1.04)


def apply_noise(
    img: Image.Image,
    rng: random.Random,
    colors: List[RGB],
    density: float,
    cluster_min: int,
    cluster_max: int,
) -> None:
    width, height = img.size
    pixels = img.load()
    noise_points = int(width * height * density / 2.0)
    for _ in range(noise_points):
        cx = rng.randrange(width)
        cy = rng.randrange(height)
        cluster_size = rng.randint(cluster_min, cluster_max)
        color = rng.choice(colors)
        for _ in range(cluster_size):
            x = cx + rng.randint(-1, 1)
            y = cy + rng.randint(-1, 1)
            if 0 <= x < width and 0 <= y < height:
                pixels[x, y] = color


def paint_wood_grain(img: Image.Image, rng: random.Random, base: RGB) -> None:
    width, height = img.size
    pixels = img.load()
    for y in range(height):
        stripe = 1.05 if (y // 3) % 2 == 0 else 0.95
        for x in range(width):
            noise = rng.uniform(0.98, 1.02)
            pixels[x, y] = adjust_color(base, stripe * noise)


def paint_grass_band(img: Image.Image, rng: random.Random, top: RGB, bottom: RGB) -> None:
    width, height = img.size
    pixels = img.load()
    for y in range(height):
        t = y / (height - 1)
        blend = t * 0.8 + rng.uniform(-0.03, 0.03)
        blend = max(0.0, min(1.0, blend))
        row_color = mix(top, bottom, blend)
        for x in range(width):
            jitter = rng.uniform(0.98, 1.02)
            pixels[x, y] = adjust_color(row_color, jitter)


def make_block_texture(
    rng: random.Random,
    size: int,
    base_color: RGB,
    noise_colors: List[RGB],
    noise_density: float,
    cluster_min: int,
    cluster_max: int,
    material: str,
    accent_colors: List[RGB],
) -> Image.Image:
    img = Image.new("RGB", (size, size), base_color)
    if material == "wood":
        paint_wood_grain(img, rng, base_color)
    elif material == "grass":
        top = rng.choice(accent_colors)
        paint_grass_band(img, rng, top, base_color)
    apply_noise(img, rng, noise_colors, noise_density, cluster_min, cluster_max)
    apply_edge_shading(img, rng)
    return img


def add_ore_clusters(
    img: Image.Image, rng: random.Random, ore_colors: List[RGB]
) -> None:
    width, height = img.size
    pixels = img.load()
    cluster_count = rng.randint(4, 9)
    for _ in range(cluster_count):
        cx = rng.randrange(width)
        cy = rng.randrange(height)
        color = rng.choice(ore_colors)
        highlight = adjust_color(color, 1.15)
        points = rng.randint(6, 16)
        for _ in range(points):
            x = cx + rng.randint(-2, 2)
            y = cy + rng.randint(-2, 2)
            if 0 <= x < width and 0 <= y < height:
                pixels[x, y] = color
                if rng.random() > 0.7 and x > 0 and y > 0:
                    pixels[x - 1, y - 1] = highlight


def generate_textures(
    output_dir: Path,
    palette_path: Path,
    block_count: int,
    ore_count: int,
    size: int,
    seed: int,
    overwrite: bool,
) -> Path:
    palette = load_palette(palette_path)
    grays = palette.get("grays (stone, metal, neutral)", [])
    browns = palette.get("browns (dirt, wood base)", [])
    greens = palette.get("greens (grass, leaves)", [])
    sands = palette.get("sands (sand, sandstone, clay)", [])
    blues = palette.get("blues (water, ice, sky accents)", [])
    reds = palette.get("reds and oranges (lava, redstone)", [])
    accents = palette.get("accents (gems and metals)", [])

    if not all([grays, browns, greens, sands, blues, reds, accents]):
        raise SystemExit("Palette groups missing or empty in docs/PALETTE.md")

    materials = [
        {"name": "stone", "colors": grays, "noise": 0.12, "min": 2, "max": 4},
        {"name": "dirt", "colors": browns, "noise": 0.14, "min": 2, "max": 4},
        {"name": "grass", "colors": browns, "noise": 0.10, "min": 2, "max": 4},
        {"name": "sand", "colors": sands, "noise": 0.08, "min": 1, "max": 3},
        {"name": "gravel", "colors": grays + sands, "noise": 0.16, "min": 3, "max": 6},
        {"name": "wood", "colors": browns, "noise": 0.09, "min": 2, "max": 4},
        {"name": "clay", "colors": sands + browns, "noise": 0.11, "min": 2, "max": 4},
    ]

    ore_palette = [*accents, *reds, *blues, *greens]

    output_dir.mkdir(parents=True, exist_ok=True)
    manifest = []
    rng = random.Random(seed)

    for index in range(block_count):
        material = rng.choice(materials)
        base_hex = rng.choice(material["colors"])
        base_color = hex_to_rgb(base_hex)
        noise_colors = [
            adjust_color(base_color, 0.92),
            adjust_color(base_color, 1.06),
            hex_to_rgb(rng.choice(material["colors"])),
        ]
        accent_colors = [hex_to_rgb(rng.choice(greens)), hex_to_rgb(rng.choice(browns))]
        img = make_block_texture(
            rng,
            size,
            base_color,
            noise_colors,
            material["noise"],
            material["min"],
            material["max"],
            material["name"],
            accent_colors,
        )
        filename = f"block_gen_{index:03d}.png"
        path = output_dir / filename
        if path.exists() and not overwrite:
            continue
        img.save(path)
        manifest.append(
            {
                "id": index,
                "type": "block",
                "material": material["name"],
                "base": base_hex,
                "file": filename,
            }
        )

    for index in range(ore_count):
        base_hex = rng.choice(grays)
        base_color = hex_to_rgb(base_hex)
        noise_colors = [
            adjust_color(base_color, 0.9),
            adjust_color(base_color, 1.05),
        ]
        img = make_block_texture(
            rng,
            size,
            base_color,
            noise_colors,
            0.12,
            2,
            4,
            "stone",
            [hex_to_rgb(rng.choice(grays))],
        )
        ore_colors = [
            hex_to_rgb(rng.choice(ore_palette)),
            hex_to_rgb(rng.choice(ore_palette)),
        ]
        add_ore_clusters(img, rng, ore_colors)
        filename = f"block_ore_gen_{index:03d}.png"
        path = output_dir / filename
        if path.exists() and not overwrite:
            continue
        img.save(path)
        manifest.append(
            {
                "id": index,
                "type": "ore",
                "material": "stone",
                "base": base_hex,
                "file": filename,
            }
        )

    manifest_path = output_dir / "manifest.json"
    manifest_path.write_text(json.dumps(manifest, indent=2), encoding="utf-8")
    return manifest_path


def main() -> None:
    parser = argparse.ArgumentParser(description="Generate block and ore textures.")
    parser.add_argument(
        "--output-dir",
        default="assets/textures/blocks/generated_500",
        help="Output directory for generated textures.",
    )
    parser.add_argument(
        "--palette",
        default="docs/PALETTE.md",
        help="Palette file to source colors from.",
    )
    parser.add_argument("--blocks", type=int, default=250, help="Block texture count.")
    parser.add_argument("--ores", type=int, default=250, help="Ore texture count.")
    parser.add_argument("--size", type=int, default=32, help="Texture size in pixels.")
    parser.add_argument("--seed", type=int, default=42, help="Random seed.")
    parser.add_argument(
        "--overwrite", action="store_true", help="Overwrite existing files."
    )
    args = parser.parse_args()

    manifest_path = generate_textures(
        output_dir=Path(args.output_dir),
        palette_path=Path(args.palette),
        block_count=args.blocks,
        ore_count=args.ores,
        size=args.size,
        seed=args.seed,
        overwrite=args.overwrite,
    )
    print(f"Wrote textures and manifest to {manifest_path}")


if __name__ == "__main__":
    main()

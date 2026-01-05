# UI Assets Directory

This directory contains all UI-related assets for the Swift frontend.

## Directory Structure

```
ui/
├── icons/          # Game icons (health, hunger, items, etc.)
├── buttons/        # Button sprites and states
├── backgrounds/    # Menu and panel backgrounds
├── borders/        # UI borders and frames
├── effects/        # Particle effects and animations
└── themes/         # Theme-specific assets
    ├── medieval/   # Medieval fantasy theme
    ├── scifi/      # Sci-fi futuristic theme
    └── modern/     # Modern/minimalist theme
```

## Asset Sources

All assets are sourced from free, commercially-licensed repositories:

1. **itch.io** - UI packs and icon sets
2. **OpenGameArt.org** - UI Pack (June 2024)
3. **Game-icons.net** - 4170+ free SVG/PNG icons
4. **Vecteezy** - Vector UI elements
5. **CraftPix.net** - Free 2D game assets

## Naming Conventions

- Icons: `icon_[category]_[name].png` (e.g., `icon_item_sword.png`)
- Buttons: `btn_[state]_[type].png` (e.g., `btn_normal_primary.png`)
- Backgrounds: `bg_[context]_[variant].png` (e.g., `bg_menu_main.png`)
- Borders: `border_[style]_[size].png` (e.g., `border_panel_large.png`)

## Asset Specifications

- **Format**: PNG with alpha channel
- **Resolution**: @2x and @3x for iOS retina displays
- **Color Space**: sRGB
- **Compression**: Optimized with pngquant or similar

## License Information

See `LICENSES.txt` for detailed attribution and license information for each asset pack.

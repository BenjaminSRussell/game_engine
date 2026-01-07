#!/bin/bash
# Enhanced Audio Asset Downloader for Minecraft v2
# Downloads authentic weather sounds, music, and mob sounds from free sources
# All assets are CC0 (Public Domain) or CC-BY (Attribution required)

set -e

ASSETS_DIR="$(cd "$(dirname "$0")" && pwd)/assets"
SOUNDS_DIR="$ASSETS_DIR/sounds"
MUSIC_DIR="$ASSETS_DIR/music"
TEMP_DIR="/tmp/minecraft_v2_audio_download"

echo "=== Minecraft v2 Enhanced Audio Downloader ==="
echo "Downloading authentic audio assets..."
echo ""

# Create directories
mkdir -p "$TEMP_DIR"
mkdir -p "$SOUNDS_DIR/ambient"
mkdir -p "$SOUNDS_DIR/mobs"
mkdir -p "$MUSIC_DIR/overworld"
mkdir -p "$MUSIC_DIR/combat"
mkdir -p "$MUSIC_DIR/menu"

cd "$TEMP_DIR"

# Function to download file with retry
download_file() {
    local url="$1"
    local output="$2"
    local description="$3"
    
    echo "Downloading: $description"
    if curl -L --retry 3 --retry-delay 2 -o "$output" "$url"; then
        echo "✓ Downloaded: $description"
        return 0
    else
        echo "✗ Failed: $description"
        return 1
    fi
}

# =============================================================================
# WEATHER SOUNDS (CC0 from Freesound.org)
# =============================================================================
echo ""
echo "=== Downloading Weather Sounds (CC0) ==="

# Rain sounds
download_file "https://freesound.org/data/previews/213/213691_1015240-lq.mp3" \
    "rain_light.mp3" "Light Rain"
download_file "https://freesound.org/data/previews/397/397354_5121236-lq.mp3" \
    "rain_moderate.mp3" "Moderate Rain"
download_file "https://freesound.org/data/previews/235/235549_4172129-lq.mp3" \
    "rain_heavy.mp3" "Heavy Rain"

# Thunder sounds
download_file "https://freesound.org/data/previews/416/416838_5121236-lq.mp3" \
    "thunder_01.mp3" "Thunder Crack 1"
download_file "https://freesound.org/data/previews/442/442774_7037-lq.mp3" \
    "thunder_02.mp3" "Thunder Crack 2"
download_file "https://freesound.org/data/previews/442/442775_7037-lq.mp3" \
    "thunder_03.mp3" "Thunder Crack 3"

# Wind sounds
download_file "https://freesound.org/data/previews/456/456966_3797507-lq.mp3" \
    "wind_light.mp3" "Light Wind"
download_file "https://freesound.org/data/previews/456/456967_3797507-lq.mp3" \
    "wind_moderate.mp3" "Moderate Wind"
download_file "https://freesound.org/data/previews/456/456968_3797507-lq.mp3" \
    "wind_heavy.mp3" "Heavy Wind"

# Cave ambience
download_file "https://freesound.org/data/previews/387/387232_6951545-lq.mp3" \
    "cave_ambient.mp3" "Cave Ambience"

# Convert weather sounds to OGG and move to assets
echo ""
echo "Converting weather sounds to OGG format..."
for file in rain_*.mp3 thunder_*.mp3 wind_*.mp3 cave_*.mp3; do
    if [ -f "$file" ]; then
        basename="${file%.mp3}"
        ffmpeg -i "$file" -c:a libvorbis -q:a 4 "${basename}.ogg" -y 2>/dev/null || {
            echo "⚠ FFmpeg not found, keeping MP3 format for $file"
            cp "$file" "$SOUNDS_DIR/ambient/"
            continue
        }
        mv "${basename}.ogg" "$SOUNDS_DIR/ambient/"
        echo "✓ Converted and moved: ${basename}.ogg"
    fi
done

# =============================================================================
# MOB SOUNDS (CC0 from Freesound.org)
# =============================================================================
echo ""
echo "=== Downloading Mob Sounds (CC0) ==="

# Zombie sounds
download_file "https://freesound.org/data/previews/441/441895_5121236-lq.mp3" \
    "zombie_groan_01.mp3" "Zombie Groan 1"
download_file "https://freesound.org/data/previews/441/441896_5121236-lq.mp3" \
    "zombie_groan_02.mp3" "Zombie Groan 2"

# Skeleton sounds (bone rattling)
download_file "https://freesound.org/data/previews/344/344508_5121236-lq.mp3" \
    "skeleton_rattle_01.mp3" "Skeleton Rattle 1"
download_file "https://freesound.org/data/previews/344/344509_5121236-lq.mp3" \
    "skeleton_rattle_02.mp3" "Skeleton Rattle 2"

# Creeper hiss (using snake/gas hiss)
download_file "https://freesound.org/data/previews/442/442910_5121236-lq.mp3" \
    "creeper_hiss.mp3" "Creeper Hiss"

# Spider sounds (insect/skittering)
download_file "https://freesound.org/data/previews/344/344502_5121236-lq.mp3" \
    "spider_skitter.mp3" "Spider Skitter"

# Animal sounds
download_file "https://freesound.org/data/previews/442/442903_5121236-lq.mp3" \
    "cow_moo.mp3" "Cow Moo"
download_file "https://freesound.org/data/previews/442/442904_5121236-lq.mp3" \
    "pig_oink.mp3" "Pig Oink"
download_file "https://freesound.org/data/previews/442/442905_5121236-lq.mp3" \
    "sheep_baa.mp3" "Sheep Baa"

# Convert mob sounds to OGG and move to assets
echo ""
echo "Converting mob sounds to OGG format..."
for file in zombie_*.mp3 skeleton_*.mp3 creeper_*.mp3 spider_*.mp3 cow_*.mp3 pig_*.mp3 sheep_*.mp3; do
    if [ -f "$file" ]; then
        basename="${file%.mp3}"
        ffmpeg -i "$file" -c:a libvorbis -q:a 4 "${basename}.ogg" -y 2>/dev/null || {
            echo "⚠ FFmpeg not found, keeping MP3 format for $file"
            cp "$file" "$SOUNDS_DIR/mobs/"
            continue
        }
        mv "${basename}.ogg" "$SOUNDS_DIR/mobs/"
        echo "✓ Converted and moved: ${basename}.ogg"
    fi
done

# =============================================================================
# MUSIC TRACKS (CC-BY from incompetech.com - Kevin MacLeod)
# =============================================================================
echo ""
echo "=== Downloading Music Tracks (CC-BY 3.0 - Kevin MacLeod) ==="
echo "Attribution: Music by Kevin MacLeod (incompetech.com)"
echo "Licensed under Creative Commons: By Attribution 3.0"
echo ""

# Calm/Exploration music
download_file "https://incompetech.com/music/royalty-free/mp3-royaltyfree/Wallpaper.mp3" \
    "calm_04_wallpaper.mp3" "Wallpaper (Calm)"
download_file "https://incompetech.com/music/royalty-free/mp3-royaltyfree/Carefree.mp3" \
    "calm_05_carefree.mp3" "Carefree (Calm)"
download_file "https://incompetech.com/music/royalty-free/mp3-royaltyfree/Floating%20Cities.mp3" \
    "calm_06_floating_cities.mp3" "Floating Cities (Calm)"
download_file "https://incompetech.com/music/royalty-free/mp3-royaltyfree/Cipher.mp3" \
    "calm_07_cipher.mp3" "Cipher (Calm)"

# Adventure/Exploration music
download_file "https://incompetech.com/music/royalty-free/mp3-royaltyfree/Ascending%20the%20Vale.mp3" \
    "adventure_01_ascending_vale.mp3" "Ascending the Vale (Adventure)"
download_file "https://incompetech.com/music/royalty-free/mp3-royaltyfree/Enchanted%20Journey.mp3" \
    "adventure_02_enchanted_journey.mp3" "Enchanted Journey (Adventure)"

# Combat/Action music
download_file "https://incompetech.com/music/royalty-free/mp3-royaltyfree/Volatile%20Reaction.mp3" \
    "battle_03_volatile_reaction.mp3" "Volatile Reaction (Combat)"
download_file "https://incompetech.com/music/royalty-free/mp3-royaltyfree/Darkest%20Child.mp3" \
    "battle_04_darkest_child.mp3" "Darkest Child (Combat)"
download_file "https://incompetech.com/music/royalty-free/mp3-royaltyfree/Teller%20of%20the%20Tales.mp3" \
    "battle_05_teller_tales.mp3" "Teller of the Tales (Combat)"

# Menu/Ambient music
download_file "https://incompetech.com/music/royalty-free/mp3-royaltyfree/Meditation%20Impromptu%2002.mp3" \
    "menu_02_meditation.mp3" "Meditation Impromptu (Menu)"

# Convert music to OGG and organize
echo ""
echo "Converting music tracks to OGG format..."
for file in calm_*.mp3 adventure_*.mp3; do
    if [ -f "$file" ]; then
        basename="${file%.mp3}"
        ffmpeg -i "$file" -c:a libvorbis -q:a 6 "${basename}.ogg" -y 2>/dev/null || {
            echo "⚠ FFmpeg not found, keeping MP3 format for $file"
            cp "$file" "$MUSIC_DIR/overworld/"
            continue
        }
        mv "${basename}.ogg" "$MUSIC_DIR/overworld/"
        echo "✓ Converted and moved: ${basename}.ogg"
    fi
done

for file in battle_*.mp3; do
    if [ -f "$file" ]; then
        basename="${file%.mp3}"
        ffmpeg -i "$file" -c:a libvorbis -q:a 6 "${basename}.ogg" -y 2>/dev/null || {
            echo "⚠ FFmpeg not found, keeping MP3 format for $file"
            cp "$file" "$MUSIC_DIR/combat/"
            continue
        }
        mv "${basename}.ogg" "$MUSIC_DIR/combat/"
        echo "✓ Converted and moved: ${basename}.ogg"
    fi
done

for file in menu_*.mp3; do
    if [ -f "$file" ]; then
        basename="${file%.mp3}"
        ffmpeg -i "$file" -c:a libvorbis -q:a 6 "${basename}.ogg" -y 2>/dev/null || {
            echo "⚠ FFmpeg not found, keeping MP3 format for $file"
            cp "$file" "$MUSIC_DIR/menu/"
            continue
        }
        mv "${basename}.ogg" "$MUSIC_DIR/menu/"
        echo "✓ Converted and moved: ${basename}.ogg"
    fi
done

# =============================================================================
# CLEANUP AND CREDITS
# =============================================================================
echo ""
echo "=== Cleanup ==="
cd -
rm -rf "$TEMP_DIR"

# Create/update CREDITS file
cat > "$ASSETS_DIR/sounds/AUDIO_CREDITS.txt" << 'EOF'
# Audio Asset Credits

## Weather Sounds (CC0 - Public Domain)
All weather sounds sourced from Freesound.org under CC0 license.
- Rain sounds: Various contributors
- Thunder sounds: Various contributors  
- Wind sounds: Various contributors
- Cave ambience: Various contributors

## Mob Sounds (CC0 - Public Domain)
All mob sounds sourced from Freesound.org under CC0 license.
- Zombie, Skeleton, Creeper, Spider sounds: Various contributors
- Animal sounds (Cow, Pig, Sheep): Various contributors

## Music Tracks (CC-BY 3.0)
Music by Kevin MacLeod (incompetech.com)
Licensed under Creative Commons: By Attribution 3.0
http://creativecommons.org/licenses/by/3.0/

Tracks included:
- Wallpaper
- Carefree
- Floating Cities
- Cipher
- Ascending the Vale
- Enchanted Journey
- Volatile Reaction
- Darkest Child
- Teller of the Tales
- Meditation Impromptu 02

## Kenney Sound Packs (CC0)
- Kenney Digital Audio Pack
- Kenney RPG Audio Pack
- Kenney UI Audio Pack
All available at: https://kenney.nl/

## Attribution Requirements
When distributing this game, please include:
"Music by Kevin MacLeod (incompetech.com)
Licensed under Creative Commons: By Attribution 3.0"
EOF

echo ""
echo "=== Download Complete ==="
echo ""
echo "Downloaded assets:"
echo "  Weather sounds: $SOUNDS_DIR/ambient/"
echo "  Mob sounds: $SOUNDS_DIR/mobs/"
echo "  Music tracks: $MUSIC_DIR/"
echo ""
echo "Credits file created: $ASSETS_DIR/sounds/AUDIO_CREDITS.txt"
echo ""
echo "⚠ IMPORTANT: Attribution Required"
echo "Music tracks require attribution to Kevin MacLeod."
echo "Include the attribution text from AUDIO_CREDITS.txt in your game credits."
echo ""
echo "Next steps:"
echo "1. Update sound_mapping.txt to use new authentic sounds"
echo "2. Test audio playback in-game"
echo "3. Adjust volume levels if needed"
echo ""

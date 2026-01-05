# Asset Licensing and Attribution

## ⚠️ Important Legal Notice

This directory contains information about asset sources and their respective licenses. **It is your responsibility to ensure compliance with all licensing requirements when using assets extracted from Minecraft modpacks.**

## License Types Overview

Minecraft mods typically use various licenses:

1. **MIT License** - Permissive, allows commercial use with attribution
2. **GPL/LGPL** - Copyleft, requires derivative works to be open source
3. **All Rights Reserved** - Copyrighted, requires explicit permission
4. **CC BY/CC BY-SA** - Creative Commons, requires attribution
5. **Custom Licenses** - Varies by mod author

## Best Practices

1. **Always Check Individual Mod Licenses**: Each mod may have a different license
2. **Maintain Attribution Records**: Keep track of which assets came from which mods
3. **Respect Author Rights**: Some mods explicitly prohibit asset extraction
4. **Contact Authors When in Doubt**: When license is unclear, contact the mod author
5. **Consider Creating Original Assets**: For commercial projects, creating original assets is safest

## Attribution Tracking

The extraction and organization scripts create manifest files that track asset sources:
- `extracted/extraction_manifest.json` - Maps extracted assets to source JARs
- `extracted/organization_manifest.json` - Maps organized assets to categories and mods

Use these manifests to:
1. Identify which mod each asset came from
2. Look up the mod's license
3. Generate attribution lists
4. Remove assets with incompatible licenses

## Mod-Specific Notes

### Vehicles
- **MrCrayfish's Vehicle Mod**: Check mod page for license
- **Flan's Mod**: Check mod page for license
- **Create**: MIT License (permissive)
- **Immersive Railroading**: Check mod page for license

### Magic
- **Thaumcraft**: Check mod page for license
- **Botania**: Check mod page for license
- **Ars Nouveau**: Check mod page for license
- **Blood Magic**: Check mod page for license

### Tech
- **Immersive Engineering**: MIT License (permissive)
- **Mekanism**: MIT License (permissive)
- **Applied Energistics 2**: Check mod page for license

## Recommended Workflow

1. **Extract assets** using the provided scripts
2. **Review organization manifest** to identify asset sources
3. **Check each mod's license** on CurseForge/Modrinth/GitHub
4. **Document licenses** in a `LICENSES.txt` file
5. **Remove incompatible assets** before using in your project
6. **Add attribution** to your project's credits/LICENSE file

## Generating Attribution List

You can generate an attribution list from the organization manifest:

```python
import json
from pathlib import Path

manifest_path = Path("extracted/organization_manifest.json")
with open(manifest_path) as f:
    manifest = json.load(f)

mods = set()
for org in manifest['organization']:
    mods.add(org['mod_name'])

print("Mods used in this project:")
for mod in sorted(mods):
    print(f"- {mod} (check license at mod page)")
```

## Disclaimer

This tool is provided for educational and development purposes. The authors are not responsible for any copyright infringement. Always verify licensing before using extracted assets in production projects.


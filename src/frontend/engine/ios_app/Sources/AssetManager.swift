import UIKit

/// Manages all UI assets including icons, sprites, and theme-specific resources
class AssetManager {
    
    static let shared = AssetManager()
    
    // Asset categories
    enum AssetCategory: String {
        case icon = "icons"
        case button = "buttons"
        case background = "backgrounds"
        case border = "borders"
        case effect = "effects"
    }
    
    // Asset cache for performance
    private var imageCache: [String: UIImage] = [:]
    private let cacheQueue = DispatchQueue(label: "com.minecraft.assetmanager.cache")
    
    private init() {
        preloadCommonAssets()
    }
    
    // MARK: - Asset Loading
    
    /// Load an asset by name from a specific category
    func loadAsset(named name: String, category: AssetCategory, theme: Theme? = nil) -> UIImage? {
        let cacheKey = buildCacheKey(name: name, category: category, theme: theme)
        
        // Check cache first
        if let cachedImage = cacheQueue.sync(execute: { imageCache[cacheKey] }) {
            return cachedImage
        }
        
        // Build asset path relative to assets/ directory
        // e.g. "ui/icons/items/icon_items_sword"
        var assetPath = "ui/\(category.rawValue)/\(name)"
        if let theme = theme {
            assetPath = "ui/themes/\(theme.identifier)/\(category.rawValue)/\(name)"
        }
        
        // Try loading
        if let image = loadImage(path: assetPath) {
            cacheQueue.async { [weak self] in
                self?.imageCache[cacheKey] = image
            }
            return image
        }
        
        // Fallback to default asset
        return loadFallbackAsset(category: category)
    }
    
    /// Load an icon by name
    func loadIcon(named name: String, theme: Theme? = nil) -> UIImage? {
        return loadAsset(named: name, category: .icon, theme: theme)
    }
    
    /// Load a button sprite by name and state
    func loadButton(named name: String, state: ButtonState = .normal, theme: Theme? = nil) -> UIImage? {
        let buttonName = "btn_\(state.rawValue)_\(name)"
        return loadAsset(named: buttonName, category: .button, theme: theme)
    }
    
    /// Load a background image
    func loadBackground(named name: String, theme: Theme? = nil) -> UIImage? {
        return loadAsset(named: name, category: .background, theme: theme)
    }
    
    // MARK: - Icon Mapping
    
    /// Map game item/entity names to icon asset names
    func iconForItem(_ itemName: String) -> UIImage? {
        // Map item names to logical paths (category/filename)
        // Files are in ui/icons/items/ or ui/icons/status/
        let iconMap: [String: String] = [
            // Tools & Weapons
            "Diamond Sword": "items/icon_items_sword",
            "Diamond Pickaxe": "items/icon_items_pickaxe",
            "Iron Axe": "items/icon_items_axe",
            "Iron Shovel": "items/icon_items_shovel",
            "Bow": "items/icon_items_bow",
            "Arrow": "items/icon_items_arrow",
            
            // Resources
            "Wood": "items/icon_resource_wood",
            "Stone": "items/icon_resource_stone",
            "Iron Ingot": "items/icon_resource_iron_ingot",
            "Gold Ingot": "items/icon_resource_gold_ingot",
            "Diamond": "items/icon_resource_diamond",
            "Coal": "items/icon_resource_coal",
            "Redstone": "items/icon_resource_redstone",
            "Emerald": "items/icon_resource_emerald",
            
            // Food
            "Apple": "items/icon_food_apple",
            "Bread": "items/icon_food_bread",
            
            // Misc
            "Torch": "items/icon_misc_torch",
            "Bucket": "items/icon_misc_bucket",
            "Compass": "items/icon_misc_compass",
            "Clock": "items/icon_misc_clock",
            
            // Status (in stats/ subfolder usually, but currently flat or status/)
            // From file listing: ui/icons/status/icon_status_...
            "Health": "status/icon_status_health",
            "Hunger": "status/icon_status_hunger",
            "Armor": "status/icon_status_armor",
            "Experience": "status/icon_status_experience",
            "Oxygen": "status/icon_status_oxygen"
        ]
        
        if let iconPath = iconMap[itemName] {
            return loadIcon(named: iconPath, theme: ThemeManager.shared.currentTheme)
        }
        
        // Fallback to placeholder
        return createPlaceholderIcon(text: String(itemName.prefix(2)))
    }
    
    // MARK: - Preloading
    
    private func preloadCommonAssets() {
        // Preload frequently used icons
        let commonIcons = [
            "status/icon_status_health",
            "status/icon_status_hunger",
            "status/icon_status_armor",
            "status/icon_status_experience"
        ]
        
        for iconName in commonIcons {
            _ = loadIcon(named: iconName)
        }
    }
    
    // MARK: - Helper Methods
    
    private func buildCacheKey(name: String, category: AssetCategory, theme: Theme?) -> String {
        if let theme = theme {
            return "\(theme.identifier)_\(category.rawValue)_\(name)"
        }
        return "\(category.rawValue)_\(name)"
    }
    
    private func loadImage(path: String) -> UIImage? {
        // 1. Try loading from main bundle (for production builds)
        if let image = UIImage(named: path) {
            return image
        }
        
        // 2. Try loading from absolute assets path (for local dev/simulators)
        // We look for the 'assets' directory relative to source root if possible
        // or rely on a known dev environment variable/path.
        
        // Check for specific scales if loading raw files
        let scales = ["@3x", "@2x", ""]
        
        // Try to find the assets directory relative to the file.
        // Hardcoded fallback for the specific user environment as requested,
        // but with a safeguard check.
        let devBasePath = "/Users/benjaminrussell/Desktop/Minecraft v2/assets"
        let fullPathBase = "\(devBasePath)/\(path)"
        
        for scale in scales {
            let pathWithScale = "\(fullPathBase)\(scale).png"
            if FileManager.default.fileExists(atPath: pathWithScale) {
                if let image = UIImage(contentsOfFile: pathWithScale) {
                    return image
                }
            }
        }
        
        return nil
    }
    
    private func loadFallbackAsset(category: AssetCategory) -> UIImage? {
        switch category {
        case .icon:
            return createPlaceholderIcon(text: "?")
        case .button:
            return createPlaceholderButton()
        default:
            return nil
        }
    }
    
    private func createPlaceholderIcon(text: String) -> UIImage {
        let size = CGSize(width: 32, height: 32)
        let renderer = UIGraphicsImageRenderer(size: size)
        
        return renderer.image { context in
            // Background
            UIColor.darkGray.setFill()
            context.fill(CGRect(origin: .zero, size: size))
            
            // Border
            UIColor.white.setStroke()
            context.stroke(CGRect(origin: .zero, size: size))
            
            // Text
            let attributes: [NSAttributedString.Key: Any] = [
                .font: UIFont.systemFont(ofSize: 16, weight: .bold),
                .foregroundColor: UIColor.white
            ]
            let textSize = text.size(withAttributes: attributes)
            let textRect = CGRect(
                x: (size.width - textSize.width) / 2,
                y: (size.height - textSize.height) / 2,
                width: textSize.width,
                height: textSize.height
            )
            text.draw(in: textRect, withAttributes: attributes)
        }
    }
    
    private func createPlaceholderButton() -> UIImage {
        let size = CGSize(width: 100, height: 44)
        let renderer = UIGraphicsImageRenderer(size: size)
        
        return renderer.image { context in
            UIColor.systemBlue.setFill()
            let rect = CGRect(origin: .zero, size: size)
            let path = UIBezierPath(roundedRect: rect, cornerRadius: 8)
            path.fill()
        }
    }
    
    // MARK: - Cache Management
    
    func clearCache() {
        cacheQueue.async { [weak self] in
            self?.imageCache.removeAll()
        }
    }
    
    func cacheSize() -> Int {
        return cacheQueue.sync { imageCache.count }
    }
}

// MARK: - Supporting Types

enum ButtonState: String {
    case normal = "normal"
    case highlighted = "highlighted"
    case disabled = "disabled"
    case selected = "selected"
}

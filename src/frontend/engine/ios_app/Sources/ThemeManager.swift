import UIKit

/// Protocol defining a UI theme
protocol Theme {
    var identifier: String { get }
    var displayName: String { get }
    var description: String { get }
    
    // Colors
    var primaryColor: UIColor { get }
    var secondaryColor: UIColor { get }
    var accentColor: UIColor { get }
    var backgroundColor: UIColor { get }
    var surfaceColor: UIColor { get }
    var textColor: UIColor { get }
    var textSecondaryColor: UIColor { get }
    
    // Status colors
    var healthColor: UIColor { get }
    var hungerColor: UIColor { get }
    var armorColor: UIColor { get }
    var experienceColor: UIColor { get }
    
    // Fonts
    var titleFont: UIFont { get }
    var bodyFont: UIFont { get }
    var captionFont: UIFont { get }
    
    // UI Properties
    var cornerRadius: CGFloat { get }
    var borderWidth: CGFloat { get }
    var shadowOpacity: Float { get }
}

/// Manages theme switching and application
class ThemeManager {
    
    static let shared = ThemeManager()
    
    private(set) var currentTheme: Theme
    private var observers: [ThemeObserver] = []
    
    // Available themes
    let availableThemes: [Theme] = [
        MedievalTheme(),
        SciFiTheme(),
        ModernTheme()
    ]
    
    private init() {
        // Load saved theme or default to Medieval
        if let savedThemeId = UserDefaults.standard.string(forKey: "selectedTheme"),
           let theme = availableThemes.first(where: { $0.identifier == savedThemeId }) {
            currentTheme = theme
        } else {
            currentTheme = MedievalTheme()
        }
    }
    
    // MARK: - Theme Switching
    
    func applyTheme(_ theme: Theme, animated: Bool = true) {
        currentTheme = theme
        
        // Save preference
        UserDefaults.standard.set(theme.identifier, forKey: "selectedTheme")
        
        // Notify observers
        notifyObservers(animated: animated)
        
        // Clear asset cache to reload themed assets
        AssetManager.shared.clearCache()
    }
    
    func applyTheme(withIdentifier identifier: String, animated: Bool = true) {
        guard let theme = availableThemes.first(where: { $0.identifier == identifier }) else {
            return
        }
        applyTheme(theme, animated: animated)
    }
    
    // MARK: - Observer Pattern
    
    func addObserver(_ observer: ThemeObserver) {
        observers.append(observer)
    }
    
    func removeObserver(_ observer: ThemeObserver) {
        observers.removeAll { $0 === observer }
    }
    
    private func notifyObservers(animated: Bool) {
        for observer in observers {
            observer.themeDidChange(currentTheme, animated: animated)
        }
    }
}

// MARK: - Theme Observer Protocol

protocol ThemeObserver: AnyObject {
    func themeDidChange(_ theme: Theme, animated: Bool)
}

// MARK: - Medieval Theme

struct MedievalTheme: Theme {
    let identifier = "medieval"
    let displayName = "Medieval Fantasy"
    let description = "Classic fantasy RPG aesthetic with warm tones"
    
    // Colors - Warm, earthy tones
    let primaryColor = UIColor(red: 0.55, green: 0.35, blue: 0.20, alpha: 1.0) // Brown
    let secondaryColor = UIColor(red: 0.70, green: 0.55, blue: 0.30, alpha: 1.0) // Tan
    let accentColor = UIColor(red: 0.85, green: 0.65, blue: 0.13, alpha: 1.0) // Gold
    let backgroundColor = UIColor(red: 0.15, green: 0.12, blue: 0.10, alpha: 1.0) // Dark brown
    let surfaceColor = UIColor(red: 0.25, green: 0.20, blue: 0.15, alpha: 1.0) // Medium brown
    let textColor = UIColor(red: 0.95, green: 0.90, blue: 0.80, alpha: 1.0) // Cream
    let textSecondaryColor = UIColor(red: 0.70, green: 0.65, blue: 0.55, alpha: 1.0) // Light brown
    
    // Status colors
    let healthColor = UIColor(red: 0.90, green: 0.20, blue: 0.20, alpha: 1.0) // Red
    let hungerColor = UIColor(red: 0.85, green: 0.55, blue: 0.20, alpha: 1.0) // Orange
    let armorColor = UIColor(red: 0.60, green: 0.65, blue: 0.70, alpha: 1.0) // Steel gray
    let experienceColor = UIColor(red: 0.40, green: 0.80, blue: 0.40, alpha: 1.0) // Green
    
    // Fonts - Serif for medieval feel
    let titleFont = UIFont(name: "Georgia-Bold", size: 24) ?? UIFont.boldSystemFont(ofSize: 24)
    let bodyFont = UIFont(name: "Georgia", size: 16) ?? UIFont.systemFont(ofSize: 16)
    let captionFont = UIFont(name: "Georgia", size: 12) ?? UIFont.systemFont(ofSize: 12)
    
    // UI Properties
    let cornerRadius: CGFloat = 4
    let borderWidth: CGFloat = 2
    let shadowOpacity: Float = 0.5
}

// MARK: - Sci-Fi Theme

struct SciFiTheme: Theme {
    let identifier = "scifi"
    let displayName = "Sci-Fi Futuristic"
    let description = "Sleek futuristic design with cool neon accents"
    
    // Colors - Cool, tech-inspired
    let primaryColor = UIColor(red: 0.10, green: 0.15, blue: 0.25, alpha: 1.0) // Dark blue
    let secondaryColor = UIColor(red: 0.15, green: 0.25, blue: 0.40, alpha: 1.0) // Medium blue
    let accentColor = UIColor(red: 0.00, green: 0.80, blue: 1.00, alpha: 1.0) // Cyan
    let backgroundColor = UIColor(red: 0.05, green: 0.05, blue: 0.10, alpha: 1.0) // Almost black
    let surfaceColor = UIColor(red: 0.12, green: 0.12, blue: 0.18, alpha: 1.0) // Dark gray-blue
    let textColor = UIColor(red: 0.85, green: 0.95, blue: 1.00, alpha: 1.0) // Light cyan
    let textSecondaryColor = UIColor(red: 0.50, green: 0.60, blue: 0.70, alpha: 1.0) // Gray-blue
    
    // Status colors
    let healthColor = UIColor(red: 1.00, green: 0.30, blue: 0.50, alpha: 1.0) // Pink-red
    let hungerColor = UIColor(red: 1.00, green: 0.70, blue: 0.00, alpha: 1.0) // Amber
    let armorColor = UIColor(red: 0.40, green: 0.70, blue: 1.00, alpha: 1.0) // Light blue
    let experienceColor = UIColor(red: 0.00, green: 1.00, blue: 0.60, alpha: 1.0) // Neon green
    
    // Fonts - Sans-serif for modern feel
    let titleFont = UIFont(name: "Futura-Bold", size: 24) ?? UIFont.boldSystemFont(ofSize: 24)
    let bodyFont = UIFont(name: "Futura-Medium", size: 16) ?? UIFont.systemFont(ofSize: 16)
    let captionFont = UIFont(name: "Futura-Medium", size: 12) ?? UIFont.systemFont(ofSize: 12)
    
    // UI Properties
    let cornerRadius: CGFloat = 8
    let borderWidth: CGFloat = 1
    let shadowOpacity: Float = 0.3
}

// MARK: - Modern Theme

struct ModernTheme: Theme {
    let identifier = "modern"
    let displayName = "Modern Minimalist"
    let description = "Clean, contemporary design with subtle colors"
    
    // Colors - Neutral, minimalist
    let primaryColor = UIColor(red: 0.20, green: 0.20, blue: 0.22, alpha: 1.0) // Dark gray
    let secondaryColor = UIColor(red: 0.35, green: 0.35, blue: 0.37, alpha: 1.0) // Medium gray
    let accentColor = UIColor(red: 0.00, green: 0.48, blue: 1.00, alpha: 1.0) // iOS blue
    let backgroundColor = UIColor(red: 0.95, green: 0.95, blue: 0.97, alpha: 1.0) // Light gray
    let surfaceColor = UIColor.white
    let textColor = UIColor(red: 0.10, green: 0.10, blue: 0.10, alpha: 1.0) // Almost black
    let textSecondaryColor = UIColor(red: 0.45, green: 0.45, blue: 0.45, alpha: 1.0) // Gray
    
    // Status colors
    let healthColor = UIColor.systemRed
    let hungerColor = UIColor.systemOrange
    let armorColor = UIColor.systemBlue
    let experienceColor = UIColor.systemGreen
    
    // Fonts - System fonts
    let titleFont = UIFont.systemFont(ofSize: 24, weight: .bold)
    let bodyFont = UIFont.systemFont(ofSize: 16, weight: .regular)
    let captionFont = UIFont.systemFont(ofSize: 12, weight: .regular)
    
    // UI Properties
    let cornerRadius: CGFloat = 12
    let borderWidth: CGFloat = 0
    let shadowOpacity: Float = 0.1
}

// MARK: - UIView Extension for Theme Application

extension UIView {
    
    func applyTheme(_ theme: Theme) {
        backgroundColor = theme.surfaceColor
        layer.cornerRadius = theme.cornerRadius
        layer.borderWidth = theme.borderWidth
        layer.borderColor = theme.primaryColor.cgColor
        layer.shadowOpacity = theme.shadowOpacity
    }
    
    func applyCurrentTheme() {
        applyTheme(ThemeManager.shared.currentTheme)
    }
}

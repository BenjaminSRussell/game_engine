import SwiftUI

// MARK: - Design System

struct DesignSystem {
    
    // MARK: - Colors
    
    struct Colors {
        // Primary Colors
        static let accentPrimary = Color(red: 0.2, green: 0.6, blue: 1.0)
        static let accentSecondary = Color(red: 0.4, green: 0.8, blue: 1.0)
        static let accentWarning = Color.orange
        static let accentError = Color.red
        static let accentSuccess = Color.green
        
        // Background Colors
        static let backgroundPrimary = Color(red: 0.05, green: 0.05, blue: 0.08)
        static let backgroundSecondary = Color(red: 0.08, green: 0.08, blue: 0.12)
        static let backgroundTertiary = Color(red: 0.12, green: 0.12, blue: 0.16)
        
        // Text Colors
        static let textPrimary = Color.white
        static let textSecondary = Color(red: 0.8, green: 0.8, blue: 0.8)
        static let textTertiary = Color(red: 0.6, green: 0.6, blue: 0.6)
        
        // UI Colors
        static let border = Color(red: 0.2, green: 0.2, blue: 0.25)
        static let selection = Color(red: 0.2, green: 0.4, blue: 0.8).opacity(0.3)
        static let selectionStrong = Color(red: 0.3, green: 0.5, blue: 0.9).opacity(0.5)
        static let hover = Color(red: 0.15, green: 0.15, blue: 0.2)
        static let hoverStrong = Color(red: 0.2, green: 0.2, blue: 0.25)
        
        // Gradient Colors
        static let gradientStart = Color(red: 0.1, green: 0.3, blue: 0.6)
        static let gradientEnd = Color(red: 0.2, green: 0.6, blue: 1.0)
    }
    
    // MARK: - Typography
    
    struct Typography {
        static let largeTitle = Font.system(size: 28, weight: .bold, design: .default)
        static let title1 = Font.system(size: 24, weight: .bold, design: .default)
        static let title2 = Font.system(size: 20, weight: .semibold, design: .default)
        static let title3 = Font.system(size: 18, weight: .semibold, design: .default)
        static let headline = Font.system(size: 16, weight: .semibold, design: .default)
        static let body = Font.system(size: 14, weight: .regular, design: .default)
        static let callout = Font.system(size: 13, weight: .medium, design: .default)
        static let subheadline = Font.system(size: 12, weight: .medium, design: .default)
        static let footnote = Font.system(size: 11, weight: .regular, design: .default)
        static let caption = Font.system(size: 10, weight: .medium, design: .default)
        static let small = Font.system(size: 9, weight: .regular, design: .default)
        
        static let mono = Font.system(size: 12, weight: .regular, design: .monospaced)
        static let monoSmall = Font.system(size: 10, weight: .regular, design: .monospaced)
    }
    
    // MARK: - Spacing
    
    struct Spacing {
        static let xxs: CGFloat = 2
        static let xs: CGFloat = 4
        static let sm: CGFloat = 8
        static let md: CGFloat = 12
        static let lg: CGFloat = 16
        static let xl: CGFloat = 20
        static let xxl: CGFloat = 24
        static let xxxl: CGFloat = 32
    }
    
    // MARK: - Corner Radius
    
    struct CornerRadius {
        static let tight: CGFloat = 4
        static let regular: CGFloat = 6
        static let medium: CGFloat = 8
        static let large: CGFloat = 12
        static let xl: CGFloat = 16
    }
    
    // MARK: - Shadows
    
    struct Shadows {
        static let small = Shadow(color: .black.opacity(0.1), radius: 2, x: 0, y: 1)
        static let medium = Shadow(color: .black.opacity(0.15), radius: 4, x: 0, y: 2)
        static let large = Shadow(color: .black.opacity(0.2), radius: 8, x: 0, y: 4)
        static let glow = Shadow(color: DesignSystem.Colors.accentPrimary.opacity(0.3), radius: 8, x: 0, y: 0)
    }
    
    // MARK: - Animations
    
    struct Animations {
        static let quick = Animation.easeInOut(duration: 0.15)
        static let standard = Animation.easeInOut(duration: 0.25)
        static let slow = Animation.easeInOut(duration: 0.35)
        static let spring = Animation.spring(response: 0.3, dampingFraction: 0.7)
        static let bouncy = Animation.spring(response: 0.4, dampingFraction: 0.6)
    }
}

// MARK: - Custom View Modifiers

struct CardBackground: ViewModifier {
    func body(content: Content) -> some View {
        content
            .background(DesignSystem.Colors.backgroundSecondary)
            .cornerRadius(DesignSystem.CornerRadius.medium)
            .shadow(color: .black.opacity(0.1), radius: 2, x: 0, y: 1)
    }
}

struct HoverEffect: ViewModifier {
    @State private var isHovered = false
    
    func body(content: Content) -> some View {
        content
            .scaleEffect(isHovered ? 1.02 : 1.0)
            .animation(DesignSystem.Animations.quick, value: isHovered)
            .onHover { hovering in
                isHovered = hovering
            }
    }
}

struct PressEffect: ViewModifier {
    @State private var isPressed = false
    
    func body(content: Content) -> some View {
        content
            .scaleEffect(isPressed ? 0.95 : 1.0)
            .animation(DesignSystem.Animations.quick, value: isPressed)
            .simultaneousGesture(
                DragGesture(minimumDistance: 0)
                    .onChanged { _ in isPressed = true }
                    .onEnded { _ in isPressed = false }
            )
    }
}

// MARK: - Custom Components

struct EditorButton: View {
    let title: String
    let icon: String?
    let style: ButtonStyle
    let action: () -> Void
    
    @State private var isHovered = false
    @State private var isPressed = false
    
    enum ButtonStyle {
        case primary
        case secondary
        case tertiary
        case destructive
        
        var backgroundColor: Color {
            switch self {
            case .primary: return DesignSystem.Colors.accentPrimary
            case .secondary: return DesignSystem.Colors.backgroundTertiary
            case .tertiary: return Color.clear
            case .destructive: return DesignSystem.Colors.accentError
            }
        }
        
        var foregroundColor: Color {
            switch self {
            case .primary, .destructive: return .white
            case .secondary: return DesignSystem.Colors.textPrimary
            case .tertiary: return DesignSystem.Colors.textSecondary
            }
        }
        
        var hoverColor: Color {
            switch self {
            case .primary: return DesignSystem.Colors.accentSecondary
            case .secondary: return DesignSystem.Colors.hoverStrong
            case .tertiary: return DesignSystem.Colors.hover
            case .destructive: return Color.red.opacity(0.8)
            }
        }
    }
    
    init(_ title: String, icon: String? = nil, style: ButtonStyle = .secondary, action: @escaping () -> Void) {
        self.title = title
        self.icon = icon
        self.style = style
        self.action = action
    }
    
    var body: some View {
        Button(action: action) {
            HStack(spacing: DesignSystem.Spacing.xs) {
                if let icon = icon {
                    Image(systemName: icon)
                        .font(.system(size: 12, weight: .semibold))
                }
                
                Text(title)
                    .font(DesignSystem.Typography.callout)
                    .fontWeight(.medium)
            }
            .foregroundColor(style.foregroundColor)
            .padding(.horizontal, DesignSystem.Spacing.md)
            .padding(.vertical, DesignSystem.Spacing.xs)
            .background(
                RoundedRectangle(cornerRadius: DesignSystem.CornerRadius.tight)
                    .fill(isHovered ? style.hoverColor : style.backgroundColor)
            )
            .scaleEffect(isPressed ? 0.95 : 1.0)
        }
        .buttonStyle(.plain)
        .onHover { hovering in
            withAnimation(DesignSystem.Animations.quick) {
                isHovered = hovering
            }
        }
        .simultaneousGesture(
            DragGesture(minimumDistance: 0)
                .onChanged { _ in
                    withAnimation(DesignSystem.Animations.quick) { isPressed = true }
                }
                .onEnded { _ in
                    withAnimation(DesignSystem.Animations.quick) { isPressed = false }
                }
        )
    }
}

struct EditorIconButton: View {
    let icon: String
    let tooltip: String
    let action: () -> Void
    
    @State private var isHovered = false
    @State private var isPressed = false
    
    init(icon: String, tooltip: String, action: @escaping () -> Void) {
        self.icon = icon
        self.tooltip = tooltip
        self.action = action
    }
    
    var body: some View {
        Button(action: action) {
            Image(systemName: icon)
                .font(.system(size: 14, weight: .medium))
                .foregroundColor(isHovered ? DesignSystem.Colors.textPrimary : DesignSystem.Colors.textSecondary)
                .frame(width: 24, height: 24)
                .background(
                    RoundedRectangle(cornerRadius: 4)
                        .fill(isHovered ? DesignSystem.Colors.hover : Color.clear)
                )
                .scaleEffect(isPressed ? 0.9 : 1.0)
        }
        .buttonStyle(.plain)
        .help(tooltip)
        .onHover { hovering in
            withAnimation(DesignSystem.Animations.quick) {
                isHovered = hovering
            }
        }
        .simultaneousGesture(
            DragGesture(minimumDistance: 0)
                .onChanged { _ in
                    withAnimation(DesignSystem.Animations.quick) { isPressed = true }
                }
                .onEnded { _ in
                    withAnimation(DesignSystem.Animations.quick) { isPressed = false }
                }
        )
    }
}

struct EditorDivider: View {
    var body: some View {
        Rectangle()
            .fill(DesignSystem.Colors.border)
            .frame(height: 1)
    }
}

// MARK: - View Extensions

extension View {
    func cardBackground() -> some View {
        modifier(CardBackground())
    }
    
    func hoverEffect() -> some View {
        modifier(HoverEffect())
    }
    
    func pressEffect() -> some View {
        modifier(PressEffect())
    }
}

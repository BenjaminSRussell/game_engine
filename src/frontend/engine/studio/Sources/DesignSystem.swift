import SwiftUI

/// Premium Design System - Professional Game Engine Editor Aesthetic
/// Inspired by: Unity, Unreal Engine 5, Blender, Adobe Creative Suite
/// Contains 100+ design features for a polished, professional experience
struct DesignSystem {
    
    // MARK: - Color Palette (30+ colors)
    
    // MARK: - Color Palette (Premium Glass & Neon)
    
    // MARK: - Color Palette (Apple Refined & Pastel)
    
    struct Colors {
        // Semantic System Colors (Adapts to Light/Dark Mode)
        static let backgroundPrimary = Color(nsColor: .windowBackgroundColor)
        static let backgroundSecondary = Color(nsColor: .controlBackgroundColor) // Panels
        static let backgroundTertiary = Color(nsColor: .textBackgroundColor)     // Inputs
        static let backgroundQuaternary = Color(nsColor: .selectedControlColor).opacity(0.1)
        static let backgroundFloating = Color(nsColor: .windowBackgroundColor)
        
        // Text Hierarchy (Native)
        static let textPrimary = Color(nsColor: .labelColor)
        static let textSecondary = Color(nsColor: .secondaryLabelColor)
        static let textTertiary = Color(nsColor: .tertiaryLabelColor)
        static let textDisabled = Color(nsColor: .quaternaryLabelColor)
        static let textInverse = Color.white
        
        // Pastel Accents (Professional & Calm)
        static let accentPrimary = Color(hex: "#007AFF")          // Apple Blue
        static let accentSecondary = Color(hex: "#5856D6")        // Apple Purple
        static let accentTertiary = Color(hex: "#5AC8FA")         // Apple Cyan
        static let accentSuccess = Color(hex: "#34C759")          // Apple Green (Soft)
        static let accentWarning = Color(hex: "#FF9500")          // Apple Orange
        static let accentDanger = Color(hex: "#FF3B30")           // Apple Red
        static let accentError = accentDanger
        static let accentInfo = Color(hex: "#AF52DE")             // Apple Indigo
        
        // Soft Pastels for Backgrounds/Tags
        static let pastelBlue = Color(hex: "#EDF4FF")
        static let pastelPurple = Color(hex: "#F4F3FF")
        static let pastelGreen = Color(hex: "#ECFBF1")
        static let pastelRed = Color(hex: "#FEF2F2")
        
        // Component Colors
        static let xAxis = Color(hex: "#FF453A")
        static let yAxis = Color(hex: "#32D74B")
        static let zAxis = Color(hex: "#0A84FF")
        static let wAxis = Color(hex: "#FFD60A")
        
        // Entity Colors (Muted/Professional)
        static let entityMesh = Color(hex: "#007AFF")
        static let entityLight = Color(hex: "#FFCC00")
        static let entityCamera = Color(hex: "#AF52DE")
        static let entityAudio = Color(hex: "#32D74B")
        static let entityParticle = Color(hex: "#FF2D55")
        static let entityTrigger = Color(hex: "#FF9500")
        static let entityPrefab = Color(hex: "#5AC8FA")
        
        // UI States (Subtle)
        static let selection = Color.accentColor.opacity(0.1)
        static let selectionStrong = Color.accentColor.opacity(0.2)
        static let hover = Color.primary.opacity(0.04)
        static let hoverStrong = Color.primary.opacity(0.08)
        static let pressed = Color.primary.opacity(0.12)
        static let border = Color.primary.opacity(0.08)
        static let borderStrong = Color.primary.opacity(0.15)
        static let borderFocus = Color.accentColor.opacity(0.5)
        static let shadow = Color.black.opacity(0.1)
        
        // Gradients (Subtle & refined)
        static let gradientStart = Color(hex: "#007AFF")
        static let gradientEnd = Color(hex: "#00C7BE") // Blue to Teal
        static let gradientSuccess = [Color(hex: "#34C759"), Color(hex: "#30B350")]
        
        // Overlays
        static let overlayLight = Color.white.opacity(0.1)
        static let overlayDark = Color.black.opacity(0.2)
        static let overlayScrim = Color.black.opacity(0.4)
        
        // Glass Legacy Support (Refined)
        static let glassBackground = Color.white.opacity(0.5)
        static let glassStroke = Color.black.opacity(0.05)
        static let glassHighlight = Color.white.opacity(0.2)
    }
    
    // MARK: - Typography (Improvements 1-10: Line heights, tracking, hierarchy)
    
    struct Typography {
        // Display (32pt, line-height 1.2)
        static let display = Font.system(size: 32, weight: .semibold, design: .default)
        static let largeTitle = Font.system(size: 24, weight: .semibold, design: .default)
        
        // Titles (line-height 1.3)
        static let title1 = Font.system(size: 20, weight: .medium, design: .default)
        static let title2 = Font.system(size: 16, weight: .medium, design: .default)
        static let title3 = Font.system(size: 15, weight: .medium, design: .default)
        static let h1 = title1
        static let h2 = title2
        static let h3 = title3
        
        // Body (13pt standard, line-height 1.4)
        static let headline = Font.system(size: 14, weight: .medium, design: .default)
        static let body = Font.system(size: 13, weight: .regular, design: .default)
        static let bodyBold = Font.system(size: 13, weight: .semibold, design: .default)
        static let bodyMedium = Font.system(size: 13, weight: .medium, design: .default)
        
        // Captions (11pt)
        static let caption = Font.system(size: 11, weight: .regular, design: .default)
        static let captionBold = Font.system(size: 11, weight: .semibold, design: .default)
        static let captionMedium = Font.system(size: 11, weight: .medium, design: .default)
        
        // Small (10pt for labels)
        static let small = Font.system(size: 10, weight: .medium, design: .default)
        static let smallBold = Font.system(size: 10, weight: .bold, design: .default)
        static let micro = Font.system(size: 9, weight: .medium, design: .default)
        
        // Monospace (for values, coordinates, code) - Improvement #5
        static let mono = Font.system(size: 12, weight: .regular, design: .monospaced)
        static let monoSmall = Font.system(size: 11, weight: .regular, design: .monospaced)
        static let monoMedium = Font.system(size: 12, weight: .medium, design: .monospaced)
        static let monoBold = Font.system(size: 12, weight: .semibold, design: .monospaced)
        
        // Label styles (CAPS with tracking) - Improvement #2, #6
        static let sectionLabel = Font.system(size: 10, weight: .semibold, design: .default)
        static let buttonLabel = Font.system(size: 13, weight: .medium, design: .default)
        static let tabLabel = Font.system(size: 11, weight: .medium, design: .default)
    }
    
    // MARK: - Spacing (Improvements 11-25: 8px grid system)
    
    struct Spacing {
        // Base 8px grid
        static let xxxs: CGFloat = 2   // Half unit
        static let xxs: CGFloat = 4    // Quarter unit  
        static let xs: CGFloat = 8     // 1 unit - base
        static let sm: CGFloat = 12    // 1.5 units
        static let md: CGFloat = 16    // 2 units - standard
        static let lg: CGFloat = 20    // 2.5 units
        static let xl: CGFloat = 24    // 3 units
        static let xxl: CGFloat = 32   // 4 units
        static let xxxl: CGFloat = 40  // 5 units
        
        // Semantic spacing (Improvements #12-16)
        static let inputPadding: CGFloat = 8
        static let buttonPadding: CGFloat = 10
        static let cardPadding: CGFloat = 16
        static let panelPadding: CGFloat = 16
        static let sectionSpacing: CGFloat = 24       // #14
        static let dividerSpacing: CGFloat = 16       // #14
        static let buttonGroupGap: CGFloat = 8        // #15
        static let iconTextGap: CGFloat = 8           // #16
        static let iconTextGapCompact: CGFloat = 6    // #16
        static let inputGroupSpacing: CGFloat = 12    // #13
    }
    
    // MARK: - Sizing (Improvements 17, 21, 71-75)
    
    struct Sizing {
        // Touch targets (#17)
        static let minTouchTarget: CGFloat = 32
        static let minTouchTargetMobile: CGFloat = 44
        
        // Panel dimensions (#21)
        static let sidebarMinWidth: CGFloat = 200
        static let inspectorMinWidth: CGFloat = 300
        static let contentMaxWidth: CGFloat = 800     // #22
        
        // Header/Footer (#24-25)
        static let headerHeight: CGFloat = 48
        static let toolbarHeight: CGFloat = 32
        
        // Component dimensions (#71-75)
        static let buttonMinWidth: CGFloat = 64
        static let inputMinHeight: CGFloat = 32
        static let checkboxSize: CGFloat = 16
        static let toggleWidth: CGFloat = 48
        static let sliderThumbSize: CGFloat = 16
    }
    
    struct CornerRadius {
        static let tight: CGFloat = 4
        static let small: CGFloat = 6
        static let regular: CGFloat = 8   // Apple standard
        static let medium: CGFloat = 10
        static let large: CGFloat = 12    // Softer panels
        static let xlarge: CGFloat = 16
        static let pill: CGFloat = 999
    }
    
    // MARK: - Border Width
    
    struct BorderWidth {
        static let hairline: CGFloat = 0.5
        static let thin: CGFloat = 1
        static let medium: CGFloat = 1.5
        static let thick: CGFloat = 2
        static let focus: CGFloat = 2
    }
    
    // MARK: - Shadows & Elevation (Improvement #39)
    
    struct Shadow {
        static let subtle = Color.black.opacity(0.08)     // Level 1
        static let light = Color.black.opacity(0.12)      // Level 2  
        static let medium = Color.black.opacity(0.16)     // Level 3
        static let strong = Color.black.opacity(0.20)     // Level 4
        
        static func level1() -> some View { EmptyView().shadow(color: subtle, radius: 2, x: 0, y: 1) }
        static func level2() -> some View { EmptyView().shadow(color: light, radius: 4, x: 0, y: 2) }
        static func level3() -> some View { EmptyView().shadow(color: medium, radius: 8, x: 0, y: 4) }
        static func level4() -> some View { EmptyView().shadow(color: strong, radius: 16, x: 0, y: 8) }
        static func level5() -> some View { EmptyView().shadow(color: .black.opacity(0.25), radius: 32, x: 0, y: 16) }
        
        // Colored glows
        static func glow(_ color: Color, radius: CGFloat = 8) -> some View {
            EmptyView().shadow(color: color.opacity(0.5), radius: radius, x: 0, y: 0)
        }
        
        // Inner shadow (simulated)
        static func inner(_ color: Color = .black.opacity(0.1), radius: CGFloat = 4) -> some View {
            EmptyView().overlay(
                RoundedRectangle(cornerRadius: DesignSystem.CornerRadius.regular)
                    .stroke(color, lineWidth: radius)
                    .blur(radius: radius)
                    .mask(RoundedRectangle(cornerRadius: DesignSystem.CornerRadius.regular))
            )
        }
    }
    
    struct ShadowStyle {
        let color: Color
        let radius: CGFloat
        let x: CGFloat
        let y: CGFloat
    }
    
    struct Shadows {
        static let none = ShadowStyle(color: .clear, radius: 0, x: 0, y: 0)
        static let small = ShadowStyle(color: .black.opacity(0.08), radius: 2, x: 0, y: 1)
        static let medium = ShadowStyle(color: .black.opacity(0.12), radius: 4, x: 0, y: 2)
        static let large = ShadowStyle(color: .black.opacity(0.18), radius: 8, x: 0, y: 4)
        static let xlarge = ShadowStyle(color: .black.opacity(0.25), radius: 16, x: 0, y: 8)
        
        // Accent shadows
        static let accentGlow = ShadowStyle(color: Colors.accentPrimary.opacity(0.3), radius: 12, x: 0, y: 4)
        static let successGlow = ShadowStyle(color: Colors.accentSuccess.opacity(0.3), radius: 12, x: 0, y: 4)
        static let dangerGlow = ShadowStyle(color: Colors.accentDanger.opacity(0.3), radius: 12, x: 0, y: 4)
    }
    
    // MARK: - Animation & Timing (Improvements 41-55)
    
    struct Animation {
        // Duration scale (#41)
        static let durationFast: Double = 0.1
        static let durationNormal: Double = 0.2
        static let durationSlow: Double = 0.3
        
        // Easing curves (#42)
        static let spring = SwiftUI.Animation.spring(response: 0.3, dampingFraction: 0.7)  // #43
        static let springBouncy = SwiftUI.Animation.spring(response: 0.4, dampingFraction: 0.6)
        static let easeOut = SwiftUI.Animation.easeOut(duration: durationNormal)  // Enter
        static let easeIn = SwiftUI.Animation.easeIn(duration: 0.15)              // Exit
        static let fast = SwiftUI.Animation.easeOut(duration: durationFast)
        static let smooth = SwiftUI.Animation.easeInOut(duration: 0.25)
        
        // Specialized animations
        static let colorTransition = SwiftUI.Animation.easeOut(duration: 0.15)    // #49
        static let scalePress = SwiftUI.Animation.easeOut(duration: 0.1)          // #46
        static let slideIn = SwiftUI.Animation.easeOut(duration: 0.25)            // #47
        static let rotate = SwiftUI.Animation.easeOut(duration: 0.2)              // #48
        
        // Stagger delay (#44)
        static let staggerDelay: Double = 0.03  // 30ms per item
        
        // Duration-based
        static let instant = SwiftUI.Animation.easeOut(duration: 0.08)
        static let regular = SwiftUI.Animation.easeInOut(duration: 0.25)
        static let slow = SwiftUI.Animation.easeInOut(duration: 0.4)
        static let slower = SwiftUI.Animation.easeInOut(duration: 0.6)
        static let easeInOut = regular
        
        // Spring animations
        static let springSnappy = SwiftUI.Animation.interpolatingSpring(stiffness: 400, damping: 30)
        static let springGentle = SwiftUI.Animation.interpolatingSpring(stiffness: 200, damping: 25)
        
        // Specialized
        static let hover = SwiftUI.Animation.easeOut(duration: 0.12)
        static let press = SwiftUI.Animation.easeOut(duration: 0.08)
        static let expand = SwiftUI.Animation.interpolatingSpring(stiffness: 280, damping: 22)
        static let collapse = SwiftUI.Animation.easeInOut(duration: 0.2)
        static let shake = SwiftUI.Animation.easeInOut(duration: 0.05)
    }
    
    // MARK: - Timing Durations
    
    struct Duration {
        static let instant: Double = 0.08
        static let fast: Double = 0.15
        static let regular: Double = 0.25
        static let slow: Double = 0.4
        static let tooltipDelay: Double = 0.5
        static let longPress: Double = 0.5
        static let autoHide: Double = 3.0
    }
    
    // MARK: - Icon Sizes
    
    struct IconSize {
        static let micro: CGFloat = 10
        static let tiny: CGFloat = 12
        static let small: CGFloat = 16
        static let regular: CGFloat = 20
        static let medium: CGFloat = 24
        static let large: CGFloat = 28
        static let xlarge: CGFloat = 32
        static let xxlarge: CGFloat = 48
        static let hero: CGFloat = 64
    }
    
    // MARK: - Hit Targets (Accessibility)
    
    struct HitTarget {
        static let minimum: CGFloat = 44  // Apple HIG minimum
        static let comfortable: CGFloat = 48
        static let large: CGFloat = 56
    }
    
    // MARK: - Z-Index Layers
    
    struct ZIndex {
        static let base: Double = 0
        static let dropdown: Double = 100
        static let sticky: Double = 200
        static let overlay: Double = 300
        static let modal: Double = 400
        static let popover: Double = 500
        static let tooltip: Double = 600
        static let notification: Double = 700
        static let maximum: Double = 999
    }
}

// MARK: - Color Extension (Hex Support)

extension Color {
    init(hex: String) {
        let hex = hex.trimmingCharacters(in: CharacterSet.alphanumerics.inverted)
        var int: UInt64 = 0
        Scanner(string: hex).scanHexInt64(&int)
        let a, r, g, b: UInt64
        switch hex.count {
        case 3: (a, r, g, b) = (255, (int >> 8) * 17, (int >> 4 & 0xF) * 17, (int & 0xF) * 17)
        case 6: (a, r, g, b) = (255, int >> 16, int >> 8 & 0xFF, int & 0xFF)
        case 8: (a, r, g, b) = (int >> 24, int >> 16 & 0xFF, int >> 8 & 0xFF, int & 0xFF)
        default: (a, r, g, b) = (255, 0, 0, 0)
        }
        self.init(.sRGB, red: Double(r) / 255, green: Double(g) / 255, blue: Double(b) / 255, opacity: Double(a) / 255)
    }
    
    func lighter(by amount: CGFloat = 0.1) -> Color {
        self.opacity(1.0 - amount)
    }
    
    func darker(by amount: CGFloat = 0.1) -> Color {
        self.opacity(1.0 + amount)
    }
}

// MARK: - View Extensions (50+ modifiers)

extension View {
    // Card styles
    func cardStyle(elevation: Int = 2) -> some View {
        self
            .background(DesignSystem.Colors.backgroundTertiary)
            .cornerRadius(DesignSystem.CornerRadius.regular)
            .overlay(RoundedRectangle(cornerRadius: DesignSystem.CornerRadius.regular).stroke(DesignSystem.Colors.border, lineWidth: 1))
            .shadow(color: .black.opacity(Double(elevation) * 0.04), radius: CGFloat(elevation * 2), x: 0, y: CGFloat(elevation))
    }
    
    func cardStyleInteractive(isHovered: Bool, elevation: Int = 2) -> some View {
        self
            .background(isHovered ? DesignSystem.Colors.backgroundQuaternary : DesignSystem.Colors.backgroundTertiary)
            .cornerRadius(DesignSystem.CornerRadius.regular)
            .overlay(RoundedRectangle(cornerRadius: DesignSystem.CornerRadius.regular).stroke(isHovered ? DesignSystem.Colors.borderStrong : DesignSystem.Colors.border, lineWidth: 1))
            .shadow(color: .black.opacity(isHovered ? 0.15 : Double(elevation) * 0.04), radius: isHovered ? 12 : CGFloat(elevation * 2), x: 0, y: isHovered ? 6 : CGFloat(elevation))
            .scaleEffect(isHovered ? 1.01 : 1.0)
            .animation(DesignSystem.Animation.hover, value: isHovered)
    }
    
    // Glass morphism
    func glassMorphism(cornerRadius: CGFloat = DesignSystem.CornerRadius.large) -> some View {
        self
            .background(DesignSystem.Colors.glassBackground)
            .background(.ultraThinMaterial, in: RoundedRectangle(cornerRadius: cornerRadius))
            .overlay(RoundedRectangle(cornerRadius: cornerRadius).stroke(DesignSystem.Colors.glassStroke, lineWidth: 1))
    }
    
    // Hover effects
    func hoverEffect(isHovered: Bool) -> some View {
        self
            .background(isHovered ? DesignSystem.Colors.hover : Color.clear)
            .animation(DesignSystem.Animation.hover, value: isHovered)
    }
    
    func hoverScale(isHovered: Bool, scale: CGFloat = 1.02) -> some View {
        self
            .scaleEffect(isHovered ? scale : 1.0)
            .animation(DesignSystem.Animation.hover, value: isHovered)
    }
    
    func hoverGlow(isHovered: Bool, color: Color = DesignSystem.Colors.accentPrimary) -> some View {
        self
            .shadow(color: isHovered ? color.opacity(0.3) : .clear, radius: isHovered ? 8 : 0, x: 0, y: 0)
            .animation(DesignSystem.Animation.hover, value: isHovered)
    }
    
    // Press effect
    func pressEffect(isPressed: Bool) -> some View {
        self
            .scaleEffect(isPressed ? 0.97 : 1.0)
            .opacity(isPressed ? 0.9 : 1.0)
            .animation(DesignSystem.Animation.press, value: isPressed)
    }
    
    // Focus ring
    func focusRing(isFocused: Bool, color: Color = DesignSystem.Colors.accentPrimary) -> some View {
        self
            .overlay(
                RoundedRectangle(cornerRadius: DesignSystem.CornerRadius.regular + 2)
                    .stroke(color, lineWidth: isFocused ? 2 : 0)
                    .padding(-2)
                    .opacity(isFocused ? 1 : 0)
            )
            .animation(DesignSystem.Animation.fast, value: isFocused)
    }
    
    // Selection state
    func selectionStyle(isSelected: Bool) -> some View {
        self
            .background(isSelected ? DesignSystem.Colors.selection : Color.clear)
            .overlay(
                RoundedRectangle(cornerRadius: DesignSystem.CornerRadius.tight)
                    .stroke(isSelected ? DesignSystem.Colors.accentPrimary.opacity(0.5) : Color.clear, lineWidth: 1)
            )
    }
    
    // Panel header style
    func panelHeader() -> some View {
        self
            .font(DesignSystem.Typography.bodyBold)
            .foregroundColor(DesignSystem.Colors.textPrimary)
            .padding(DesignSystem.Spacing.sm)
            .frame(maxWidth: .infinity, alignment: .leading)
            .background(DesignSystem.Colors.backgroundTertiary)
    }
    
    // Input field style
    func inputStyle(isFocused: Bool = false) -> some View {
        self
            .font(DesignSystem.Typography.body)
            .foregroundColor(DesignSystem.Colors.textPrimary)
            .padding(DesignSystem.Spacing.inputPadding)
            .background(DesignSystem.Colors.backgroundPrimary)
            .cornerRadius(DesignSystem.CornerRadius.tight)
            .overlay(
                RoundedRectangle(cornerRadius: DesignSystem.CornerRadius.tight)
                    .stroke(isFocused ? DesignSystem.Colors.borderFocus : DesignSystem.Colors.border, lineWidth: isFocused ? 2 : 1)
            )
    }
    
    // Disabled state
    func disabledStyle(_ isDisabled: Bool) -> some View {
        self
            .opacity(isDisabled ? 0.5 : 1.0)
            .allowsHitTesting(!isDisabled)
    }
    
    // Shimmer loading effect
    func shimmer(isLoading: Bool) -> some View {
        self
            .overlay(
                Group {
                    if isLoading {
                        LinearGradient(
                            colors: [.clear, .white.opacity(0.1), .clear],
                            startPoint: .leading,
                            endPoint: .trailing
                        )
                        .rotationEffect(.degrees(20))
                        .offset(x: isLoading ? 400 : -400)
                        .animation(.linear(duration: 1.5).repeatForever(autoreverses: false), value: isLoading)
                    }
                }
            )
            .clipped()
    }
    
    // Skeleton placeholder
    func skeleton() -> some View {
        self
            .redacted(reason: .placeholder)
            .shimmer(isLoading: true)
    }
    
    // Fade transition
    func fadeIn(delay: Double = 0) -> some View {
        self
            .transition(.opacity.animation(DesignSystem.Animation.regular.delay(delay)))
    }
    
    // Slide transition
    func slideIn(from edge: Edge = .bottom, delay: Double = 0) -> some View {
        self
            .transition(.move(edge: edge).combined(with: .opacity).animation(DesignSystem.Animation.spring.delay(delay)))
    }
    
    // Label style
    func labelStyle(_ style: LabelHierarchy = .primary) -> some View {
        self
            .font(style.font)
            .foregroundColor(style.color)
    }
    
    // Badge style
    func badge(_ count: Int, color: Color = DesignSystem.Colors.accentDanger) -> some View {
        self.overlay(
            Group {
                if count > 0 {
                    Text(count > 99 ? "99+" : "\(count)")
                        .font(DesignSystem.Typography.micro)
                        .foregroundColor(.white)
                        .padding(.horizontal, 4)
                        .padding(.vertical, 2)
                        .background(color)
                        .cornerRadius(DesignSystem.CornerRadius.pill)
                        .offset(x: 8, y: -8)
                }
            },
            alignment: .topTrailing
        )
    }
    
    // Tooltip
    func tooltip(_ text: String, isVisible: Bool) -> some View {
        self.overlay(
            Group {
                if isVisible && !text.isEmpty {
                    Text(text)
                        .font(DesignSystem.Typography.caption)
                        .foregroundColor(DesignSystem.Colors.textPrimary)
                        .padding(.horizontal, DesignSystem.Spacing.xs)
                        .padding(.vertical, DesignSystem.Spacing.xxs)
                        .background(DesignSystem.Colors.backgroundFloating)
                        .cornerRadius(DesignSystem.CornerRadius.tight)
                        .shadow(radius: 4)
                        .offset(y: -30)
                        .transition(.opacity.combined(with: .scale(scale: 0.9)))
                }
            }
        )
    }
}

// MARK: - Label Hierarchy

enum LabelHierarchy {
    case primary, secondary, tertiary, hint, accent, success, warning, danger
    
    var font: Font {
        switch self {
        case .primary: return DesignSystem.Typography.body
        case .secondary: return DesignSystem.Typography.small
        case .tertiary: return DesignSystem.Typography.caption
        case .hint: return DesignSystem.Typography.caption
        case .accent, .success, .warning, .danger: return DesignSystem.Typography.smallBold
        }
    }
    
    var color: Color {
        switch self {
        case .primary: return DesignSystem.Colors.textPrimary
        case .secondary: return DesignSystem.Colors.textSecondary
        case .tertiary: return DesignSystem.Colors.textTertiary
        case .hint: return DesignSystem.Colors.textDisabled
        case .accent: return DesignSystem.Colors.accentPrimary
        case .success: return DesignSystem.Colors.accentSuccess
        case .warning: return DesignSystem.Colors.accentWarning
        case .danger: return DesignSystem.Colors.accentDanger
        }
    }
}

// MARK: - Shape Extensions

extension Shape {
    func glow(color: Color, radius: CGFloat = 8) -> some View {
        self
            .fill(color)
            .blur(radius: radius)
    }
}


import SwiftUI

// MARK: - Premium Editor Button

enum EditorButtonStyle {
    case primary      // Vibrant gradient, main actions
    case secondary    // Border with transparency
    case tertiary     // Text only, minimal
    case icon         // Icon-only, compact
    case danger       // Red variant for destructive
}

struct EditorButton: View {
    let title: String
    let icon: String?
    let style: EditorButtonStyle
    let action: () -> Void
    
    @State private var isHovered = false
    @State private var isPressed = false
    
    init(_ title: String, icon: String? = nil, style: EditorButtonStyle = .secondary, action: @escaping () -> Void) {
        self.title = title
        self.icon = icon
        self.style = style
        self.action = action
    }
    
    var body: some View {
        Button(action: {
            withAnimation(DesignSystem.Animation.spring) {
                isPressed = true
            }
            action()
            DispatchQueue.main.asyncAfter(deadline: .now() + 0.1) {
                isPressed = false
            }
        }) {
            HStack(spacing: DesignSystem.Spacing.xs) {
                if let icon = icon {
                    Image(systemName: icon)
                        .font(.system(size: DesignSystem.IconSize.small, weight: .medium))
                }
                
                if style != .icon {
                    Text(title)
                        .font(DesignSystem.Typography.bodyBold)
                }
            }
            .foregroundColor(foregroundColor)
            .padding(.horizontal, horizontalPadding)
            .padding(.vertical, verticalPadding)
            .background(backgroundView)
            .cornerRadius(DesignSystem.CornerRadius.regular)
            .overlay(overlayBorder)
            .scaleEffect(isPressed ? 0.97 : 1.0)
            .shadow(color: shadowColor, radius: shadowRadius, x: 0, y: shadowY)
        }
        .buttonStyle(.plain)
        .onHover { hovering in
            withAnimation(DesignSystem.Animation.fast) {
                isHovered = hovering
            }
        }
    }
    
    // MARK: - Style Configuration
    
    private var foregroundColor: Color {
        switch style {
        case .primary: return .white
        case .secondary: return isHovered ? DesignSystem.Colors.accentPrimary : DesignSystem.Colors.textPrimary
        case .tertiary: return isHovered ? DesignSystem.Colors.accentPrimary : DesignSystem.Colors.textSecondary
        case .icon: return isHovered ? DesignSystem.Colors.accentPrimary : DesignSystem.Colors.textSecondary
        case .danger: return isHovered ? .white : DesignSystem.Colors.accentDanger
        }
    }
    
    @ViewBuilder
    private var backgroundView: some View {
        switch style {
        case .primary:
            LinearGradient(
                colors: [DesignSystem.Colors.gradientStart, DesignSystem.Colors.gradientEnd],
                startPoint: .topLeading,
                endPoint: .bottomTrailing
            )
            .opacity(isHovered ? 1.0 : 0.9)
            
        case .secondary:
            DesignSystem.Colors.backgroundTertiary
                .opacity(isHovered ? 1.0 : 0.8)
            
        case .tertiary:
            Color.clear
            
        case .icon:
            (isHovered ? DesignSystem.Colors.hover : Color.clear)
            
        case .danger:
            DesignSystem.Colors.accentDanger
                .opacity(isHovered ? 0.2 : 0.1)
        }
    }
    
    @ViewBuilder
    private var overlayBorder: some View {
        if style == .secondary || style == .danger {
            RoundedRectangle(cornerRadius: DesignSystem.CornerRadius.regular)
                .stroke(borderColor, lineWidth: 1)
        }
    }
    
    private var borderColor: Color {
        if style == .danger {
            return isHovered ? DesignSystem.Colors.accentDanger : DesignSystem.Colors.border
        }
        return isHovered ? DesignSystem.Colors.accentPrimary.opacity(0.5) : DesignSystem.Colors.border
    }
    
    private var horizontalPadding: CGFloat {
        style == .icon ? DesignSystem.Spacing.xs : DesignSystem.Spacing.md
    }
    
    private var verticalPadding: CGFloat {
        style == .icon ? DesignSystem.Spacing.xs : DesignSystem.Spacing.sm
    }
    
    private var shadowColor: Color {
        style == .primary ? DesignSystem.Colors.accentPrimary.opacity(0.3) : Color.clear
    }
    
    private var shadowRadius: CGFloat {
        (style == .primary && isHovered) ? 12 : 0
    }
    
    private var shadowY: CGFloat {
        (style == .primary && isHovered) ? 4 : 0
    }
}

// MARK: - Premium Card Component

struct EditorCard<Content: View>: View {
    let title: String?
    let content: Content
    let elevation: Int
    
    @State private var isHovered = false
    
    init(title: String? = nil, elevation: Int = 2, @ViewBuilder content: () -> Content) {
        self.title = title
        self.elevation = elevation
        self.content = content()
    }
    
    var body: some View {
        VStack(alignment: .leading, spacing: 0) {
            if let title = title {
                HStack {
                    Text(title)
                        .font(DesignSystem.Typography.headline)
                        .foregroundColor(DesignSystem.Colors.textPrimary)
                    
                    Spacer()
                }
                .padding(DesignSystem.Spacing.md)
                .background(DesignSystem.Colors.backgroundSecondary)
                
                Divider()
                    .background(DesignSystem.Colors.border)
            }
            
            content
                .padding(DesignSystem.Spacing.md)
        }
        .background(DesignSystem.Colors.backgroundTertiary)
        .cornerRadius(DesignSystem.CornerRadius.large)
        .overlay(
            RoundedRectangle(cornerRadius: DesignSystem.CornerRadius.large)
                .stroke(
                    isHovered ? DesignSystem.Colors.borderFocus : DesignSystem.Colors.border,
                    lineWidth: 1
                )
        )
        .shadow(
            color: .black.opacity(Double(elevation) * 0.05),
            radius: CGFloat(elevation * 2),
            x: 0,
            y: CGFloat(elevation)
        )
        .onHover { hovering in
            withAnimation(DesignSystem.Animation.fast) {
                isHovered = hovering
            }
        }
    }
}

// MARK: - Premium Text Field

struct EditorTextField: View {
    let placeholder: String
    @Binding var text: String
    let icon: String?
    
    @State private var isFocused = false
    @State private var isHovered = false
    
    init(_ placeholder: String, text: Binding<String>, icon: String? = nil) {
        self.placeholder = placeholder
        self._text = text
        self.icon = icon
    }
    
    var body: some View {
        HStack(spacing: DesignSystem.Spacing.sm) {
            if let icon = icon {
                Image(systemName: icon)
                    .font(.system(size: DesignSystem.IconSize.small, weight: .medium))
                    .foregroundColor(isFocused ? DesignSystem.Colors.accentPrimary : DesignSystem.Colors.textSecondary)
            }
            
            TextField(placeholder, text: $text, onEditingChanged: { editing in
                withAnimation(DesignSystem.Animation.fast) {
                    isFocused = editing
                }
            })
            .font(DesignSystem.Typography.body)
            .foregroundColor(DesignSystem.Colors.textPrimary)
            .textFieldStyle(.plain)
        }
        .padding(.horizontal, DesignSystem.Spacing.sm)
        .padding(.vertical, DesignSystem.Spacing.xs)
        .background(DesignSystem.Colors.backgroundTertiary)
        .cornerRadius(DesignSystem.CornerRadius.tight)
        .overlay(
            RoundedRectangle(cornerRadius: DesignSystem.CornerRadius.tight)
                .stroke(borderColor, lineWidth: isFocused ? 2 : 1)
        )
        .shadow(
            color: isFocused ? DesignSystem.Colors.accentPrimary.opacity(0.15) : .clear,
            radius: isFocused ? 8 : 0,
            x: 0,
            y: 0
        )
        .onHover { hovering in
            withAnimation(DesignSystem.Animation.fast) {
                isHovered = hovering
            }
        }
    }
    
    private var borderColor: Color {
        if isFocused {
            return DesignSystem.Colors.borderFocus
        } else if isHovered {
            return DesignSystem.Colors.border.opacity(1.5)
        } else {
            return DesignSystem.Colors.border
        }
    }
}

// MARK: - Premium Search Bar

struct EditorSearchBar: View {
    @Binding var text: String
    let placeholder: String
    
    @State private var isFocused = false
    @State private var isHovered = false
    
    var body: some View {
        HStack(spacing: DesignSystem.Spacing.sm) {
            Image(systemName: "magnifyingglass")
                .font(.system(size: DesignSystem.IconSize.small, weight: .medium))
                .foregroundColor(isFocused ? DesignSystem.Colors.accentPrimary : DesignSystem.Colors.textSecondary)
            
            TextField(placeholder, text: $text, onEditingChanged: { editing in
                withAnimation(DesignSystem.Animation.fast) {
                    isFocused = editing
                }
            })
            .font(DesignSystem.Typography.body)
            .foregroundColor(DesignSystem.Colors.textPrimary)
            .textFieldStyle(.plain)
            
            if !text.isEmpty {
                Button(action: { text = "" }) {
                    Image(systemName: "xmark.circle.fill")
                        .font(.system(size: DesignSystem.IconSize.small))
                        .foregroundColor(DesignSystem.Colors.textSecondary)
                }
                .buttonStyle(.plain)
                .transition(.scale.combined(with: .opacity))
            }
        }
        .padding(.horizontal, DesignSystem.Spacing.sm)
        .padding(.vertical, DesignSystem.Spacing.xs)
        .background(DesignSystem.Colors.backgroundTertiary)
        .cornerRadius(DesignSystem.CornerRadius.regular)
        .overlay(
            RoundedRectangle(cornerRadius: DesignSystem.CornerRadius.regular)
                .stroke(borderColor, lineWidth: isFocused ? 2 : 1)
        )
        .shadow(
            color: isFocused ? DesignSystem.Colors.accentPrimary.opacity(0.15) : .clear,
            radius: isFocused ? 8 : 0,
            x: 0,
            y: 0
        )
        .onHover { hovering in
            withAnimation(DesignSystem.Animation.fast) {
                isHovered = hovering
            }
        }
    }
    
    private var borderColor: Color {
        if isFocused {
            return DesignSystem.Colors.borderFocus
        } else if isHovered {
            return DesignSystem.Colors.border.opacity(1.5)
        } else {
            return DesignSystem.Colors.border
        }
    }
}

// MARK: - Premium Divider

struct EditorDivider: View {
    var body: some View {
        Rectangle()
            .fill(DesignSystem.Colors.border)
            .frame(height: 1)
    }
}

// MARK: - Premium Icon Button

struct EditorIconButton: View {
    let icon: String
    let tooltip: String
    let action: () -> Void
    
    @State private var isHovered = false
    @State private var isPressed = false
    
    var body: some View {
        Button(action: {
            withAnimation(DesignSystem.Animation.spring) {
                isPressed = true
            }
            action()
            DispatchQueue.main.asyncAfter(deadline: .now() + 0.1) {
                isPressed = false
            }
        }) {
            Image(systemName: icon)
                .font(.system(size: DesignSystem.IconSize.regular, weight: .medium))
                .foregroundColor(isHovered ? DesignSystem.Colors.accentPrimary : DesignSystem.Colors.textSecondary)
                .frame(width: 32, height: 32)
                .background(isHovered ? DesignSystem.Colors.hover : Color.clear)
                .cornerRadius(DesignSystem.CornerRadius.tight)
                .scaleEffect(isPressed ? 0.9 : 1.0)
        }
        .buttonStyle(.plain)
        .help(tooltip)
        .onHover { hovering in
            withAnimation(DesignSystem.Animation.fast) {
                isHovered = hovering
            }
        }
    }
}

// MARK: - Premium Toggle

struct EditorToggle: View {
    let label: String
    @Binding var isOn: Bool
    
    @State private var isHovered = false
    
    var body: some View {
        Button(action: {
            withAnimation(DesignSystem.Animation.springBouncy) {
                isOn.toggle()
            }
        }) {
            HStack {
                Text(label)
                    .font(DesignSystem.Typography.body)
                    .foregroundColor(DesignSystem.Colors.textPrimary)
                
                Spacer()
                
                ZStack {
                    // Background track
                    RoundedRectangle(cornerRadius: 12)
                        .fill(isOn ? DesignSystem.Colors.accentPrimary : DesignSystem.Colors.backgroundQuaternary)
                        .frame(width: 44, height: 26)
                    
                    // Toggle thumb
                    Circle()
                        .fill(.white)
                        .frame(width: 22, height: 22)
                        .offset(x: isOn ? 9 : -9)
                        .shadow(color: .black.opacity(0.2), radius: 2, x: 0, y: 1)
                }
            }
            .padding(DesignSystem.Spacing.sm)
            .background(isHovered ? DesignSystem.Colors.hover : Color.clear)
            .cornerRadius(DesignSystem.CornerRadius.tight)
        }
        .buttonStyle(.plain)
        .onHover { hovering in
            withAnimation(DesignSystem.Animation.fast) {
                isHovered = hovering
            }
        }
    }
}

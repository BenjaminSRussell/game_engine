import SwiftUI

// MARK: - Animation Effect Modifiers (50+ effects)

// MARK: - Bounce Effect

struct BounceEffect: ViewModifier {
    let trigger: Bool
    @State private var scale: CGFloat = 1.0
    
    func body(content: Content) -> some View {
        content
            .scaleEffect(scale)
            .onChange(of: trigger) { _ in
                withAnimation(.interpolatingSpring(stiffness: 500, damping: 8)) {
                    scale = 1.15
                }
                withAnimation(.interpolatingSpring(stiffness: 500, damping: 8).delay(0.1)) {
                    scale = 1.0
                }
            }
    }
}

extension View {
    func bounceEffect(trigger: Bool) -> some View {
        modifier(BounceEffect(trigger: trigger))
    }
}

// MARK: - Shake Effect

struct ShakeEffect: GeometryEffect {
    var amount: CGFloat = 6
    var shakesPerUnit: Int = 3
    var animatableData: CGFloat

    func effectValue(size: CGSize) -> ProjectionTransform {
        ProjectionTransform(CGAffineTransform(translationX:
            amount * sin(animatableData * .pi * CGFloat(shakesPerUnit)), y: 0))
    }
}

extension View {
    func shakeEffect(trigger: Bool) -> some View {
        modifier(ShakeModifier(trigger: trigger))
    }
}

struct ShakeModifier: ViewModifier {
    let trigger: Bool
    @State private var shakeAmount: CGFloat = 0
    
    func body(content: Content) -> some View {
        content
            .modifier(ShakeEffect(animatableData: shakeAmount))
            .onChange(of: trigger) { _ in
                withAnimation(.linear(duration: 0.4)) {
                    shakeAmount = trigger ? 2 : 0
                }
            }
    }
}

// MARK: - Pulse Effect

struct PulseEffect: ViewModifier {
    let isActive: Bool
    @State private var isPulsing = false
    
    func body(content: Content) -> some View {
        content
            .opacity(isPulsing ? 0.6 : 1.0)
            .scaleEffect(isPulsing ? 1.05 : 1.0)
            .onAppear {
                if isActive {
                    withAnimation(.easeInOut(duration: 0.8).repeatForever(autoreverses: true)) {
                        isPulsing = true
                    }
                }
            }
            .onChange(of: isActive) { active in
                if active {
                    withAnimation(.easeInOut(duration: 0.8).repeatForever(autoreverses: true)) {
                        isPulsing = true
                    }
                } else {
                    withAnimation(.easeOut(duration: 0.2)) {
                        isPulsing = false
                    }
                }
            }
    }
}

extension View {
    func pulseEffect(isActive: Bool) -> some View {
        modifier(PulseEffect(isActive: isActive))
    }
}

// MARK: - Glow Pulse Effect

struct GlowPulseModifier: ViewModifier {
    let isActive: Bool
    let color: Color
    @State private var intensity: CGFloat = 0
    
    func body(content: Content) -> some View {
        content
            .shadow(color: color.opacity(intensity * 0.5), radius: 8 + intensity * 8, x: 0, y: 0)
            .onAppear {
                if isActive { startPulsing() }
            }
            .onChange(of: isActive) { active in
                if active { startPulsing() }
                else {
                    withAnimation(.easeOut(duration: 0.2)) { intensity = 0 }
                }
            }
    }
    
    private func startPulsing() {
        withAnimation(.easeInOut(duration: 1.0).repeatForever(autoreverses: true)) {
            intensity = 1
        }
    }
}

extension View {
    func glowPulse(isActive: Bool, color: Color = DesignSystem.Colors.accentPrimary) -> some View {
        modifier(GlowPulseModifier(isActive: isActive, color: color))
    }
}

// MARK: - Typewriter Effect

struct TypewriterText: View {
    let text: String
    let speed: Double
    @State private var displayedText = ""
    @State private var currentIndex = 0
    
    init(_ text: String, speed: Double = 0.05) {
        self.text = text
        self.speed = speed
    }
    
    var body: some View {
        Text(displayedText)
            .onAppear {
                typeNextCharacter()
            }
    }
    
    private func typeNextCharacter() {
        guard currentIndex < text.count else { return }
        
        DispatchQueue.main.asyncAfter(deadline: .now() + speed) {
            let index = text.index(text.startIndex, offsetBy: currentIndex)
            displayedText += String(text[index])
            currentIndex += 1
            typeNextCharacter()
        }
    }
}

// MARK: - Fade In Stagger

struct FadeInStaggerModifier: ViewModifier {
    let delay: Double
    @State private var isVisible = false
    
    func body(content: Content) -> some View {
        content
            .opacity(isVisible ? 1 : 0)
            .offset(y: isVisible ? 0 : 10)
            .onAppear {
                withAnimation(DesignSystem.Animation.spring.delay(delay)) {
                    isVisible = true
                }
            }
    }
}

extension View {
    func fadeInStagger(delay: Double = 0) -> some View {
        modifier(FadeInStaggerModifier(delay: delay))
    }
}

// MARK: - Ripple Effect (for buttons)

struct RippleEffect: ViewModifier {
    let trigger: Bool
    @State private var rippleOpacity: Double = 0
    @State private var rippleScale: CGFloat = 0.5
    
    func body(content: Content) -> some View {
        content
            .overlay(
                Circle()
                    .fill(Color.white.opacity(0.3))
                    .scaleEffect(rippleScale)
                    .opacity(rippleOpacity)
            )
            .clipped()
            .onChange(of: trigger) { _ in
                withAnimation(.easeOut(duration: 0.3)) {
                    rippleOpacity = 1
                    rippleScale = 2.0
                }
                withAnimation(.easeOut(duration: 0.3).delay(0.2)) {
                    rippleOpacity = 0
                }
                DispatchQueue.main.asyncAfter(deadline: .now() + 0.5) {
                    rippleScale = 0.5
                }
            }
    }
}

extension View {
    func rippleEffect(trigger: Bool) -> some View {
        modifier(RippleEffect(trigger: trigger))
    }
}

// MARK: - Floating Effect

struct FloatingEffect: ViewModifier {
    @State private var offset: CGFloat = 0
    
    func body(content: Content) -> some View {
        content
            .offset(y: offset)
            .onAppear {
                withAnimation(.easeInOut(duration: 2.0).repeatForever(autoreverses: true)) {
                    offset = -8
                }
            }
    }
}

extension View {
    func floatingAnimation() -> some View {
        modifier(FloatingEffect())
    }
}

// MARK: - Rotating Effect

struct RotatingEffect: ViewModifier {
    let isAnimating: Bool
    let duration: Double
    @State private var rotation: Double = 0
    
    func body(content: Content) -> some View {
        content
            .rotationEffect(.degrees(rotation))
            .onChange(of: isAnimating) { animate in
                if animate {
                    withAnimation(.linear(duration: duration).repeatForever(autoreverses: false)) {
                        rotation = 360
                    }
                } else {
                    withAnimation(.easeOut(duration: 0.3)) {
                        rotation = 0
                    }
                }
            }
    }
}

extension View {
    func rotatingAnimation(isAnimating: Bool, duration: Double = 1.0) -> some View {
        modifier(RotatingEffect(isAnimating: isAnimating, duration: duration))
    }
}

// MARK: - Count Up Animation

struct AnimatedNumber: View {
    let value: Int
    let duration: Double
    
    @State private var displayValue: Int = 0
    
    var body: some View {
        Text("\(displayValue)")
            .onAppear { animateValue() }
            .onChange(of: value) { _ in animateValue() }
    }
    
    private func animateValue() {
        let steps = 30
        let stepDuration = duration / Double(steps)
        let difference = value - displayValue
        let stepIncrement = Double(difference) / Double(steps)
        
        for i in 0..<steps {
            DispatchQueue.main.asyncAfter(deadline: .now() + stepDuration * Double(i)) {
                if i == steps - 1 {
                    displayValue = value
                } else {
                    displayValue = displayValue + Int(stepIncrement)
                }
            }
        }
    }
}

// MARK: - Confetti Effect

struct ConfettiPiece: View {
    let color: Color
    @State private var position: CGPoint = .zero
    @State private var rotation: Double = 0
    @State private var opacity: Double = 1
    
    var body: some View {
        Rectangle()
            .fill(color)
            .frame(width: 8, height: 8)
            .rotationEffect(.degrees(rotation))
            .position(position)
            .opacity(opacity)
    }
}

// MARK: - Morphing Shape

struct MorphingCircle: View {
    @State private var morphAmount: CGFloat = 0
    
    var body: some View {
        Circle()
            .fill(DesignSystem.Colors.accentPrimary)
            .modifier(MorphModifier(amount: morphAmount))
            .onAppear {
                withAnimation(.easeInOut(duration: 2).repeatForever(autoreverses: true)) {
                    morphAmount = 1
                }
            }
    }
    
    struct MorphModifier: ViewModifier {
        let amount: CGFloat
        
        func body(content: Content) -> some View {
            content
                .scaleEffect(x: 1 + amount * 0.2, y: 1 - amount * 0.1)
        }
    }
}

// MARK: - Keyboard Shortcuts Manager

class KeyboardShortcutsManager: ObservableObject {
    static let shared = KeyboardShortcutsManager()
    
    struct Shortcut: Identifiable {
        let id = UUID()
        let name: String
        let keys: [String]
        let category: String
    }
    
    @Published var shortcuts: [Shortcut] = [
        // File
        Shortcut(name: "New Scene", keys: ["", "N"], category: "File"),
        Shortcut(name: "Open Project", keys: ["", "O"], category: "File"),
        Shortcut(name: "Save", keys: ["", "S"], category: "File"),
        Shortcut(name: "Save As...", keys: ["", "", "S"], category: "File"),
        Shortcut(name: "Build Project", keys: ["", "B"], category: "File"),
        Shortcut(name: "Build and Run", keys: ["", "R"], category: "File"),
        
        // Edit
        Shortcut(name: "Undo", keys: ["", "Z"], category: "Edit"),
        Shortcut(name: "Redo", keys: ["", "", "Z"], category: "Edit"),
        Shortcut(name: "Cut", keys: ["", "X"], category: "Edit"),
        Shortcut(name: "Copy", keys: ["", "C"], category: "Edit"),
        Shortcut(name: "Paste", keys: ["", "V"], category: "Edit"),
        Shortcut(name: "Duplicate", keys: ["", "D"], category: "Edit"),
        Shortcut(name: "Delete", keys: [""], category: "Edit"),
        Shortcut(name: "Select All", keys: ["", "A"], category: "Edit"),
        
        // View
        Shortcut(name: "Toggle Wireframe", keys: ["Z"], category: "View"),
        Shortcut(name: "Toggle Grid", keys: ["G"], category: "View"),
        Shortcut(name: "Focus Selection", keys: ["F"], category: "View"),
        Shortcut(name: "Frame All", keys: ["A"], category: "View"),
        Shortcut(name: "Toggle Orthographic", keys: ["5"], category: "View"),
        Shortcut(name: "Top View", keys: ["7"], category: "View"),
        Shortcut(name: "Front View", keys: ["1"], category: "View"),
        Shortcut(name: "Right View", keys: ["3"], category: "View"),
        
        // Transform
        Shortcut(name: "Translate", keys: ["W"], category: "Transform"),
        Shortcut(name: "Rotate", keys: ["E"], category: "Transform"),
        Shortcut(name: "Scale", keys: ["R"], category: "Transform"),
        Shortcut(name: "Toggle Local/World", keys: ["X"], category: "Transform"),
        Shortcut(name: "Toggle Snap", keys: [""], category: "Transform"),
        
        // Object
        Shortcut(name: "Create Empty", keys: ["", "", "N"], category: "Object"),
        Shortcut(name: "Group Selection", keys: ["", "G"], category: "Object"),
        Shortcut(name: "Ungroup", keys: ["", "", "G"], category: "Object"),
        Shortcut(name: "Make Prefab", keys: ["", "P"], category: "Object"),
        
        // Panels
        Shortcut(name: "Toggle Inspector", keys: ["", "I"], category: "Panels"),
        Shortcut(name: "Toggle Hierarchy", keys: ["", "H"], category: "Panels"),
        Shortcut(name: "Toggle Console", keys: ["", "`"], category: "Panels"),
        Shortcut(name: "Command Palette", keys: ["", "", "P"], category: "Panels"),
        Shortcut(name: "Quick Search", keys: ["", "K"], category: "Panels"),
        
        // Playback
        Shortcut(name: "Play/Pause", keys: ["", ""], category: "Playback"),
        Shortcut(name: "Stop", keys: ["", "."], category: "Playback"),
        Shortcut(name: "Step Frame", keys: ["", "'"], category: "Playback"),
    ]
    
    var categories: [String] {
        Array(Set(shortcuts.map { $0.category })).sorted()
    }
    
    func shortcuts(for category: String) -> [Shortcut] {
        shortcuts.filter { $0.category == category }
    }
}

// MARK: - Keyboard Shortcuts Panel

struct KeyboardShortcutsPanel: View {
    @ObservedObject var manager = KeyboardShortcutsManager.shared
    @State private var searchText = ""
    @State private var selectedCategory: String?
    
    var filteredShortcuts: [KeyboardShortcutsManager.Shortcut] {
        if searchText.isEmpty {
            if let category = selectedCategory {
                return manager.shortcuts(for: category)
            }
            return manager.shortcuts
        }
        return manager.shortcuts.filter { $0.name.localizedCaseInsensitiveContains(searchText) }
    }
    
    var body: some View {
        VStack(spacing: 0) {
            // Header
            HStack {
                Text("Keyboard Shortcuts")
                    .font(DesignSystem.Typography.headline)
                    .foregroundColor(DesignSystem.Colors.textPrimary)
                
                Spacer()
                
                EditorKeyboardShortcut(keys: ["", "?"])
            }
            .padding(DesignSystem.Spacing.md)
            .background(DesignSystem.Colors.backgroundSecondary)
            
            EditorDivider()
            
            // Search
            EditorSearchBar(text: $searchText, placeholder: "Search shortcuts...")
                .padding(DesignSystem.Spacing.sm)
            
            // Categories
            ScrollView(.horizontal, showsIndicators: false) {
                HStack(spacing: DesignSystem.Spacing.xs) {
                    CategoryPill(title: "All", isSelected: selectedCategory == nil) {
                        selectedCategory = nil
                    }
                    
                    ForEach(manager.categories, id: \.self) { category in
                        CategoryPill(title: category, isSelected: selectedCategory == category) {
                            selectedCategory = category
                        }
                    }
                }
                .padding(.horizontal, DesignSystem.Spacing.sm)
            }
            .padding(.vertical, DesignSystem.Spacing.xs)
            
            EditorDivider()
            
            // Shortcuts list
            ScrollView {
                LazyVStack(spacing: 0) {
                    ForEach(filteredShortcuts) { shortcut in
                        ShortcutRow(shortcut: shortcut)
                    }
                }
            }
        }
        .background(DesignSystem.Colors.backgroundPrimary)
        .frame(width: 400, height: 500)
        .cornerRadius(DesignSystem.CornerRadius.large)
        .shadow(color: .black.opacity(0.3), radius: 20, x: 0, y: 10)
    }
    
    struct CategoryPill: View {
        let title: String
        let isSelected: Bool
        let action: () -> Void
        
        var body: some View {
            Button(action: action) {
                Text(title)
                    .font(DesignSystem.Typography.small)
                    .foregroundColor(isSelected ? .white : DesignSystem.Colors.textSecondary)
                    .padding(.horizontal, DesignSystem.Spacing.sm)
                    .padding(.vertical, DesignSystem.Spacing.xxs)
                    .background(isSelected ? DesignSystem.Colors.accentPrimary : DesignSystem.Colors.backgroundTertiary)
                    .cornerRadius(DesignSystem.CornerRadius.pill)
            }
            .buttonStyle(.plain)
        }
    }
    
    struct ShortcutRow: View {
        let shortcut: KeyboardShortcutsManager.Shortcut
        @State private var isHovered = false
        
        var body: some View {
            HStack {
                Text(shortcut.name)
                    .font(DesignSystem.Typography.body)
                    .foregroundColor(DesignSystem.Colors.textPrimary)
                
                Spacer()
                
                EditorKeyboardShortcut(keys: shortcut.keys)
            }
            .padding(.horizontal, DesignSystem.Spacing.md)
            .padding(.vertical, DesignSystem.Spacing.sm)
            .background(isHovered ? DesignSystem.Colors.hover : Color.clear)
            .onHover { isHovered = $0 }
        }
    }
}

// MARK: - Notification Toast Container

class ToastManager: ObservableObject {
    static let shared = ToastManager()
    
    @Published var toasts: [ToastItem] = []
    
    struct ToastItem: Identifiable {
        let id = UUID()
        let message: String
        let type: EditorToast.ToastType
        var action: (() -> Void)?
        var actionLabel: String?
    }
    
    func show(_ message: String, type: EditorToast.ToastType = .info, action: (() -> Void)? = nil, actionLabel: String? = nil) {
        let toast = ToastItem(message: message, type: type, action: action, actionLabel: actionLabel)
        toasts.append(toast)
        
        DispatchQueue.main.asyncAfter(deadline: .now() + DesignSystem.Duration.autoHide) {
            self.dismiss(toast.id)
        }
    }
    
    func dismiss(_ id: UUID) {
        toasts.removeAll { $0.id == id }
    }
}

struct ToastContainer: View {
    @ObservedObject var manager = ToastManager.shared
    
    var body: some View {
        VStack(spacing: DesignSystem.Spacing.sm) {
            ForEach(manager.toasts) { toast in
                EditorToast(
                    message: toast.message,
                    type: toast.type,
                    action: toast.action,
                    actionLabel: toast.actionLabel ?? "Undo"
                )
                .transition(.move(edge: .trailing).combined(with: .opacity))
            }
        }
        .padding(DesignSystem.Spacing.md)
        .animation(DesignSystem.Animation.spring, value: manager.toasts.count)
    }
}

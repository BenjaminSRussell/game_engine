import SwiftUI
// MARK: - Property Editor Base Protocol (TODO-1461)
protocol PropertyEditor: View {
    associatedtype Value
    var value: Binding<Value> { get }
    var label: String { get }
    var isEnabled: Bool { get }
    var isVisible: Bool { get }
    
    // Enhanced protocol with additional capabilities
    var onValueChange: ((Value) -> Void)? { get }
    var onBeginEdit: (() -> Void)? { get }
    var onEndEdit: (() -> Void)? { get }
    var onValidateValue: ((Value) -> Bool)? { get }
    var onLinkProperty: (() -> Void)? { get }
    
    // Animation support
    var isAnimated: Bool { get }
    var animationCurve: AnimationCurve? { get }
    
    // Multi-value support
    var supportsMultipleValues: Bool { get }
    var multipleValues: [Value]? { get set }
    
    // Context menu support
    var contextMenuItems: [ContextMenuItem]? { get }
    
    // Validation
    func validate(_ value: Value) -> ValidationResult
    
    // Clipboard support
    func copyValue() -> String
    func pasteValue(_ stringValue: String) -> Bool
    
    // Reset support
    func resetToDefault()
    
    // Optional configuration
    var defaultValue: Value? { get }
    var isLocked: Bool { get }
    var isOverridden: Bool { get }
    var tooltip: String? { get }
    var unit: String? { get }
    var propertyType: PropertyType { get }
    
    // Optional callbacks
    var onValueChanged: ((Value) -> Void)? { get }
    var onReset: (() -> Void)? { get }
    var onLockToggle: ((Bool) -> Void)? { get }
    var onAnimationKeyframe: (() -> Void)? { get }
    var onExpressionInput: (() -> Void)? { get }
}


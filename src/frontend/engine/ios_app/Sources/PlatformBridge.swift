import Foundation
import UIKit
import CoreHaptics

/// iOS platform bridge for native platform features
class PlatformBridge {
    
    static let shared = PlatformBridge()
    
    // MARK: - Types
    
    enum HapticStyle {
        case light
        case medium
        case heavy
        case selection
        case success
        case warning
        case error
    }
    
    struct DeviceInfo {
        let model: String
        let osVersion: String
        let screenScale: CGFloat
        let hasNotch: Bool
        let safeAreaInsets: UIEdgeInsets
        let screenSize: CGSize
    }
    
    struct TouchEvent {
        let touchID: Int
        let position: CGPoint
        let phase: UITouch.Phase
        let timestamp: TimeInterval
        let force: CGFloat
    }
    
    // MARK: - Properties
    
    private var hapticEngine: CHHapticEngine?
    private var activeTouches: [UITouch: Int] = [:]
    private var nextTouchID = 0
    
    // MARK: - Initialization
    
    private init() {
        setupHaptics()
    }
    
    // MARK: - Device Info
    
    func getDeviceInfo() -> DeviceInfo {
        let model = UIDevice.current.model
        let osVersion = UIDevice.current.systemVersion
        let screenScale = UIScreen.main.scale
        let screenSize = UIScreen.main.bounds.size
        
        // Detect notch by checking safe area
        let window = UIApplication.shared.windows.first
        let safeArea = window?.safeAreaInsets ?? .zero
        let hasNotch = safeArea.top > 20 // iPhone X+ have top inset > 20
        
        return DeviceInfo(
            model: model,
            osVersion: osVersion,
            screenScale: screenScale,
            hasNotch: hasNotch,
            safeAreaInsets: safeArea,
            screenSize: screenSize
        )
    }
    
    func getSafeAreaInsets() -> UIEdgeInsets {
        return UIApplication.shared.windows.first?.safeAreaInsets ?? .zero
    }
    
    // MARK: - Touch Input
    
    func processTouches(_ touches: Set<UITouch>, with event: UIEvent?) -> [TouchEvent] {
        var events: [TouchEvent] = []
        
        for touch in touches {
            let touchID = getTouchID(for: touch)
            let position = touch.location(in: touch.view)
            let phase = touch.phase
            let timestamp = event?.timestamp ?? Date().timeIntervalSince1970
            let force = touch.force
            
            let touchEvent = TouchEvent(
                touchID: touchID,
                position: position,
                phase: phase,
                timestamp: timestamp,
                force: force
            )
            
            events.append(touchEvent)
            
            // Clean up ended/cancelled touches
            if phase == .ended || phase == .cancelled {
                activeTouches.removeValue(forKey: touch)
            }
        }
        
        return events
    }
    
    private func getTouchID(for touch: UITouch) -> Int {
        if let id = activeTouches[touch] {
            return id
        } else {
            let id = nextTouchID
            nextTouchID += 1
            activeTouches[touch] = id
            return id
        }
    }
    
    // MARK: - Haptic Feedback
    
    private func setupHaptics() {
        guard CHHapticEngine.capabilitiesForHardware().supportsHaptics else {
            print("Device does not support haptics")
            return
        }
        
        do {
            hapticEngine = try CHHapticEngine()
            try hapticEngine?.start()
        } catch {
            print("Failed to start haptic engine: \(error)")
        }
    }
    
    func triggerHaptic(style: HapticStyle) {
        switch style {
        case .light, .medium, .heavy:
            triggerImpactFeedback(style: style)
        case .selection:
            triggerSelectionFeedback()
        case .success:
            triggerNotificationFeedback(type: .success)
        case .warning:
            triggerNotificationFeedback(type: .warning)
        case .error:
            triggerNotificationFeedback(type: .error)
        }
    }
    
    private func triggerImpactFeedback(style: HapticStyle) {
        let impactStyle: UIImpactFeedbackGenerator.FeedbackStyle
        switch style {
        case .light: impactStyle = .light
        case .medium: impactStyle = .medium
        case .heavy: impactStyle = .heavy
        default: impactStyle = .medium
        }
        
        let generator = UIImpactFeedbackGenerator(style: impactStyle)
        generator.prepare()
        generator.impactOccurred()
    }
    
    private func triggerSelectionFeedback() {
        let generator = UISelectionFeedbackGenerator()
        generator.prepare()
        generator.selectionChanged()
    }
    
    private func triggerNotificationFeedback(type: UINotificationFeedbackGenerator.FeedbackType) {
        let generator = UINotificationFeedbackGenerator()
        generator.prepare()
        generator.notificationOccurred(type)
    }
    
    // MARK: - Lifecycle Management
    
    func onAppSuspend() {
        // Save game state
        print("App suspending - saving state")
        
        // Pause haptic engine
        hapticEngine?.stop(completionHandler: { error in
            if let error = error {
                print("Error stopping haptic engine: \(error)")
            }
        })
    }
    
    func onAppResume() {
        // Restore game state
        print("App resuming - restoring state")
        
        // Restart haptic engine
        do {
            try hapticEngine?.start()
        } catch {
            print("Error restarting haptic engine: \(error)")
        }
    }
    
    func onEnterForeground() {
        print("App entered foreground")
        // Resume audio
        // Resume rendering
    }
    
    func onEnterBackground() {
        print("App entered background")
        // Pause audio
        // Pause rendering
        // Save state
    }
    
    // MARK: - Metal Context
    
    func getMetalDevice() -> MTLDevice? {
        return MTLCreateSystemDefaultDevice()
    }
    
    // MARK: - Orientation
    
    func lockOrientation(_ orientation: UIInterfaceOrientationMask) {
        // This would be handled by the app delegate
        // For now, just log
        print("Locking orientation to: \(orientation)")
    }
    
    // MARK: - Screen Brightness
    
    func getScreenBrightness() -> CGFloat {
        return UIScreen.main.brightness
    }
    
    func setScreenBrightness(_ brightness: CGFloat) {
        UIScreen.main.brightness = brightness
    }
}

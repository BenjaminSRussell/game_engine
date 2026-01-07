import Foundation

/// Analytics Manager replacing the deleted Next.js analytics integration.
/// Provides a unified interface for tracking events in the editor.
class AnalyticsManager: ObservableObject {
    
    static let shared = AnalyticsManager()
    
    @Published var eventCount: Int = 0
    @Published var isTracking: Bool = true
    
    private init() {}
    
    func track(event: String, parameters: [String: Any] = [:]) {
        guard isTracking else { return }
        
        // In a real implementation, this would send data to a backend (e.g. TelemetryDeck, Firebase)
        print("[Analytics] Event: \(event), Params: \(parameters)")
        
        DispatchQueue.main.async {
            self.eventCount += 1
        }
    }
    
    func toggleTracking() {
        isTracking.toggle()
    }
}

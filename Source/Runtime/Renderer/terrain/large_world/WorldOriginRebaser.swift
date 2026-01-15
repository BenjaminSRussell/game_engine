import Foundation
import simd

// MARK: - World Origin Rebasing System

/// World origin rebasing system provides dynamic origin rebasing for large world support
@MainActor
class WorldOriginRebaser: ObservableObject {
    static let shared = WorldOriginRebaser()
    
    @Published var currentOrigin: SIMD3<Double> = .zero
    @Published var rebasingInProgress: Bool = false
    @Published var rebaseHistory: [RebaseEvent] = []
    @Published var autoRebaseEnabled: Bool = true
    @Published var rebaseThreshold: Double = 50_000.0
    @Published var rebasingMode: RebasingMode = .cameraBased
    @Published var smoothRebasing: Bool = true
    @Published var rebaseInterpolationDuration: Double = 2.0
    
    private var lastRebaseTime: Date = Date()
    private var interpolationStartTime: Date?
    private var interpolationStartOrigin: SIMD3<Double> = .zero
    private var interpolationTargetOrigin: SIMD3<Double> = .zero
    private var rebasableSystems: [RebasableSystem] = []
    private var rebaseTimer: Timer?
    
    struct RebaseEvent: Codable, Identifiable {
        let id: UUID
        let timestamp: Date
        let oldOrigin: SIMD3<Double>
        let newOrigin: SIMD3<Double>
        let delta: SIMD3<Double>
        let reason: RebaseReason
        let duration: Double
        
        enum RebaseReason: String, Codable, CaseIterable {
            case cameraDistance = "camera_distance"
            case worldBoundary = "world_boundary"
            case manual = "manual"
            case performance = "performance"
            case precision = "precision"
            case chunkLoad = "chunk_load"
        }
    }
    
    enum RebasingMode {
        case cameraBased
        case chunkBased
        case performanceBased
        case manual
        case hybrid
    }
    
    protocol RebasableSystem {
        var systemID: String { get }
        func prepareForRebase(oldOrigin: SIMD3<Double>, newOrigin: SIMD3<Double>, delta: SIMD3<Double>)
        func executeRebase(oldOrigin: SIMD3<Double>, newOrigin: SIMD3<Double>, delta: SIMD3<Double>)
        func finalizeRebase(oldOrigin: SIMD3<Double>, newOrigin: SIMD3<Double>, delta: SIMD3<Double>)
    }
    
    private init() {
        setupRebaseTimer()
    }
    
    deinit {
        rebaseTimer?.invalidate()
    }
    
    // MARK: - System Registration
    
    /// Register a system that needs rebasing notifications
    func registerSystem(_ system: RebasableSystem) {
        rebasableSystems.append(system)
        print("Registered rebasable system: \(system.systemID)")
    }
    
    /// Unregister a rebasable system
    func unregisterSystem(_ systemID: String) {
        rebasableSystems.removeAll { $0.systemID == systemID }
        print("Unregistered rebasable system: \(systemID)")
    }
    
    // MARK: - Manual Rebasing
    
    /// Manually trigger a rebase to specific origin
    func rebaseToOrigin(_ newOrigin: SIMD3<Double>, reason: RebaseEvent.RebaseReason = .manual) {
        guard !rebasingInProgress else {
            print("Rebase already in progress, ignoring request")
            return
        }
        
        let startTime = Date()
        let oldOrigin = currentOrigin
        let delta = newOrigin - oldOrigin
        
        print("Starting rebase from \(oldOrigin) to \(newOrigin) (reason: \(reason.rawValue))")
        
        if smoothRebasing {
            startSmoothRebase(from: oldOrigin, to: newOrigin, reason: reason)
        } else {
            performImmediateRebase(from: oldOrigin, to: newOrigin, delta: delta, reason: reason)
        }
        
        let duration = Date().timeIntervalSince(startTime)
        recordRebaseEvent(oldOrigin: oldOrigin, newOrigin: newOrigin, delta: delta, reason: reason, duration: duration)
    }
    
    /// Rebase to center of loaded chunks
    func rebaseToChunkCenter() {
        let largeWorld = LargeWorldCoordinates.shared
        let centerOrigin = largeWorld.getRecommendedOrigin(for: largeWorld.lastCameraPosition)
        rebaseToOrigin(centerOrigin, reason: .chunkLoad)
    }
    
    /// Rebase to optimal origin for performance
    func rebaseForPerformance() {
        // Calculate optimal origin based on loaded objects and camera
        let optimalOrigin = calculateOptimalOrigin()
        rebaseToOrigin(optimalOrigin, reason: .performance)
    }
    
    // MARK: - Automatic Rebasing
    
    /// Check if rebase is needed based on current conditions
    func checkRebaseNeeded(for cameraPosition: SIMD3<Double>) -> Bool {
        guard autoRebaseEnabled && !rebasingInProgress else { return false }
        
        switch rebasingMode {
        case .cameraBased:
            return checkCameraBasedRebase(cameraPosition)
        case .chunkBased:
            return checkChunkBasedRebase()
        case .performanceBased:
            return checkPerformanceBasedRebase()
        case .manual:
            return false
        case .hybrid:
            return checkCameraBasedRebase(cameraPosition) || 
                   checkChunkBasedRebase() || 
                   checkPerformanceBasedRebase()
        }
    }
    
    private func checkCameraBasedRebase(_ cameraPosition: SIMD3<Double>) -> Bool {
        let distanceFromOrigin = length(cameraPosition - currentOrigin)
        return distanceFromOrigin > rebaseThreshold
    }
    
    private func checkChunkBasedRebase() -> Bool {
        let largeWorld = LargeWorldCoordinates.shared
        let currentChunk = largeWorld.currentChunk
        let recommendedChunk = largeWorld.worldToChunk(largeWorld.lastCameraPosition)
        
        return currentChunk.distance(to: recommendedChunk) > 2
    }
    
    private func checkPerformanceBasedRebase() -> Bool {
        // Check if we have too many objects far from origin
        let distantObjectCount = countDistantObjects()
        return distantObjectCount > 1000 // Threshold for performance-based rebase
    }
    
    private func countDistantObjects() -> Int {
        // This would integrate with the rendering system to count objects
        // For now, return a mock value
        return 0
    }
    
    // MARK: - Smooth Rebasing
    
    private func startSmoothRebase(from oldOrigin: SIMD3<Double>, to newOrigin: SIMD3<Double>, reason: RebaseEvent.RebaseReason) {
        rebasingInProgress = true
        interpolationStartTime = Date()
        interpolationStartOrigin = oldOrigin
        interpolationTargetOrigin = newOrigin
        
        // Start smooth rebase timer
        Timer.scheduledTimer(withTimeInterval: 1.0/60.0, repeats: true) { [weak self] timer in
            self?.updateSmoothRebase(timer: timer, reason: reason)
        }
    }
    
    private func updateSmoothRebase(timer: Timer, reason: RebaseEvent.RebaseReason) {
        guard let startTime = interpolationStartTime else {
            timer.invalidate()
            return
        }
        
        let elapsed = Date().timeIntervalSince(startTime)
        let progress = min(elapsed / rebaseInterpolationDuration, 1.0)
        
        // Use easing function for smooth interpolation
        let easedProgress = easeInOutCubic(progress)
        
        let newOrigin = interpolate(
            from: interpolationStartOrigin,
            to: interpolationTargetOrigin,
            progress: easedProgress
        )
        
        // Update current origin
        let delta = newOrigin - currentOrigin
        currentOrigin = newOrigin
        
        // Notify systems of incremental change
        notifyIncrementalRebase(delta: delta, progress: progress)
        
        if progress >= 1.0 {
            timer.invalidate()
            finalizeSmoothRebase(reason: reason)
        }
    }
    
    private func finalizeSmoothRebase(reason: RebaseEvent.RebaseReason) {
        let oldOrigin = interpolationStartOrigin
        let newOrigin = interpolationTargetOrigin
        let delta = newOrigin - oldOrigin
        
        // Finalize rebase for all systems
        for system in rebasableSystems {
            system.finalizeRebase(oldOrigin: oldOrigin, newOrigin: newOrigin, delta: delta)
        }
        
        rebasingInProgress = false
        interpolationStartTime = nil
        
        print("Completed smooth rebase to \(newOrigin)")
    }
    
    private func performImmediateRebase(from oldOrigin: SIMD3<Double>, to newOrigin: SIMD3<Double>, delta: SIMD3<Double>, reason: RebaseEvent.RebaseReason) {
        rebasingInProgress = true
        let startTime = Date()
        
        // Phase 1: Prepare systems for rebase
        for system in rebasableSystems {
            system.prepareForRebase(oldOrigin: oldOrigin, newOrigin: newOrigin, delta: delta)
        }
        
        // Phase 2: Execute rebase
        currentOrigin = newOrigin
        for system in rebasableSystems {
            system.executeRebase(oldOrigin: oldOrigin, newOrigin: newOrigin, delta: delta)
        }
        
        // Phase 3: Finalize rebase
        for system in rebasableSystems {
            system.finalizeRebase(oldOrigin: oldOrigin, newOrigin: newOrigin, delta: delta)
        }
        
        let duration = Date().timeIntervalSince(startTime)
        rebasingInProgress = false
        
        print("Completed immediate rebase in \(duration)s")
    }
    
    // MARK: - Utility Functions
    
    private func interpolate(from start: SIMD3<Double>, to end: SIMD3<Double>, progress: Double) -> SIMD3<Double> {
        return start + (end - start) * progress
    }
    
    private func easeInOutCubic(_ t: Double) -> Double {
        return t < 0.5 ? 4 * t * t * t : 1 - pow(-2 * t + 2, 3) / 2
    }
    
    private func calculateOptimalOrigin() -> SIMD3<Double> {
        // This would calculate the optimal origin based on:
        // 1. Camera position
        // 2. Distribution of loaded objects
        // 3. Performance metrics
        // For now, return camera-based origin
        
        let largeWorld = LargeWorldCoordinates.shared
        return largeWorld.getRecommendedOrigin(for: largeWorld.lastCameraPosition)
    }
    
    private func notifyIncrementalRebase(delta: SIMD3<Double>, progress: Double) {
        NotificationCenter.default.post(
            name: .originRebaseProgress,
            object: nil,
            userInfo: [
                "delta": delta,
                "progress": progress,
                "currentOrigin": currentOrigin
            ]
        )
    }
    
    private func recordRebaseEvent(oldOrigin: SIMD3<Double>, newOrigin: SIMD3<Double>, delta: SIMD3<Double>, reason: RebaseEvent.RebaseReason, duration: Double) {
        let event = RebaseEvent(
            id: UUID(),
            timestamp: Date(),
            oldOrigin: oldOrigin,
            newOrigin: newOrigin,
            delta: delta,
            reason: reason,
            duration: duration
        )
        
        rebaseHistory.append(event)
        
        // Keep history size manageable
        if rebaseHistory.count > 100 {
            rebaseHistory.removeFirst()
        }
        
        lastRebaseTime = Date()
        
        // Notify listeners
        NotificationCenter.default.post(
            name: .originRebaseCompleted,
            object: nil,
            userInfo: [
                "event": event,
                "currentOrigin": currentOrigin
            ]
        )
    }
    
    private func setupRebaseTimer() {
        rebaseTimer = Timer.scheduledTimer(withTimeInterval: 1.0, repeats: true) { [weak self] _ in
            self?.periodicRebaseCheck()
        }
    }
    
    private func periodicRebaseCheck() {
        let largeWorld = LargeWorldCoordinates.shared
        let cameraPosition = largeWorld.lastCameraPosition
        
        if checkRebaseNeeded(for: cameraPosition) {
            let newOrigin = largeWorld.getRecommendedOrigin(for: cameraPosition)
            rebaseToOrigin(newOrigin, reason: .cameraDistance)
        }
    }
    
    // MARK: - Configuration
    
    /// Configure rebase settings
    func configureRebasing(
        autoEnabled: Bool,
        threshold: Double,
        mode: RebasingMode,
        smooth: Bool,
        duration: Double
    ) {
        autoRebaseEnabled = autoEnabled
        rebaseThreshold = threshold
        rebasingMode = mode
        smoothRebasing = smooth
        rebaseInterpolationDuration = duration
        
        print("Rebasing configured: auto=\(autoEnabled), threshold=\(threshold), mode=\(mode), smooth=\(smooth)")
    }
    
    /// Get current rebase statistics
    func getRebaseStatistics() -> RebaseStatistics {
        let recentRebases = rebaseHistory.suffix(10)
        let averageInterval = recentRebases.count > 1 ? 
            recentRebases.dropFirst().map { $0.timestamp.timeIntervalSince(recentRebases.first!.timestamp) }.reduce(0, +) / Double(recentRebases.count - 1) : 0
        
        let rebaseReasons = Dictionary(grouping: rebaseHistory) { $0.reason }
        
        return RebaseStatistics(
            totalRebases: rebaseHistory.count,
            recentRebases: recentRebases.count,
            averageInterval: averageInterval,
            lastRebaseTime: lastRebaseTime,
            rebaseReasons: rebaseReasons,
            currentOrigin: currentOrigin,
            autoRebaseEnabled: autoRebaseEnabled,
            rebaseThreshold: rebaseThreshold
        )
    }
    
    struct RebaseStatistics {
        let totalRebases: Int
        let recentRebases: Int
        let averageInterval: Double
        let lastRebaseTime: Date
        let rebaseReasons: [RebaseEvent.RebaseReason: Int]
        let currentOrigin: SIMD3<Double>
        let autoRebaseEnabled: Bool
        let rebaseThreshold: Double
        
        var rebaseFrequency: Double {
            guard recentRebases > 1 else { return 0 }
            let timeSpan = Date().timeIntervalSince(lastRebaseTime)
            return timeSpan > 0 ? Double(recentRebases) / timeSpan : 0
        }
    }
}

// MARK: - Notification Names

extension Notification.Name {
    static let originRebaseCompleted = Notification.Name("originRebaseCompleted")
    static let originRebaseProgress = Notification.Name("originRebaseProgress")
    static let originRebaseStarted = Notification.Name("originRebaseStarted")
}

// MARK: - Example Rebasable Systems

class TerrainRebasableSystem: WorldOriginRebaser.RebasableSystem {
    let systemID = "TerrainSystem"
    
    func prepareForRebase(oldOrigin: SIMD3<Double>, newOrigin: SIMD3<Double>, delta: SIMD3<Double>) {
        // Prepare terrain system for rebase
        print("Terrain system preparing for rebase: delta=\(delta)")
    }
    
    func executeRebase(oldOrigin: SIMD3<Double>, newOrigin: SIMD3<Double>, delta: SIMD3<Double>) {
        // Execute terrain rebase
        print("Terrain system executing rebase")
    }
    
    func finalizeRebase(oldOrigin: SIMD3<Double>, newOrigin: SIMD3<Double>, delta: SIMD3<Double>) {
        // Finalize terrain rebase
        print("Terrain system finalized rebase")
    }
}

class RenderingRebasableSystem: WorldOriginRebaser.RebasableSystem {
    let systemID = "RenderingSystem"
    
    func prepareForRebase(oldOrigin: SIMD3<Double>, newOrigin: SIMD3<Double>, delta: SIMD3<Double>) {
        // Prepare rendering system for rebase
        print("Rendering system preparing for rebase")
    }
    
    func executeRebase(oldOrigin: SIMD3<Double>, newOrigin: SIMD3<Double>, delta: SIMD3<Double>) {
        // Execute rendering rebase
        print("Rendering system executing rebase")
    }
    
    func finalizeRebase(oldOrigin: SIMD3<Double>, newOrigin: SIMD3<Double>, delta: SIMD3<Double>) {
        // Finalize rendering rebase
        print("Rendering system finalized rebase")
    }
}

class PhysicsRebasableSystem: WorldOriginRebaser.RebasableSystem {
    let systemID = "PhysicsSystem"
    
    func prepareForRebase(oldOrigin: SIMD3<Double>, newOrigin: SIMD3<Double>, delta: SIMD3<Double>) {
        // Prepare physics system for rebase
        print("Physics system preparing for rebase")
    }
    
    func executeRebase(oldOrigin: SIMD3<Double>, newOrigin: SIMD3<Double>, delta: SIMD3<Double>) {
        // Execute physics rebase
        print("Physics system executing rebase")
    }
    
    func finalizeRebase(oldOrigin: SIMD3<Double>, newOrigin: SIMD3<Double>, delta: SIMD3<Double>) {
        // Finalize physics rebase
        print("Physics system finalized rebase")
    }
}

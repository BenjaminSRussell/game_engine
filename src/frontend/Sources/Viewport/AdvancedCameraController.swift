import SwiftUI
import simd

// MARK: - Advanced Camera Controller
// Provides professional-grade camera controls for viewport navigation

class AdvancedCameraController: ObservableObject {
    @Published var cameraMode: CameraMode = .orbit
    @Published var cameraSpeed: Float = 1.0
    @Published var fieldOfView: Float = 60.0
    @Published var nearPlane: Float = 0.1
    @Published var farPlane: Float = 1000.0
    
    // Camera bookmarks
    @Published var bookmarks: [CameraBookmark] = []
    
    // Camera path recording
    @Published var isRecordingPath = false
    @Published var recordedPath: [CameraKeyframe] = []
    
    // Camera settings
    @Published var enableInertia = true
    @Published var inertiaDamping: Float = 0.9
    @Published var enableAcceleration = true
    @Published var maxSpeed: Float = 10.0
    
    // Camera shake simulation
    @Published var isShaking = false
    @Published var shakeIntensity: Float = 0.0
    @Published var shakeFrequency: Float = 10.0
    @Published var shakeDuration: TimeInterval = 0.0
    @Published var shakeTime: TimeInterval = 0.0
    
    // Handheld camera simulation
    @Published var handheldEnabled = false
    @Published var handheldIntensity: Float = 0.5
    @Published var handheldSpeed: Float = 1.0
    @Published var handheldTime: TimeInterval = 0.0
    
    // Camera roll
    @Published var cameraRoll: Float = 0.0
    
    // Look-at target
    @Published var lookAtTarget: SIMD3<Float>?
    @Published var lookAtUpVector: SIMD3<Float> = SIMD3(0, 1, 0)
    
    // Frustum visualization
    @Published var showFrustum = false
    @Published var frustumColor: SIMD4<Float> = SIMD4(1.0, 1.0, 0.0, 0.5) // Yellow semi-transparent

    // Follow target
    @Published var followTarget: UUID?
    @Published var followDistance: Float = 5.0
    @Published var followHeight: Float = 2.0

    // 3D cursor
    @Published var cursor3D: SIMD3<Float> = .zero

    // Camera position and rotation
    @Published var cameraPosition: SIMD3<Float> = SIMD3(0, 5, 10)
    @Published var cameraRotation: SIMD3<Float> = SIMD3(-20, 0, 0)
    @Published var orbitDistance: Float = 10.0
    @Published var orbitAngle: SIMD2<Float> = SIMD2(0, 45)

    // Path playback state
    @Published var isPlayingPath = false
    @Published var pathPlaybackTime: TimeInterval = 0
    private var pathPlaybackTimer: Timer?
    private var pathPlaybackStartTime: TimeInterval = 0
    private var pathPlaybackSpeed: Float = 1.0

    // Orbit camera limits
    private let minOrbitDistance: Float = 0.5
    private let maxOrbitDistance: Float = 500.0
    private let minPitch: Float = -85.0
    private let maxPitch: Float = 85.0
    
    // MARK: - Camera Modes
    enum CameraMode: String, CaseIterable, Identifiable {
        case orbit = "Orbit"
        case freeFly = "Free Fly"
        case locked = "Locked to Object"
        case follow = "Follow"
        case firstPerson = "First Person"
        case thirdPerson = "Third Person"
        case topDown = "Top Down"
        case sideView = "Side View"
        case frontView = "Front View"
        case isometric = "Isometric"
        
        var id: String { rawValue }
        
        var icon: String {
            switch self {
            case .orbit: return "arrow.triangle2.circlepath.circle"
            case .freeFly: return "paperplane"
            case .locked: return "lock.circle"
            case .follow: return "person.fill.viewfinder"
            case .firstPerson: return "eye"
            case .thirdPerson: return "figure.walk"
            case .topDown: return "arrow.down.to.line"
            case .sideView: return "arrow.right.to.line"
            case .frontView: return "arrow.forward.to.line"
            case .isometric: return "cube.transparent"
            }
        }
    }
    
    // MARK: - Camera Bookmark
    struct CameraBookmark: Identifiable, Codable {
        let id: UUID
        var name: String
        var position: SIMD3<Float>
        var rotation: SIMD3<Float>
        var fov: Float
        var timestamp: Date

        init(id: UUID = UUID(), name: String, position: SIMD3<Float>, rotation: SIMD3<Float>, fov: Float, timestamp: Date = Date()) {
            self.id = id
            self.name = name
            self.position = position
            self.rotation = rotation
            self.fov = fov
            self.timestamp = timestamp
        }

        enum CodingKeys: String, CodingKey {
            case id, name, positionX, positionY, positionZ
            case rotationX, rotationY, rotationZ, fov, timestamp
        }

        func encode(to encoder: Encoder) throws {
            var container = encoder.container(keyedBy: CodingKeys.self)
            try container.encode(id, forKey: .id)
            try container.encode(name, forKey: .name)
            try container.encode(position.x, forKey: .positionX)
            try container.encode(position.y, forKey: .positionY)
            try container.encode(position.z, forKey: .positionZ)
            try container.encode(rotation.x, forKey: .rotationX)
            try container.encode(rotation.y, forKey: .rotationY)
            try container.encode(rotation.z, forKey: .rotationZ)
            try container.encode(fov, forKey: .fov)
            try container.encode(timestamp, forKey: .timestamp)
        }

        init(from decoder: Decoder) throws {
            let container = try decoder.container(keyedBy: CodingKeys.self)
            id = try container.decode(UUID.self, forKey: .id)
            name = try container.decode(String.self, forKey: .name)
            let posX = try container.decode(Float.self, forKey: .positionX)
            let posY = try container.decode(Float.self, forKey: .positionY)
            let posZ = try container.decode(Float.self, forKey: .positionZ)
            position = SIMD3(posX, posY, posZ)
            let rotX = try container.decode(Float.self, forKey: .rotationX)
            let rotY = try container.decode(Float.self, forKey: .rotationY)
            let rotZ = try container.decode(Float.self, forKey: .rotationZ)
            rotation = SIMD3(rotX, rotY, rotZ)
            fov = try container.decode(Float.self, forKey: .fov)
            timestamp = try container.decode(Date.self, forKey: .timestamp)
        }
    }
    
    // MARK: - Camera Keyframe
    struct CameraKeyframe {
        var timestamp: TimeInterval
        var position: SIMD3<Float>
        var rotation: SIMD3<Float>
        var fov: Float

        enum InterpolationType {
            case linear
            case easeInOut
            case bezier(p1: SIMD3<Float>, p2: SIMD3<Float>)
            case catmullRom
        }

        func interpolate(to other: CameraKeyframe, at t: Float, type: InterpolationType = .linear) -> CameraKeyframe {
            let clampedT = max(0, min(1, t))

            let position = interpolatePosition(to: other, at: clampedT, type: type)
            let rotation = interpolateRotation(to: other, at: clampedT, type: type)
            let fov = interpolateFloat(fov, other.fov, at: clampedT, type: type)

            return CameraKeyframe(timestamp: timestamp + (other.timestamp - timestamp) * Double(clampedT),
                                position: position, rotation: rotation, fov: fov)
        }

        private func interpolatePosition(to other: CameraKeyframe, at t: Float, type: InterpolationType) -> SIMD3<Float> {
            switch type {
            case .linear:
                return position + (other.position - position) * t

            case .easeInOut:
                let easedT = t * t * (3.0 - 2.0 * t) // Smoothstep
                return position + (other.position - position) * easedT

            case .bezier(let p1, let p2):
                // Cubic Bezier curve
                let u = 1.0 - t
                return u*u*u*position +
                       3*u*u*t*p1 +
                       3*u*t*t*p2 +
                       t*t*t*other.position

            case .catmullRom:
                // For catmull-rom we'd need neighboring keyframes, use linear for now
                return position + (other.position - position) * t
            }
        }

        private func interpolateRotation(to other: CameraKeyframe, at t: Float, type: InterpolationType) -> SIMD3<Float> {
            switch type {
            case .easeInOut:
                let easedT = t * t * (3.0 - 2.0 * t)
                return rotation + (other.rotation - rotation) * easedT
            default:
                return rotation + (other.rotation - rotation) * t
            }
        }

        private func interpolateFloat(_ start: Float, _ end: Float, at t: Float, type: InterpolationType) -> Float {
            switch type {
            case .easeInOut:
                let easedT = t * t * (3.0 - 2.0 * t)
                return start + (end - start) * easedT
            default:
                return start + (end - start) * t
            }
        }
    }
    
    // MARK: - Camera Speed Presets
    enum SpeedPreset: String, CaseIterable {
        case slow = "Slow (0.25x)"
        case normal = "Normal (1x)"
        case fast = "Fast (2x)"
        case veryFast = "Very Fast (5x)"
        case ultraFast = "Ultra Fast (10x)"
        
        var multiplier: Float {
            switch self {
            case .slow: return 0.25
            case .normal: return 1.0
            case .fast: return 2.0
            case .veryFast: return 5.0
            case .ultraFast: return 10.0
            }
        }
    }
    
    // MARK: - Functions

    // MARK: Orbital Camera with Zoom Limits
    func updateOrbitCamera(distance: Float, angle: SIMD2<Float>) {
        // Clamp distance to limits
        orbitDistance = max(minOrbitDistance, min(distance, maxOrbitDistance))

        // Clamp pitch (Y rotation) to prevent flipping
        let clampedPitch = max(minPitch, min(angle.x, maxPitch))
        orbitAngle = SIMD2(clampedPitch, angle.y)

        // Convert spherical coordinates to cartesian
        let pitchRad = clampedPitch * .pi / 180.0
        let yawRad = angle.y * .pi / 180.0

        let x = orbitDistance * cos(pitchRad) * sin(yawRad)
        let y = orbitDistance * sin(pitchRad)
        let z = orbitDistance * cos(pitchRad) * cos(yawRad)

        cameraPosition = SIMD3(x, y, z)

        // Calculate look-at direction
        let lookDirection = normalize(-cameraPosition)
        cameraRotation.x = asin(-lookDirection.y) * 180.0 / .pi
        cameraRotation.y = atan2(lookDirection.x, lookDirection.z) * 180.0 / .pi

        print("[Camera] Orbit mode: distance=\(orbitDistance), angle=\(orbitAngle)")
    }

    // MARK: Free-fly Camera
    func updateFreeFlyCameraMovement(forward: Float, right: Float, up: Float) {
        let moveSpeed = cameraSpeed * (enableAcceleration ? 2.0 : 1.0)
        let sensitivity = 0.1 * moveSpeed

        // Calculate movement direction based on camera rotation
        let yawRad = cameraRotation.y * .pi / 180.0
        let pitchRad = cameraRotation.x * .pi / 180.0

        // Forward vector
        let forwardDir = SIMD3(
            sin(yawRad) * cos(pitchRad),
            sin(pitchRad),
            cos(yawRad) * cos(pitchRad)
        )

        // Right vector
        let rightDir = SIMD3(
            cos(yawRad),
            0,
            -sin(yawRad)
        )

        // Up vector
        let upDir = SIMD3(0, 1, 0)

        // Apply movement
        let movement = forwardDir * (forward * sensitivity) +
                      rightDir * (right * sensitivity) +
                      upDir * (up * sensitivity)

        if enableInertia {
            // Smooth movement with inertia
            cameraPosition = cameraPosition + movement * inertiaDamping
        } else {
            cameraPosition = cameraPosition + movement
        }

        print("[Camera] Free fly movement: forward=\(forward), right=\(right), up=\(up)")
    }
    
    // MARK: Bookmark System
    func saveBookmark(name: String, position: SIMD3<Float>, rotation: SIMD3<Float>) {
        let bookmark = CameraBookmark(name: name, position: position, rotation: rotation, fov: fieldOfView)
        bookmarks.append(bookmark)
        saveBookmarksToStorage()
        print("[Camera] Saved bookmark: \(name)")
    }

    func loadBookmark(_ bookmark: CameraBookmark) {
        // Animate camera to bookmark position over 0.5 seconds
        let startPosition = cameraPosition
        let startRotation = cameraRotation
        let startFOV = fieldOfView

        let targetPosition = bookmark.position
        let targetRotation = bookmark.rotation
        let targetFOV = bookmark.fov

        let duration: TimeInterval = 0.5
        let startTime = Date()

        let timer = Timer.scheduledTimer(withTimeInterval: 1.0/60.0, repeats: true) { [weak self] timer in
            guard let self = self else {
                timer.invalidate()
                return
            }

            let elapsed = Date().timeIntervalSince(startTime)
            let progress = min(Float(elapsed / duration), 1.0)
            let easedProgress = progress * progress * (3.0 - 2.0 * progress) // Smoothstep easing

            // Interpolate camera properties
            self.cameraPosition = startPosition + (targetPosition - startPosition) * easedProgress
            self.cameraRotation = startRotation + (targetRotation - startRotation) * easedProgress
            self.fieldOfView = startFOV + (targetFOV - startFOV) * easedProgress

            if progress >= 1.0 {
                timer.invalidate()
                self.cameraPosition = targetPosition
                self.cameraRotation = targetRotation
                self.fieldOfView = targetFOV
            }
        }

        print("[Camera] Loading bookmark: \(bookmark.name)")
    }

    // MARK: Bookmark Persistence
    private func saveBookmarksToStorage() {
        if let encoded = try? JSONEncoder().encode(bookmarks) {
            UserDefaults.standard.set(encoded, forKey: "camera.bookmarks")
        }
    }

    private func loadBookmarksFromStorage() {
        if let data = UserDefaults.standard.data(forKey: "camera.bookmarks"),
           let decoded = try? JSONDecoder().decode([CameraBookmark].self, from: data) {
            bookmarks = decoded
        }
    }

    func initializeBookmarks() {
        loadBookmarksFromStorage()
    }
    
    func deleteBookmark(_ id: UUID) {
        bookmarks.removeAll { $0.id == id }
    }
    
    // MARK: Path Recording
    func startPathRecording() {
        isRecordingPath = true
        recordedPath.removeAll()
        print("[Camera] Started path recording")
    }

    func stopPathRecording() {
        isRecordingPath = false
        print("[Camera] Stopped path recording (\(recordedPath.count) keyframes)")
    }

    func recordKeyframe(position: SIMD3<Float>, rotation: SIMD3<Float>, timestamp: TimeInterval) {
        guard isRecordingPath else { return }
        let keyframe = CameraKeyframe(timestamp: timestamp, position: position, rotation: rotation, fov: fieldOfView)
        recordedPath.append(keyframe)
    }

    // MARK: Path Playback
    func playbackPath(speed: Float = 1.0) {
        guard !recordedPath.isEmpty else {
            print("[Camera] No path recorded to playback")
            return
        }

        // Stop any existing playback
        stopPathPlayback()

        isPlayingPath = true
        pathPlaybackTime = 0
        pathPlaybackSpeed = speed
        pathPlaybackStartTime = Date().timeIntervalSince1970

        pathPlaybackTimer = Timer.scheduledTimer(withTimeInterval: 1.0/60.0, repeats: true) { [weak self] timer in
            guard let self = self else {
                timer.invalidate()
                return
            }

            let elapsed = Date().timeIntervalSince1970 - self.pathPlaybackStartTime
            self.pathPlaybackTime = elapsed * Double(self.pathPlaybackSpeed)

            // Find the keyframe range to interpolate
            guard let (index, t) = self.findKeyframeRange(at: self.pathPlaybackTime) else {
                // Playback finished
                timer.invalidate()
                self.isPlayingPath = false
                print("[Camera] Path playback finished")
                return
            }

            let startFrame = self.recordedPath[index]
            let endFrame = self.recordedPath[index + 1]
            let interpolated = startFrame.interpolate(to: endFrame, at: t, type: .easeInOut)

            self.cameraPosition = interpolated.position
            self.cameraRotation = interpolated.rotation
            self.fieldOfView = interpolated.fov
        }

        print("[Camera] Playing back recorded path at speed: \(speed)x")
    }

    func stopPathPlayback() {
        pathPlaybackTimer?.invalidate()
        pathPlaybackTimer = nil
        isPlayingPath = false
        pathPlaybackTime = 0
        print("[Camera] Stopped path playback")
    }

    private func findKeyframeRange(at time: TimeInterval) -> (index: Int, t: Float)? {
        guard recordedPath.count >= 2 else { return nil }

        for i in 0..<(recordedPath.count - 1) {
            let start = recordedPath[i].timestamp
            let end = recordedPath[i + 1].timestamp

            if time >= start && time <= end {
                let duration = end - start
                let t = duration > 0 ? Float((time - start) / duration) : 0
                return (i, t)
            }
        }

        // Check if we've passed the last keyframe
        if time > recordedPath.last?.timestamp ?? 0 {
            return nil
        }

        return nil
    }

    // MARK: Orbit Around Point
    func orbitAroundPoint(_ point: SIMD3<Float>, angularVelocity: Float) {
        // Calculate current offset from pivot
        let offset = cameraPosition - point
        let distance = length(offset)

        // Update angle based on angular velocity
        let deltaAngle = angularVelocity * 0.016 // Assume 60fps, 0.016s per frame
        orbitAngle.y += deltaAngle

        // Recalculate position
        let yawRad = orbitAngle.y * .pi / 180.0
        let pitchRad = orbitAngle.x * .pi / 180.0

        let x = distance * cos(pitchRad) * sin(yawRad)
        let y = distance * sin(pitchRad)
        let z = distance * cos(pitchRad) * cos(yawRad)

        cameraPosition = point + SIMD3(x, y, z)

        // Look at pivot point
        let lookDirection = normalize(point - cameraPosition)
        cameraRotation.x = asin(lookDirection.y) * 180.0 / .pi
        cameraRotation.y = atan2(lookDirection.x, lookDirection.z) * 180.0 / .pi

        print("[Camera] Orbiting around point: \(point) with velocity: \(angularVelocity)")
    }

    // MARK: Dolly Zoom Effect
    func dollyZoom(targetFov: Float, duration: TimeInterval) {
        let startFov = fieldOfView
        let startDistance = length(cameraPosition)

        // Clamp target FOV to reasonable range
        let clampedTargetFov = max(1.0, min(targetFov, 179.0))

        // Calculate target distance using dolly zoom formula
        let startFovRad = startFov * .pi / 360.0
        let targetFovRad = clampedTargetFov * .pi / 360.0
        let targetDistance = startDistance * tan(startFovRad) / tan(targetFovRad)

        let startTime = Date()

        let timer = Timer.scheduledTimer(withTimeInterval: 1.0/60.0, repeats: true) { [weak self] timer in
            guard let self = self else {
                timer.invalidate()
                return
            }

            let elapsed = Date().timeIntervalSince(startTime)
            let progress = min(Float(elapsed / duration), 1.0)
            let easedProgress = progress * progress * (3.0 - 2.0 * progress) // Smoothstep

            // Interpolate FOV and distance
            let currentFov = startFov + (clampedTargetFov - startFov) * easedProgress
            let currentDistance = startDistance + (targetDistance - startDistance) * easedProgress

            // Update camera position while maintaining look-at direction
            let direction = normalize(self.cameraPosition)
            self.cameraPosition = direction * currentDistance
            self.fieldOfView = currentFov

            if progress >= 1.0 {
                timer.invalidate()
                self.fieldOfView = clampedTargetFov
                self.cameraPosition = normalize(self.cameraPosition) * targetDistance
            }
        }

        print("[Camera] Dolly zoom to FOV: \(clampedTargetFov)")
    }
    
    // MARK: Camera View Modes
    func setupOrthographicView(position: SIMD3<Float>, rotation: SIMD3<Float>) {
        cameraPosition = position
        cameraRotation = rotation
    }

    func setTopDownView() {
        // Position camera above the scene looking straight down
        let centerY = cameraPosition.y
        cameraPosition = SIMD3(0, centerY + 50, 0)
        cameraRotation = SIMD3(-90, 0, 0)
        cameraMode = .topDown
        print("[Camera] Set to top-down orthographic view")
    }

    func setSideView() {
        // Position camera to the side looking straight
        let centerY = cameraPosition.y
        let centerZ = cameraPosition.z
        cameraPosition = SIMD3(-50, centerY, centerZ)
        cameraRotation = SIMD3(0, 90, 0)
        cameraMode = .sideView
        print("[Camera] Set to side orthographic view")
    }

    func setFrontView() {
        // Position camera in front looking straight
        let centerY = cameraPosition.y
        let centerX = cameraPosition.x
        cameraPosition = SIMD3(centerX, centerY, -50)
        cameraRotation = SIMD3(0, 0, 0)
        cameraMode = .frontView
        print("[Camera] Set to front orthographic view")
    }

    func setIsometricView() {
        // Isometric angle: 45° horizontal, 35.264° vertical
        let distance: Float = 10
        let angle: Float = 35.264
        let horizontalAngle: Float = 45

        let pitchRad = angle * .pi / 180.0
        let yawRad = horizontalAngle * .pi / 180.0

        let x = distance * cos(pitchRad) * sin(yawRad)
        let y = distance * sin(pitchRad)
        let z = distance * cos(pitchRad) * cos(yawRad)

        cameraPosition = SIMD3(x, y, z)
        cameraRotation = SIMD3(-angle, horizontalAngle, 0)
        cameraMode = .isometric
        print("[Camera] Set to isometric view")
    }

    func setFirstPersonView(targetPosition: SIMD3<Float>) {
        // Position camera at eye level of target
        cameraPosition = targetPosition + SIMD3(0, 1.6, 0) // Approximate eye height
        cameraMode = .firstPerson
        print("[Camera] Set to first-person view")
    }

    func setThirdPersonView(targetPosition: SIMD3<Float>, distance: Float = 3.0) {
        // Position camera behind and above target
        let offset = SIMD3(0, 1, -distance)
        cameraPosition = targetPosition + offset
        cameraRotation = SIMD3(-20, 0, 0)
        cameraMode = .thirdPerson
        print("[Camera] Set to third-person view")
    }

    func setLockedToObject(target: UUID, offset: SIMD3<Float> = SIMD3(0, 2, 3)) {
        followTarget = target
        followDistance = length(offset)
        followHeight = offset.y
        cameraMode = .locked
        print("[Camera] Locked camera to object: \(target)")
    }

    func setFollowMode(target: UUID, distance: Float = 5.0, height: Float = 2.0) {
        followTarget = target
        followDistance = distance
        followHeight = height
        cameraMode = .follow
        print("[Camera] Set follow mode for target: \(target)")
    }

    // TODO: Implement camera roll control
    func setCameraRoll(_ roll: Float) {
        cameraRoll = roll
        EngineBridge.shared.setCameraRoll(roll)
        print("[Camera] Setting camera roll: \(roll)")
    }
    
    // TODO: Implement camera shake simulation
    func applyCameraShake(intensity: Float, frequency: Float, duration: TimeInterval) {
        shakeIntensity = intensity
        shakeFrequency = frequency
        shakeDuration = duration
        shakeTime = 0.0
        isShaking = true
        
        EngineBridge.shared.setCameraShake(intensity: intensity, frequency: frequency, duration: duration)
        print("[Camera] Applying camera shake: intensity=\(intensity), frequency=\(frequency), duration=\(duration)")
    }
    
    func updateCameraShake(deltaTime: TimeInterval) {
        guard isShaking else { return }
        
        shakeTime += deltaTime
        if shakeTime >= shakeDuration {
            isShaking = false
            shakeIntensity = 0.0
            EngineBridge.shared.setCameraShake(intensity: 0.0, frequency: 0.0, duration: 0.0)
            return
        }
        
        // Calculate shake offset
        let progress = Float(shakeTime / shakeDuration)
        let fadeOut = 1.0 - progress // Linear fade out
        let currentIntensity = shakeIntensity * fadeOut
        
        let time = Float(shakeTime)
        let shakeOffset = SIMD3<Float>(
            sin(time * shakeFrequency * 2.0) * currentIntensity,
            cos(time * shakeFrequency * 3.0) * currentIntensity,
            sin(time * shakeFrequency * 1.5) * currentIntensity * 0.5
        )
        
        EngineBridge.shared.setCameraShakeOffset(shakeOffset)
    }
    
    // TODO: Implement handheld camera simulation
    func enableHandheldSimulation(intensity: Float) {
        handheldEnabled = true
        handheldIntensity = intensity
        handheldTime = 0.0
        
        EngineBridge.shared.setHandheldSimulation(enabled: true, intensity: intensity)
        print("[Camera] Enabling handheld simulation with intensity: \(intensity)")
    }
    
    func disableHandheldSimulation() {
        handheldEnabled = false
        handheldIntensity = 0.0
        
        EngineBridge.shared.setHandheldSimulation(enabled: false, intensity: 0.0)
        print("[Camera] Disabling handheld simulation")
    }
    
    func updateHandheldSimulation(deltaTime: TimeInterval) {
        guard handheldEnabled else { return }
        
        handheldTime += deltaTime
        let time = Float(handheldTime * handheldSpeed)
        
        // Simulate natural handheld movement
        let movement = SIMD3<Float>(
            sin(time * 0.8) * handheldIntensity * 0.3 +
            sin(time * 2.3) * handheldIntensity * 0.1,
            cos(time * 1.2) * handheldIntensity * 0.2 +
            cos(time * 3.1) * handheldIntensity * 0.1,
            sin(time * 0.5) * handheldIntensity * 0.1
        )
        
        // Add subtle rotation
        let rotation = SIMD3<Float>(
            sin(time * 1.5) * handheldIntensity * 0.02,
            cos(time * 2.1) * handheldIntensity * 0.02,
            sin(time * 0.9) * handheldIntensity * 0.01
        )
        
        EngineBridge.shared.setHandheldOffset(movement)
        EngineBridge.shared.setHandheldRotation(rotation)
    }
    
    // TODO: Implement camera look-at target
    func lookAt(_ target: SIMD3<Float>, up: SIMD3<Float> = SIMD3(0, 1, 0)) {
        lookAtTarget = target
        lookAtUpVector = up
        
        EngineBridge.shared.setCameraLookAt(target: target, up: up)
        print("[Camera] Looking at target: \(target)")
    }
    
    func clearLookAtTarget() {
        lookAtTarget = nil
        EngineBridge.shared.clearCameraLookAt()
        print("[Camera] Cleared look-at target")
    }
    
    // TODO: Implement frustumvisualization
    func getFrustumCorners() -> [SIMD3<Float>] {
        // Calculate 8 corners of camera frustum
        let fov = fieldOfView * Float.pi / 180.0
        let aspect = 16.0 / 9.0 // Default aspect ratio
        let near = nearPlane
        let far = farPlane
        
        let nearHeight = tan(fov / 2.0) * near
        let nearWidth = nearHeight * aspect
        let farHeight = tan(fov / 2.0) * far
        let farWidth = farHeight * aspect
        
        // Assuming camera is at origin looking down -Z axis
        let corners = [
            // Near plane
            SIMD3<Float>(-nearWidth, -nearHeight, -near),
            SIMD3<Float>(nearWidth, -nearHeight, -near),
            SIMD3<Float>(nearWidth, nearHeight, -near),
            SIMD3<Float>(-nearWidth, nearHeight, -near),
            // Far plane
            SIMD3<Float>(-farWidth, -farHeight, -far),
            SIMD3<Float>(farWidth, -farHeight, -far),
            SIMD3<Float>(farWidth, farHeight, -far),
            SIMD3<Float>(-farWidth, farHeight, -far)
        ]
        
        return corners
    }
    
    func setFrustumVisualization(_ enabled: Bool) {
        showFrustum = enabled
        EngineBridge.shared.setFrustumVisualization(enabled: enabled, color: frustumColor)
        print("[Camera] Frustum visualization: \(enabled)")
    }
    
    func updateCamera(deltaTime: TimeInterval) {
        updateCameraShake(deltaTime: deltaTime)
        updateHandheldSimulation(deltaTime: deltaTime)
    }
    
    func setSpeedPreset(_ preset: SpeedPreset) {
        cameraSpeed = preset.multiplier
    }
    
    func resetCamera() {
        cameraMode = .orbit
        cameraSpeed = 1.0
        fieldOfView = 60.0
        nearPlane = 0.1
        farPlane = 1000.0
        followTarget = nil
        
        // Reset additional features
        cameraRoll = 0.0
        isShaking = false
        shakeIntensity = 0.0
        handheldEnabled = false
        handheldIntensity = 0.0
        lookAtTarget = nil
        showFrustum = false
        
        // Reset engine state
        EngineBridge.shared.setCameraRoll(0.0)
        EngineBridge.shared.setCameraShake(intensity: 0.0, frequency: 0.0, duration: 0.0)
        EngineBridge.shared.setHandheldSimulation(enabled: false, intensity: 0.0)
        EngineBridge.shared.clearCameraLookAt()
        EngineBridge.shared.setFrustumVisualization(enabled: false, color: SIMD4<Float>(0, 0, 0, 0))
        
        print("[Camera] Reset to defaults")
    }
}

// MARK: - Camera Controls UI Panel
struct CameraControlsPanel: View {
    @ObservedObject var controller: AdvancedCameraController
    @State private var showingBookmarks = false
    @State private var showingPathRecorder = false
    @State private var newBookmarkName = ""
    
    var body: some View {
        VStack(spacing: 0) {
            // Camera mode selector
            HStack {
                Text("Camera Mode")
                    .font(DesignSystem.Typography.bodyBold)
                    .foregroundColor(DesignSystem.Colors.textSecondary)
                
                Spacer()
                
                Menu {
                    ForEach(AdvancedCameraController.CameraMode.allCases) { mode in
                        Button(action: {
                            controller.cameraMode = mode
                        }) {
                            HStack {
                                Image(systemName: mode.icon)
                                Text(mode.rawValue)
                            }
                        }
                    }
                } label: {
                    HStack {
                        Image(systemName: controller.cameraMode.icon)
                        Text(controller.cameraMode.rawValue)
                        Image(systemName: "chevron.down")
                            .font(.system(size: 10))
                    }
                    .font(DesignSystem.Typography.body)
                    .foregroundColor(DesignSystem.Colors.textPrimary)
                    .padding(.horizontal, 8)
                    .padding(.vertical, 4)
                    .background(DesignSystem.Colors.backgroundTertiary)
                    .cornerRadius(4)
                }
            }
            .padding(8)
            
            EditorDivider()
            
            // Camera settings
            VStack(spacing: 8) {
                // Speed
                HStack {
                    Text("Speed")
                        .font(DesignSystem.Typography.small)
                        .foregroundColor(DesignSystem.Colors.textSecondary)
                        .frame(width: 80, alignment: .leading)
                    
                    Slider(value: $controller.cameraSpeed, in: 0.1...10.0)
                    
                    Text(String(format: "%.1fx", controller.cameraSpeed))
                        .font(DesignSystem.Typography.small)
                        .foregroundColor(DesignSystem.Colors.textPrimary)
                        .frame(width: 40, alignment: .trailing)
                }
                
                // Field of View
                HStack {
                    Text("FOV")
                        .font(DesignSystem.Typography.small)
                        .foregroundColor(DesignSystem.Colors.textSecondary)
                        .frame(width: 80, alignment: .leading)
                    
                    Slider(value: $controller.fieldOfView, in: 30...120)
                    
                    Text("\(Int(controller.fieldOfView))")
                        .font(DesignSystem.Typography.small)
                        .foregroundColor(DesignSystem.Colors.textPrimary)
                        .frame(width: 40, alignment: .trailing)
                }
                
              // Inertia toggle
                Toggle("Enable Inertia", isOn: $controller.enableInertia)
                    .font(DesignSystem.Typography.small)
                    .toggleStyle(.checkbox)
                
                Toggle("Enable Acceleration", isOn: $controller.enableAcceleration)
                    .font(DesignSystem.Typography.small)
                    .toggleStyle(.checkbox)
            }
            .padding(8)
            
            EditorDivider()
            
            // Bookmarks
            VStack(alignment: .leading, spacing: 8) {
                HStack {
                    Text("Bookmarks (\(controller.bookmarks.count))")
                        .font(DesignSystem.Typography.bodyBold)
                        .foregroundColor(DesignSystem.Colors.textSecondary)
                    
                    Spacer()
                    
                    Button(action: {
                        showingBookmarks.toggle()
                    }) {
                        Image(systemName: showingBookmarks ? "chevron.up" : "chevron.down")
                            .font(.system(size: 10))
                    }
                    .buttonStyle(.plain)
                }
                
                if showingBookmarks {
                    ForEach(controller.bookmarks) { bookmark in
                        BookmarkRow(
                            bookmark: bookmark,
                            onLoad: {
                                controller.loadBookmark(bookmark)
                            },
                            onDelete: {
                                controller.deleteBookmark(bookmark.id)
                            }
                        )
                    }
                    
                    EditorButton("Save Current Position", icon: "plus", style: .primary) {
                        let currentPosition = EngineBridge.shared.getCameraPosition()
                        let currentRotation = EngineBridge.shared.getCameraRotation()
                        controller.saveBookmark(name: "Bookmark \(controller.bookmarks.count + 1)", position: currentPosition, rotation: currentRotation)
                    }
                }
            }
            .padding(8)
            
            EditorDivider()
            
            // Path Recording
            HStack {
                Text("Path Recording")
                    .font(DesignSystem.Typography.bodyBold)
                    .foregroundColor(DesignSystem.Colors.textSecondary)
                
                Spacer()
                
                if controller.isRecordingPath {
                    Button(action: {
                        controller.stopPathRecording()
                    }) {
                        HStack {
                            Image(systemName: "stop.circle.fill")
                            Text("Stop")
                        }
                        .foregroundColor(.red)
                    }
                    .buttonStyle(.plain)
                } else {
                    Button(action: {
                        controller.startPathRecording()
                    }) {
                        HStack {
                            Image(systemName: "record.circle")
                            Text("Record")
                        }
                    }
                    .buttonStyle(.plain)
                }
            }
            .padding(8)
        }
        .background(DesignSystem.Colors.backgroundSecondary)
        .cornerRadius(4)
    }
}

// MARK: - Bookmark Row
private struct BookmarkRow: View {
    let bookmark: AdvancedCameraController.CameraBookmark
    let onLoad: () -> Void
    let onDelete: () -> Void
    
    var body: some View {
        HStack {
            VStack(alignment: .leading, spacing: 2) {
                Text(bookmark.name)
                    .font(DesignSystem.Typography.body)
                    .foregroundColor(DesignSystem.Colors.textPrimary)
                
                Text(bookmark.timestamp, style: .relative)
                    .font(DesignSystem.Typography.small)
                    .foregroundColor(DesignSystem.Colors.textSecondary)
            }
            
            Spacer()
            
            Button(action: onLoad) {
                Image(systemName: "arrow.uturn.backward")
                    .foregroundColor(DesignSystem.Colors.accentPrimary)
            }
            .buttonStyle(.plain)
            
            Button(action: onDelete) {
                Image(systemName: "trash")
                    .foregroundColor(.red)
            }
            .buttonStyle(.plain)
        }
        .padding(4)
        .background(DesignSystem.Colors.backgroundTertiary)
        .cornerRadius(4)
    }
}

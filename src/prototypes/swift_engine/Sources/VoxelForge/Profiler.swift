import SwiftUI

public struct ProfilerStats: Sendable {
    public var fps: Double
    public var frameTime: Double
    public var drawCalls: Int
    public var memoryUsed: Int64
}

@MainActor
public final class Profiler: ObservableObject {
    public static let shared = Profiler()
    
    @Published public var stats = ProfilerStats(fps: 0, frameTime: 0, drawCalls: 0, memoryUsed: 0)
    
    private var lastTime: CFTimeInterval = 0
    private var frameCount: Int = 0
    
    private init() {}
    
    public func tick() {
        let currentTime = CACurrentMediaTime()
        frameCount += 1
        
        if currentTime - lastTime >= 1.0 {
            stats.fps = Double(frameCount) / (currentTime - lastTime)
            stats.frameTime = (currentTime - lastTime) / Double(frameCount) * 1000.0
            stats.memoryUsed = Int64(reportMemory())
            
            frameCount = 0
            lastTime = currentTime
        }
    }
    
    private func reportMemory() -> UInt64 {
        var info = mach_task_basic_info()
        var count = mach_msg_type_number_t(MemoryLayout<mach_task_basic_info>.size) / 4
        
        let kerr: kern_return_t = withUnsafeMutablePointer(to: &info) {
            $0.withMemoryRebound(to: integer_t.self, capacity: Int(count)) {
                task_info(mach_task_self_, task_flavor_t(MACH_TASK_BASIC_INFO), $0, &count)
            }
        }
        
        if kerr == KERN_SUCCESS {
            return info.resident_size
        } else {
            return 0
        }
    }
}

public struct ProfilerView: View {
    @ObservedObject var profiler = Profiler.shared
    
    public init() {}
    
    public var body: some View {
        VStack(alignment: .leading, spacing: 4) {
            Text("FPS: \(String(format: "%.1f", profiler.stats.fps))")
            Text("Frame Time: \(String(format: "%.2f", profiler.stats.frameTime))ms")
            Text("Memory: \(profiler.stats.memoryUsed / 1024 / 1024)MB")
        }
        .padding(8)
        .background(Color.black.opacity(0.7))
        .foregroundColor(.green)
        .font(.system(.caption, design: .monospaced))
        .cornerRadius(4)
    }
}

import Foundation

public enum JobPriority: Int, Comparable {
    case low = 0
    case medium = 1
    case high = 2
    case immediate = 3
    
    public static func < (lhs: JobPriority, rhs: JobPriority) -> Bool {
        return lhs.rawValue < rhs.rawValue
    }
}

public final class JobSystem: Sendable {
    public static let shared = JobSystem()
    
    private let workerQueue = DispatchQueue(label: "com.voxelforge.jobs", attributes: .concurrent)
    private let highPriorityQueue = DispatchQueue(label: "com.voxelforge.jobs.high", qos: .userInteractive, attributes: .concurrent)
    private let backgroundQueue = DispatchQueue(label: "com.voxelforge.jobs.background", qos: .background, attributes: .concurrent)
    
    private init() {}
    
    /// Dispatches a job to the appropriate queue based on priority.
    public func dispatch(priority: JobPriority = .medium, work: @escaping @Sendable () -> Void) {
        switch priority {
        case .immediate:
            highPriorityQueue.async(execute: work)
        case .high:
            highPriorityQueue.async(execute: work)
        case .medium:
            workerQueue.async(execute: work)
        case .low:
            backgroundQueue.async(execute: work)
        }
    }
    
    /// Spawns a group of parallel jobs (e.g., for chunk generation).
    public func parallelFor(count: Int, priority: JobPriority = .medium, work: @escaping @Sendable (Int) -> Void) {
        DispatchQueue.concurrentPerform(iterations: count) { index in
            work(index)
        }
    }
}

import Foundation

/// A high-performance memory management system for the engine.
@MainActor
public final class MemoryManager {
    public static let shared = MemoryManager()
    
    // Dedicated allocator for frame-temporary data
    public let frameAllocator: StackAllocator
    
    private init() {
        // Initialize frame allocator with 16MB of pre-allocated memory
        self.frameAllocator = StackAllocator(capacity: 16 * 1024 * 1024)
    }
    
    /// Resets frame-temporary memory at the end of each engine tick.
    public func resetFrameAllocator() {
        frameAllocator.reset()
    }
}

/// A simple, fast stack-based allocator for temporary data.
public final class StackAllocator {
    private let buffer: UnsafeMutableRawPointer
    private let capacity: Int
    private var offset: Int = 0
    
    public init(capacity: Int) {
        self.capacity = capacity
        self.buffer = UnsafeMutableRawPointer.allocate(byteCount: capacity, alignment: 16)
    }
    
    deinit {
        buffer.deallocate()
    }
    
    /// Allocates memory of type T from the stack.
    public func allocate<T>(count: Int = 1) -> UnsafeMutablePointer<T>? {
        let size = MemoryLayout<T>.stride * count
        let alignment = MemoryLayout<T>.alignment
        
        // Align the current offset
        let alignedOffset = (offset + alignment - 1) & ~(alignment - 1)
        
        if alignedOffset + size > capacity {
            print("StackAllocator: Out of memory!")
            return nil
        }
        
        offset = alignedOffset + size
        return (buffer + alignedOffset).assumingMemoryBound(to: T.self)
    }
    
    /// Resets the stack pointer to the beginning.
    public func reset() {
        offset = 0
    }
}

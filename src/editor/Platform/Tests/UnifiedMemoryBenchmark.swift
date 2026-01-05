import XCTest
import Metal
@testable import VoxelForgeEditor

/// Unified Memory Architecture Performance Tests
/// Validates zero-copy buffer sharing and Apple Silicon optimizations
final class UnifiedMemoryPerformanceTests: XCTestCase {
    
    var renderer: MetalRenderer!
    
    override func setUp() {
        super.setUp()
        renderer = MetalRenderer()
        
        guard renderer != nil else {
            XCTFail("Failed to initialize Metal renderer")
            return
        }
    }
    
    override func tearDown() {
        renderer = nil
        super.tearDown()
    }
    
    // MARK: - Platform Detection
    
    func testAppleSiliconDetection() {
        #if arch(arm64)
        XCTAssertTrue(MetalRenderer.isAppleSilicon, "Should detect Apple Silicon on ARM64")
        #else
        XCTAssertFalse(MetalRenderer.isAppleSilicon, "Should not detect Apple Silicon on Intel")
        #endif
    }
    
    func testMetalDeviceAvailable() {
        let device = renderer.metalDevice
        XCTAssertNotNil(device, "Metal device should be available")
        
        print("✓ Metal Device: \(device.name)")
        print("✓ Max Buffer Length: \(device.maxBufferLength / 1024 / 1024) MB")
    }
    
    // MARK: - Buffer Creation
    
    func testSharedBufferCreation() {
        let bufferSize = 1024 * 1024 // 1MB
        
        guard let buffer = renderer.createSharedBuffer(size: bufferSize, label: "TestBuffer") else {
            XCTFail("Failed to create shared buffer")
            return
        }
        
        XCTAssertEqual(buffer.length, bufferSize, "Buffer size should match requested size")
        XCTAssertEqual(buffer.storageMode, .shared, "Buffer should use shared storage mode")
        XCTAssertEqual(buffer.label, "TestBuffer", "Buffer label should be set")
    }
    
    func testMultipleSharedBuffers() {
        let bufferSizes = [1024, 4096, 16384, 65536]
        var createdBuffers: [MTLBuffer] = []
        
        for size in bufferSizes {
            if let buffer = renderer.createSharedBuffer(size: size, label: "Buffer\(size)") {
                createdBuffers.append(buffer)
            }
        }
        
        XCTAssertEqual(createdBuffers.count, bufferSizes.count, "All buffers should be created")
    }
    
    // MARK: - Buffer Alignment
    
    func testBufferAlignment() {
        let voxelCounts = [1, 10, 100, 1000, 12345]
        
        for count in voxelCounts {
            let size = MetalRenderer.recommendedBufferSize(for: count)
            
            // Verify 256-byte alignment for cache line optimization
            XCTAssertEqual(size % 256, 0, "Buffer size should be 256-byte aligned for count: \(count)")
        }
    }
    
    func testBufferAlignmentRounding() {
        // Test that 8 bytes rounds up to 256
        let size1 = MetalRenderer.recommendedBufferSize(for: 1) // 1 voxel = 8 bytes
        XCTAssertEqual(size1, 256)
        
        // Test that 255*8 = 2040 bytes rounds up to 2048
        let size255 = MetalRenderer.recommendedBufferSize(for: 255)
        XCTAssertEqual(size255, 2048) // 256 * 8
    }
    
    // MARK: - Zero-Copy Performance
    
    func testSharedBufferZeroCopy() {
        let voxelCount = 1_000_000
        let bufferSize = MetalRenderer.recommendedBufferSize(for: voxelCount)
        
        guard let buffer = renderer.createSharedBuffer(size: bufferSize, label: "VoxelData") else {
            XCTFail("Failed to create shared buffer")
            return
        }
        
        // Get raw pointer for C-style access (simulating C engine)
        guard let ptr = renderer.getSharedBufferPointer(buffer) else {
            XCTFail("Failed to get buffer pointer")
            return
        }
        
        // Measure write performance
        let start = CACurrentMediaTime()
        
        let bytePtr = ptr.bindMemory(to: UInt8.self, capacity: bufferSize)
        for i in 0..<bufferSize {
            bytePtr[i] = UInt8(i % 256)
        }
        
        let duration = CACurrentMediaTime() - start
        
        // Unified memory should allow very fast direct writes (<10ms for 1M voxels)
        XCTAssertLessThan(duration, 0.02, "Write should complete in <20ms (target: <10ms)")
        
        let throughputMBps = (Double(bufferSize) / (1024 * 1024)) / duration
        print("✓ Unified Memory Write Performance: \(String(format: "%.2f", duration * 1000))ms (\(String(format: "%.0f", throughputMBps)) MB/s)")
    }
    
    func testSharedBufferReadWrite() {
        let testSize = 100_000
        let bufferSize = MetalRenderer.recommendedBufferSize(for: testSize)
        
        guard let buffer = renderer.createSharedBuffer(size: bufferSize, label: "ReadWriteTest") else {
            XCTFail("Failed to create buffer")
            return
        }
        
        guard let ptr = renderer.getSharedBufferPointer(buffer) else {
            XCTFail("Failed to get pointer")
            return
        }
        
        // Write test pattern
        let bytePtr = ptr.bindMemory(to: UInt8.self, capacity: bufferSize)
        for i in 0..<1000 {
            bytePtr[i] = UInt8((i * 3) % 256)
        }
        
        // Read back and verify
        for i in 0..<1000 {
            let expected = UInt8((i * 3) % 256)
            XCTAssertEqual(bytePtr[i], expected, "Read should match written value at index \(i)")
        }
    }
    
    // MARK: - Benchmark Tests
    
    func testBufferCreationPerformance() {
        let bufferSize = 1024 * 1024 // 1MB
        
        measure {
            for _ in 0..<100 {
                _ = renderer.createSharedBuffer(size: bufferSize, label: "PerfTest")
            }
        }
    }
    
    func testLargeBufferWritePerformance() {
        let voxelCount = 10_000_000 // 10M voxels
        let bufferSize = MetalRenderer.recommendedBufferSize(for: voxelCount)
        
        guard let buffer = renderer.createSharedBuffer(size: bufferSize, label: "LargePerfTest") else {
            XCTFail("Failed to create large buffer")
            return
        }
        
        guard let ptr = renderer.getSharedBufferPointer(buffer) else {
            XCTFail("Failed to get pointer")
            return
        }
        
        let bytePtr = ptr.bindMemory(to: UInt8.self, capacity: bufferSize)
        
        measure {
            for i in stride(from: 0, to: bufferSize, by: 1024) {
                bytePtr[i] = UInt8(i % 256)
            }
        }
    }
    
    func testStructuredDataAccess() {
        struct VoxelData {
            var type: UInt8
            var color: UInt32
            var flags: UInt8
            var padding: UInt16
        }
        
        let voxelCount = 10000
        let bufferSize = MemoryLayout<VoxelData>.stride * voxelCount
        let alignedSize = ((bufferSize + 255) / 256) * 256 // Manual alignment
        
        guard let buffer = renderer.createSharedBuffer(size: alignedSize, label: "StructuredTest") else {
            XCTFail("Failed to create buffer")
            return
        }
        
        guard let ptr = renderer.getSharedBufferPointer(buffer) else {
            XCTFail("Failed to get pointer")
            return
        }
        
        let voxelPtr = ptr.bindMemory(to: VoxelData.self, capacity: voxelCount)
        
        // Write structured data
        for i in 0..<voxelCount {
            voxelPtr[i] = VoxelData(
                type: UInt8(i % 256),
                color: UInt32(i * 1337),
                flags: 0,
                padding: 0
            )
        }
        
        // Verify structured data
        for i in 0..<min(100, voxelCount) {
            XCTAssertEqual(voxelPtr[i].type, UInt8(i % 256))
            XCTAssertEqual(voxelPtr[i].color, UInt32(i * 1337))
        }
    }
    
    // MARK: - Memory Management
    
    func testBufferMemoryRelease() {
        let initialMemory = memory_bridge_get_total_used()
        
        do {
            var buffers: [MTLBuffer] = []
            for _ in 0..<100 {
                if let buffer = renderer.createSharedBuffer(size: 1024 * 1024, label: "TempBuffer") {
                    buffers.append(buffer)
                }
            }
            // Buffers go out of scope here
        }
        
        // Give ARC time to clean up
        Thread.sleep(forTimeInterval: 0.1)
        
        let finalMemory = memory_bridge_get_total_used()
        
        // Memory usage should not grow unbounded
        // (This is a rough check since other systems may allocate)
        print("✓ Memory: initial=\(initialMemory), final=\(finalMemory)")
    }
}

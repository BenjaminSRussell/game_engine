// ThumbnailGeneratorAPI.swift
// Swift wrapper for thumbnail generation

import Foundation

public class ThumbnailGeneratorAPI {
    public static let shared = ThumbnailGeneratorAPI()
    private init() {}
    
    // MARK: - Thumbnail Generation
    
    /// Request a thumbnail for an asset
    public func requestThumbnail(for path: String, width: UInt32 = 128, height: UInt32 = 128) -> UInt64 {
        return thumbnail_request(path, width, height)
    }
    
    /// Check if a thumbnail is ready
    public func isReady(_ requestId: UInt64) -> Bool {
        return thumbnail_is_ready(requestId)
    }
    
    /// Get thumbnail data directly into a buffer
    public func getData(requestId: UInt64, buffer: UnsafeMutableRawPointer, size: Int) -> Bool {
        return thumbnail_get_data(requestId, buffer, UInt32(size))
    }
    
    /// Helper to get data as Data object
    public func getThumbnailData(requestId: UInt64, expectedSize: Int) -> Data? {
        guard isReady(requestId) else { return nil }
        
        var data = Data(count: expectedSize)
        let success = data.withUnsafeMutableBytes { ptr in
            guard let baseAddress = ptr.baseAddress else { return false }
            return thumbnail_get_data(requestId, baseAddress, UInt32(expectedSize))
        }
        
        return success ? data : nil
    }
    
    /// Cancel a request
    public func cancelRequest(_ requestId: UInt64) {
        thumbnail_cancel(requestId)
    }
    
    /// Clear cache
    public func clearCache() {
        thumbnail_clear_cache()
    }
    
    /// Get pending count
    public func getPendingCount() -> UInt32 {
        return thumbnail_get_pending_count()
    }
}

// MARK: - C Bridging

@_silgen_name("thumbnail_request")
private func thumbnail_request(_ path: String, _ width: UInt32, _ height: UInt32) -> UInt64

@_silgen_name("thumbnail_is_ready")
private func thumbnail_is_ready(_ requestId: UInt64) -> Bool

@_silgen_name("thumbnail_get_data")
private func thumbnail_get_data(_ requestId: UInt64, _ buffer: UnsafeMutableRawPointer, _ size: UInt32) -> Bool

@_silgen_name("thumbnail_cancel")
private func thumbnail_cancel(_ requestId: UInt64)

@_silgen_name("thumbnail_clear_cache")
private func thumbnail_clear_cache()

@_silgen_name("thumbnail_get_pending_count")
private func thumbnail_get_pending_count() -> UInt32

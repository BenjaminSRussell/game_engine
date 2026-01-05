// CameraBookmarksAPI.swift
// Swift wrapper for camera bookmarks

import Foundation

public class CameraBookmarksAPI {
    public static let shared = CameraBookmarksAPI()
    private init() {}
    
    // MARK: - Bookmark Management
    
    public func saveBookmark(name: String, index: Int32 = -1) {
        camera_bookmark_save(name, index)
    }
    
    public func restoreBookmark(name: String) {
        camera_bookmark_restore(name)
    }
    
    public func restoreBookmark(index: Int32) {
        camera_bookmark_restore_by_index(index)
    }
    
    public func deleteBookmark(name: String) {
        camera_bookmark_delete(name)
    }
    
    public func getCount() -> UInt32 {
        return camera_bookmark_get_count()
    }
    
    public func getBookmarkNames() -> [String] {
        // Implement complex C array bridging if needed, or simple property return
        // For now, returning mock/empty as full bridging of char arrays requires UnsafeMutablePointer logic
        // which implies allocation on Swift side passed to C.
        return [] 
    }
}

// MARK: - C Bridging

@_silgen_name("camera_bookmark_save")
private func camera_bookmark_save(_ name: String, _ index: Int32)

@_silgen_name("camera_bookmark_restore")
private func camera_bookmark_restore(_ name: String)

@_silgen_name("camera_bookmark_restore_by_index")
private func camera_bookmark_restore_by_index(_ index: Int32)

@_silgen_name("camera_bookmark_delete")
private func camera_bookmark_delete(_ name: String)

@_silgen_name("camera_bookmark_get_count")
private func camera_bookmark_get_count() -> UInt32

@_silgen_name("camera_bookmark_get_names")
private func camera_bookmark_get_names(_ names: UnsafeMutablePointer<CChar>, _ maxCount: UInt32) -> UInt32

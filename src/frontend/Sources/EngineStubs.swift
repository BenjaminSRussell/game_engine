import Foundation

// MARK: - Engine Bridge Stubs
// These stubs provide implementations for C functions that are currently missing from the linked engine library.
// They allow the frontend to compile and link, serving as a functional mock layer.

@_cdecl("engine_get_component_count")
func engine_get_component_count_stub(_ entityID: UUID) -> Int32 {
    return 0
}

@_cdecl("engine_get_component_types")
func engine_get_component_types_stub(_ entityID: UUID, _ types: UnsafeMutablePointer<UnsafeMutablePointer<CChar>?>, _ maxCount: Int32) -> Int32 {
    return 0
}

@_cdecl("engine_get_entity_active")
func engine_get_entity_active_stub(_ entityID: UUID) -> Bool {
    return true
}

@_cdecl("engine_get_entity_layer")
func engine_get_entity_layer_stub(_ entityID: UUID) -> UnsafePointer<CChar>? {
    return nil // string pointers need careful handling, returning nil is crashing safer for now if handled, or static string
}

@_cdecl("engine_get_entity_static")
func engine_get_entity_static_stub(_ entityID: UUID) -> Bool {
    return false
}

@_cdecl("engine_get_entity_tag")
func engine_get_entity_tag_stub(_ entityID: UUID) -> UnsafePointer<CChar>? {
    return nil
}

@_cdecl("engine_set_entity_active")
func engine_set_entity_active_stub(_ entityID: UUID, _ isActive: Bool) {
    print("[Stub] Set active: \(isActive) for \(entityID)")
}

@_cdecl("engine_set_entity_layer")
func engine_set_entity_layer_stub(_ entityID: UUID, _ layer: UnsafePointer<CChar>?) {
    if let layer = layer {
        let str = String(cString: layer)
        print("[Stub] Set layer: \(str) for \(entityID)")
    }
}

@_cdecl("engine_set_entity_static")
func engine_set_entity_static_stub(_ entityID: UUID, _ isStatic: Bool) {
    print("[Stub] Set static: \(isStatic) for \(entityID)")
}

@_cdecl("engine_set_entity_tag")
func engine_set_entity_tag_stub(_ entityID: UUID, _ tag: UnsafePointer<CChar>?) {
    if let tag = tag {
        let str = String(cString: tag)
        print("[Stub] Set tag: \(str) for \(entityID)")
    }
}

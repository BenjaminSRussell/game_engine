// ConsoleAPI.swift
// Swift wrapper for Console Commands

import Foundation

public class ConsoleAPI {
    public static let shared = ConsoleAPI()
    private init() {}
    
    public var isEnabled: Bool {
        get { return console_is_enabled() }
        set { console_set_enabled(newValue) }
    }
    
    public func execute(_ command: String) {
        console_execute(command)
    }
    
    public func print(_ message: String) {
        console_print(message)
    }
    
    public func clear() {
        console_clear()
    }
    
    public func getHistoryCount() -> UInt32 {
        return console_get_history_count()
    }
    
    public func getHistoryEntry(at index: UInt32) -> String {
        guard let cString = console_get_history_entry(index) else { return "" }
        return String(cString: cString)
    }
}

// MARK: - C Bridging

@_silgen_name("console_execute")
private func console_execute(_ command: String)

@_silgen_name("console_print")
private func console_print(_ message: String)

@_silgen_name("console_clear")
private func console_clear()

@_silgen_name("console_get_history_count")
private func console_get_history_count() -> UInt32

@_silgen_name("console_get_history_entry")
private func console_get_history_entry(_ index: UInt32) -> UnsafePointer<CChar>?

@_silgen_name("console_set_enabled")
private func console_set_enabled(_ enabled: Bool)

@_silgen_name("console_is_enabled")
private func console_is_enabled() -> Bool

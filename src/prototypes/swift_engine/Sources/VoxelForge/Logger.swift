import Foundation
import os.log

public final class Logger {
    public enum Level: String {
        case debug = "DEBUG"
        case info = "INFO"
        case warning = "WARN"
        case error = "ERROR"
        case fatal = "FATAL"
    }
    
    public static func log(_ message: String, level: Level = .info, category: String = "Engine") {
        let timestamp = ISO8601DateFormatter().string(from: Date())
        let formattedMessage = "[\(timestamp)] [\(level.rawValue)] [\(category)] \(message)"
        
        // Print to console
        print(formattedMessage)
        
        // Also log to system logs
        let type: OSLogType
        switch level {
        case .debug: type = .debug
        case .info: type = .info
        case .warning: type = .default
        case .error, .fatal: type = .error
        }
        
        os_log("%{public}@", log: OSLog(subsystem: "com.voxelforge", category: category), type: type, formattedMessage)
    }
    
    public static func debug(_ message: String, category: String = "Engine") { log(message, level: .debug, category: category) }
    public static func info(_ message: String, category: String = "Engine") { log(message, level: .info, category: category) }
    public static func warn(_ message: String, category: String = "Engine") { log(message, level: .warning, category: category) }
    public static func error(_ message: String, category: String = "Engine") { log(message, level: .error, category: category) }
}

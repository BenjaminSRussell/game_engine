import Foundation

public struct DeploymentConfig {
    public let platform: Platform
    public let minOSVersion: String
    
    public enum Platform {
        case macOS
        case iOS
        case visionOS
    }
    
    public static func defaultIOS() -> DeploymentConfig {
        return DeploymentConfig(platform: .iOS, minOSVersion: "17.0")
    }
}

@MainActor
public final class DeploymentManager {
    public static let shared = DeploymentManager()
    
    private init() {}
    
    public func generateInfoPlist() -> String {
        return """
        <?xml version="1.0" encoding="UTF-8"?>
        <!DOCTYPE plist PUBLIC "-//Apple//DTD PLIST 1.0//EN" "http://www.apple.com/DTDs/PropertyList-1.0.dtd">
        <plist version="1.0">
        <dict>
            <key>CFBundleIdentifier</key>
            <string>com.voxelforge.engine</string>
            <key>LSRequiresIPhoneOS</key>
            <true/>
            <key>MinimumOSVersion</key>
            <string>17.0</string>
        </dict>
        </plist>
        """
    }
}

import SwiftUI

// MARK: - Color Extensions for HDR Support
extension Color {
    var components: (red: Float, green: Float, blue: Float, alpha: Float) {
        #if canImport(UIKit)
        let uiColor = UIColor(self)
        var r: CGFloat = 0, g: CGFloat = 0, b: CGFloat = 0, a: CGFloat = 0
        uiColor.getRed(&r, green: &g, blue: &b, alpha: &a)
        return (Float(r), Float(g), Float(b), Float(a))
        #elseif canImport(AppKit)
        let nsColor = NSColor(self)
        var r: CGFloat = 0, g: CGFloat = 0, b: CGFloat = 0, a: CGFloat = 0
        nsColor.getRed(&r, green: &g, blue: &b, alpha: &a)
        return (Float(r), Float(g), Float(b), Float(a))
        #else
        return (0, 0, 0, 1)
        #endif
    }
    
    func toHSV() -> (hue: Float, saturation: Float, value: Float) {
        let r = components.red
        let g = components.green
        let b = components.blue
        
        let maxVal = max(r, g, b)
        let minVal = min(r, g, b)
        let delta = maxVal - minVal
        
        var hue: Float = 0
        var saturation: Float = maxVal == 0 ? 0 : delta / maxVal
        let value = maxVal
        
        if delta != 0 {
            if maxVal == r {
                hue = ((g - b) / delta).truncatingRemainder(dividingBy: 6) / 6
            } else if maxVal == g {
                hue = ((b - r) / delta + 2) / 6
            } else {
                hue = ((r - g) / delta + 4) / 6
            }
        }
        
        return (hue: hue, saturation: saturation, value: value)
    }
    
    func toHex() -> String {
        let r = Int(components.red * 255)
        let g = Int(components.green * 255)
        let b = Int(components.blue * 255)
        return String(format: "#%02X%02X%02X", r, g, b)
    }
}

// MARK: - SIMD Extensions for Quaternion Support
extension simd_quatf {
    var vector: SIMD3<Float> {
        return SIMD3<Float>(self.x, self.y, self.z)
    }
    
    var x: Float { self.imag.x }
    var y: Float { self.imag.y }
    var z: Float { self.imag.z }
    var w: Float { self.real }
}

extension SIMD3<Float> {
    static func + (left: SIMD3<Float>, right: SIMD3<Float>) -> SIMD3<Float> {
        return SIMD3<Float>(left.x + right.x, left.y + right.y, left.z + right.z)
    }
    
    static func - (left: SIMD3<Float>, right: SIMD3<Float>) -> SIMD3<Float> {
        return SIMD3<Float>(left.x - right.x, left.y - right.y, left.z - right.z)
    }
    
    static func * (left: SIMD3<Float>, right: Float) -> SIMD3<Float> {
        return SIMD3<Float>(left.x * right, left.y * right, left.z * right)
    }
    
    static func / (left: SIMD3<Float>, right: Float) -> SIMD3<Float> {
        return SIMD3<Float>(left.x / right, left.y / right, left.z / right)
    }
}

extension simd_quatf {
    init(vector: SIMD3<Float>, scalar: Float) {
        self.init(ix: vector.x, iy: vector.y, iz: vector.z, r: scalar)
    }
    
    var eulerAngles: (xyz: SIMD3<Float>, yzx: SIMD3<Float>, zxy: SIMD3<Float>) {
        let q = normalize(self)
        
        // XYZ Euler angles
        let xyz = SIMD3<Float>(
            atan2(2 * (q.w * q.x + q.y * q.z), 1 - 2 * (q.x * q.x + q.y * q.y)),
            asin(2 * (q.w * q.y - q.z * q.x)),
            atan2(2 * (q.w * q.z + q.x * q.y), 1 - 2 * (q.y * q.y + q.z * q.z))
        )
        
        // YZX Euler angles
        let yzx = SIMD3<Float>(
            atan2(2 * (q.w * q.x - q.y * q.z), 1 - 2 * (q.x * q.x + q.z * q.z)),
            atan2(2 * (q.w * q.y - q.x * q.z), 1 - 2 * (q.y * q.y + q.z * q.z)),
            asin(2 * (q.w * q.z + q.x * q.y))
        )
        
        // ZXY Euler angles
        let zxy = SIMD3<Float>(
            asin(2 * (q.w * q.x + q.y * q.z)),
            atan2(2 * (q.w * q.y - q.x * q.z), 1 - 2 * (q.x * q.x + q.z * q.z)),
            atan2(2 * (q.w * q.z - q.x * q.y), 1 - 2 * (q.y * q.y + q.z * q.z))
        )
        
        return (xyz: xyz, yzx: yzx, zxy: zxy)
    }
}

// 
// MARK: - CATEGORY 3: INSPECTOR & PROPERTY SYSTEM (TODO-1461 to TODO-1800)
// 

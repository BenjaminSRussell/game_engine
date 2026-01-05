import Foundation
import simd

public struct PerlinNoise: Sendable {
    private let permutation: [Int]
    
    public init(seed: Int) {
        var p = [Int](0..<256)
        var rng = SplitMix64(seed: UInt64(truncatingIfNeeded: seed))
        p.shuffle(using: &rng)
        self.permutation = p + p
    }
    
    public func noise2D(x: Double, z: Double) -> Double {
        let X = Int(floor(x)) & 255
        let Z = Int(floor(z)) & 255
        
        let xf = x - floor(x)
        let zf = z - floor(z)
        
        let u = fade(xf)
        let v = fade(zf)
        
        let a = permutation[X] + Z
        let aa = permutation[a]
        let ab = permutation[a + 1]
        let b = permutation[X + 1] + Z
        let ba = permutation[b]
        let bb = permutation[b + 1]
        
        return lerp(v, lerp(u, grad(permutation[aa], xf, zf),
                            grad(permutation[ba], xf - 1, zf)),
                       lerp(u, grad(permutation[ab], xf, zf - 1),
                            grad(permutation[bb], xf - 1, zf - 1)))
    }
    
    private func fade(_ t: Double) -> Double {
        return t * t * t * (t * (t * 6 - 15) + 10)
    }
    
    private func lerp(_ t: Double, _ a: Double, _ b: Double) -> Double {
        return a + t * (b - a)
    }
    
    private func grad(_ hash: Int, _ x: Double, _ z: Double) -> Double {
        let h = hash & 15
        let u = h < 8 ? x : z
        let v = h < 4 ? z : (h == 12 || h == 14 ? x : z) // Simplified 2D grad
        return ((h & 1) == 0 ? u : -u) + ((h & 2) == 0 ? v : -v)
    }
}

// Simple PRNG for shuffling
struct SplitMix64: RandomNumberGenerator {
    private var state: UInt64
    
    init(seed: UInt64) {
        self.state = seed
    }
    
    mutating func next() -> UInt64 {
        state &+= 0x9E3779B97F4A7C15
        var z = state
        z = (z ^ (z >> 30)) &* 0xBF58476D1CE4E5B9
        z = (z ^ (z >> 27)) &* 0x94D049BB133111EB
        return z ^ (z >> 31)
    }
}

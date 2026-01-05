import SwiftUI

/// Container for animation frames
class SpriteFrame: ObservableObject {
    @Published var layers: [SpriteLayer] = []
    
    init(canvasSize: Int) {
        addLayer(name: "Layer 1", canvasSize: canvasSize)
    }
    
    func addLayer(name: String? = nil, canvasSize: Int) {
        let layerName = name ?? "Layer \(layers.count + 1)"
        let newLayer = SpriteLayer(name: layerName, width: canvasSize, height: canvasSize)
        layers.insert(newLayer, at: 0)
    }
}

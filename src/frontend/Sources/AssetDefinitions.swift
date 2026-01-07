import SwiftUI
import UniformTypeIdentifiers

enum AssetType: String, CaseIterable, Codable {
    case folder = "Folder"
    case texture = "Texture"
    case material = "Material"
    case mesh = "Mesh"
    case audio = "Audio"
    case animation = "Animation"
    case script = "Script"
    case prefab = "Prefab"
    case other = "Other"
    
    var icon: String {
        switch self {
        case .folder: return "folder.fill"
        case .texture: return "photo"
        case .material: return "paintpalette"
        case .mesh: return "cube.transparent"
        case .audio: return "waveform"
        case .animation: return "play.rectangle"
        case .script: return "scroll"
        case .prefab: return "cube.fill"
        case .other: return "doc"
        }
    }
    
    var color: Color {
        switch self {
        case .folder: return .blue
        case .texture: return .green
        case .material: return .orange
        case .mesh: return .cyan
        case .audio: return .purple
        case .animation: return .pink
        case .script: return .yellow
        case .prefab: return .indigo
        case .other: return .gray
        }
    }
}

struct AssetItem: Identifiable, Hashable {
    let id = UUID()
    let name: String
    let type: AssetType
    let size: Int64
    let modifiedDate: Date
    let path: String
    
    var formattedSize: String {
        let formatter = ByteCountFormatter()
        formatter.allowedUnits = [.useAll]
        formatter.countStyle = .file
        return formatter.string(fromByteCount: size)
    }
}

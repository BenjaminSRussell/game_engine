import Foundation

public actor World {
    var chunks: [ChunkPosition: Chunk] = [:]
    
    func getChunk(at position: ChunkPosition) -> Chunk? {
        return chunks[position]
    }
    
    func loadChunk(at position: ChunkPosition) async {
        if chunks[position] != nil { return }
        
        let generator = AdvancedGenerator(seed: Int.random(in: 0...10000))
        
        // Dispatch generation to JobSystem
        await withCheckedContinuation { continuation in
            JobSystem.shared.dispatch(priority: .medium) {
                var newChunk = Chunk(position: position)
                generator.generate(chunk: &newChunk)
                
                Task {
                    await self.setChunk(newChunk, at: position)
                    continuation.resume()
                }
            }
        }
    }
    
    private func setChunk(_ chunk: Chunk, at position: ChunkPosition) {
        chunks[position] = chunk
    }
    
    func getChunksToRender() -> [Chunk] {
        // Return all chunks for now. Later: Frustum culling
        return Array(chunks.values)
    }
}

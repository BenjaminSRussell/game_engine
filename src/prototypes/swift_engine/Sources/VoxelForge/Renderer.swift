import Metal
import MetalKit
import simd

// Replicating definitions from ShaderTypes.h for Swift usage
public struct Vertex {
    public var position: SIMD3<Float>
    public var color: SIMD4<Float>
}

public struct Uniforms {
    public var modelViewProjectionMatrix: matrix_float4x4
}

public struct InstanceData {
    public var instancePosition: SIMD3<Float>
    public var instanceColor: SIMD4<Float>
    public var textureIndex: UInt32
}

public struct Light {
    public var position: SIMD3<Float>
    public var color: SIMD3<Float>
    public var intensity: Float
}

enum GBufferIndices {
    static let albedo = 0
    static let normal = 1
    static let depth = 2
}

@MainActor
public class Renderer: NSObject, MTKViewDelegate {
    let device: MTLDevice
    let commandQueue: MTLCommandQueue
    var pipelineState: MTLRenderPipelineState?
    var lightingPipelineState: MTLRenderPipelineState?
    var depthStencilState: MTLDepthStencilState?
    var cullingPipelineState: MTLComputePipelineState?
    
    // G-Buffer Textures
    var albedoTexture: MTLTexture?
    var normalTexture: MTLTexture?
    var gDepthTexture: MTLTexture?
    
    var texture: MTLTexture?
    
    // World Data
    public let world = World()
    var chunkBuffers: [ChunkPosition: MTLBuffer] = [:]
    var chunkInstanceCounts: [ChunkPosition: Int] = [:]
    
    // GPU Culling Data
    var visibleInstanceBuffers: [ChunkPosition: MTLBuffer] = [:]
    var countBuffers: [ChunkPosition: MTLBuffer] = [:]
    
    // Rendering data
    var vertexBuffer: MTLBuffer?
    
    // Projection
    var projectionMatrix: matrix_float4x4 = matrix_identity_float4x4
    var rotation: Float = 0
    
    public init?(metalKitView: MTKView) {
        guard let device = metalKitView.device else { return nil }
        self.device = device
        self.commandQueue = device.makeCommandQueue()!
        
        super.init()
        
        metalKitView.delegate = self
        metalKitView.clearColor = MTLClearColor(red: 0.5, green: 0.7, blue: 0.9, alpha: 1.0) // Sky blue
        metalKitView.depthStencilPixelFormat = .depth32Float
        
        buildPipelineState(view: metalKitView)
        buildCullingPipeline()
        buildDepthState()
        buildBuffers()
        loadTexture()
    }
    
    func buildCullingPipeline() {
        guard let library = try? device.makeDefaultLibrary(bundle: Bundle.module),
              let function = library.makeFunction(name: "frustumCull") else {
            Logger.error("Failed to load culling shader")
            return
        }
        
        do {
            cullingPipelineState = try device.makeComputePipelineState(function: function)
        } catch {
            Logger.error("Failed to create culling pipeline scale: \(error)")
        }
    }
    
    func loadTexture() {
        let textureLoader = MTKTextureLoader(device: device)
        let filePath = "/Users/benjaminrussell/Desktop/Minecraft v2/assets/textures/blocks/block_dirt.png"
        let url = URL(fileURLWithPath: filePath)
        
        do {
            texture = try textureLoader.newTexture(URL: url, options: [
                .origin: MTKTextureLoader.Origin.bottomLeft,
                .generateMipmaps: true
            ])
            print("Loaded texture: \(filePath)")
        } catch {
            print("Failed to load texture: \(error)")
        }
    }
    
    func buildPipelineState(view: MTKView) {
        guard let library = try? device.makeDefaultLibrary(bundle: Bundle.module) else {
            Logger.error("Failed to load default library")
            return
        }
        
        // --- 1. G-Buffer Pass ---
        let gDescriptor = MTLRenderPipelineDescriptor()
        gDescriptor.label = "G-Buffer Pass"
        gDescriptor.vertexFunction = library.makeFunction(name: "gBufferVertex")
        gDescriptor.fragmentFunction = library.makeFunction(name: "gBufferFragment")
        gDescriptor.colorAttachments[GBufferIndices.albedo].pixelFormat = .bgra8Unorm
        gDescriptor.colorAttachments[GBufferIndices.normal].pixelFormat = .rgba16Float
        gDescriptor.colorAttachments[GBufferIndices.depth].pixelFormat = .r32Float
        gDescriptor.depthAttachmentPixelFormat = .depth32Float
        
        do {
            pipelineState = try device.makeRenderPipelineState(descriptor: gDescriptor)
        } catch {
            Logger.error("Failed to create G-Buffer pipeline: \(error)")
        }
        
        // --- 2. Lighting Pass ---
        let lDescriptor = MTLRenderPipelineDescriptor()
        lDescriptor.label = "Lighting Pass"
        lDescriptor.vertexFunction = library.makeFunction(name: "lightingVertex")
        lDescriptor.fragmentFunction = library.makeFunction(name: "lightingFragment")
        lDescriptor.colorAttachments[0].pixelFormat = view.colorPixelFormat
        
        do {
            lightingPipelineState = try device.makeRenderPipelineState(descriptor: lDescriptor)
        } catch {
            Logger.error("Failed to create Lighting pipeline: \(error)")
        }
    }
    
    func buildDepthState() {
        let descriptor = MTLDepthStencilDescriptor()
        descriptor.depthCompareFunction = .less
        descriptor.isDepthWriteEnabled = true
        depthStencilState = device.makeDepthStencilState(descriptor: descriptor)
    }
    
    func buildBuffers() {
        // Standard Unit Cube (centered at 0,0,0)
        let s: Float = 0.5
        let vertices = [
            // Front
            Vertex(position: SIMD3(-s, -s, s), color: SIMD4(0,0,1,1)),
            Vertex(position: SIMD3( s, -s, s), color: SIMD4(0,0,1,1)),
            Vertex(position: SIMD3( s,  s, s), color: SIMD4(0,0,1,1)),
            Vertex(position: SIMD3(-s, -s, s), color: SIMD4(0,0,1,1)),
            Vertex(position: SIMD3( s,  s, s), color: SIMD4(0,0,1,1)),
            Vertex(position: SIMD3(-s,  s, s), color: SIMD4(0,0,1,1))
        ]
        
        vertexBuffer = device.makeBuffer(bytes: vertices,
                                         length: vertices.count * MemoryLayout<Vertex>.stride,
                                         options: .storageModeShared)
        
        // Initial World Load
        Task {
            // Load a 4x4 area of chunks
            for x in -2..<2 {
                for z in -2..<2 {
                    let pos = ChunkPosition(x: x, z: z)
                    await world.loadChunk(at: pos)
                    if let chunk = await world.getChunk(at: pos) {
                        updateChunkMesh(chunk: chunk)
                    }
                }
            }
        }
    }
    
    func updateChunkMesh(chunk: Chunk) {
        var instances: [InstanceData] = []
        
        // Naive iteration for now (optimize to skip drawing hidden blocks later)
        let chunkOffsetX = Float(chunk.position.x * CHUNK_SIZE)
        let chunkOffsetZ = Float(chunk.position.z * CHUNK_SIZE)
        
        for index in 0..<chunk.blocks.count {
            let block = chunk.blocks[index]
            if block.active {
                // Recover x,y,z from index
                let y = index / (CHUNK_SIZE * CHUNK_SIZE)
                let remaining = index % (CHUNK_SIZE * CHUNK_SIZE)
                let z = remaining / CHUNK_SIZE
                let x = remaining % CHUNK_SIZE
                
                let worldPos = SIMD3<Float>(
                    chunkOffsetX + Float(x),
                    Float(y),
                    chunkOffsetZ + Float(z)
                )
                
                var color: SIMD4<Float> = SIMD4(1, 1, 1, 1)
                var texIndex: UInt32 = 0
                
                if block.type == 2 {
                    color = SIMD4(0.2, 0.8, 0.2, 1)
                    texIndex = 0 // Grass
                } else if block.type == 1 {
                    color = SIMD4(0.6, 0.4, 0.2, 1)
                    texIndex = 1 // Dirt
                }
                
                instances.append(InstanceData(
                    instancePosition: worldPos,
                    instanceColor: color,
                    textureIndex: texIndex
                ))
            }
        }
        
        if !instances.isEmpty {
            let buffer = device.makeBuffer(bytes: instances,
                                           length: instances.count * MemoryLayout<InstanceData>.stride,
                                           options: .storageModeShared)
            chunkBuffers[chunk.position] = buffer
            chunkInstanceCounts[chunk.position] = instances.count
            
            // Allocate GPU-side buffers for culling results
            visibleInstanceBuffers[chunk.position] = device.makeBuffer(length: instances.count * MemoryLayout<InstanceData>.stride,
                                                                      options: .storageModePrivate)
            countBuffers[chunk.position] = device.makeBuffer(length: MemoryLayout<UInt32>.stride,
                                                            options: .storageModeShared)
        }
    }
    
    public func mtkView(_ view: MTKView, drawableSizeWillChange size: CGSize) {
        let width = Int(size.width)
        let height = Int(size.height)
        
        let descriptor = MTLTextureDescriptor.texture2DDescriptor(pixelFormat: .bgra8Unorm, width: width, height: height, mipmapped: false)
        descriptor.usage = [.renderTarget, .shaderRead]
        descriptor.storageMode = .private
        
        albedoTexture = device.makeTexture(descriptor: descriptor)
        
        descriptor.pixelFormat = .rgba16Float
        normalTexture = device.makeTexture(descriptor: descriptor)
        
        descriptor.pixelFormat = .r32Float
        gDepthTexture = device.makeTexture(descriptor: descriptor)
        
        let aspect = Float(size.width) / Float(size.height)
        let fov: Float = 65.0 * (.pi / 180.0)
        let near: Float = 0.1
        let far: Float = 1000.0
        let yScale = 1.0 / tan(fov * 0.5)
        let xScale = yScale / aspect
        let zScale = far / (far - near)
        let zTrans = -near * zScale
        
        projectionMatrix = matrix_float4x4(rows: [
            SIMD4(xScale, 0, 0, 0),
            SIMD4(0, yScale, 0, 0),
            SIMD4(0, 0, zScale, 1),
            SIMD4(0, 0, zTrans, 0)
        ])
    }
    
    public func draw(in view: MTKView) {
        // Update Atmosphere
        SkySystem.shared.update(deltaTime: 1.0 / 60.0) // Assume 60 FPS for smooth transitions
        let sky = SkySystem.shared.skyColor
        view.clearColor = MTLClearColor(red: Double(sky.x), green: Double(sky.y), blue: Double(sky.z), alpha: 1.0)
        
        guard let drawable = view.currentDrawable,
              let descriptor = view.currentRenderPassDescriptor,
              let pipelineState = pipelineState,
              let depthState = depthStencilState,
              let vBuffer = vertexBuffer else { return }
        
        // Update Camera
        rotation += 0.005
        let radius: Float = 60.0
        let camX = sin(rotation/5.0) * radius
        let camZ = cos(rotation/5.0) * radius
        
        // Look at center of world (0, 0, 0)
        let lookAt = matrix_float4x4_look_at(eye: SIMD3(camX, 30, camZ),
                                             target: SIMD3(0, 10, 0),
                                             up: SIMD3(0, 1, 0))
        
        // Pre-calculate Frustum for Culling
        let vp = projectionMatrix * lookAt
        var frustum = Frustum(planes: (
            normalize_plane(SIMD4(vp.columns.0.w + vp.columns.0.x, vp.columns.1.w + vp.columns.1.x, vp.columns.2.w + vp.columns.2.x, vp.columns.3.w + vp.columns.3.x)), // Left
            normalize_plane(SIMD4(vp.columns.0.w - vp.columns.0.x, vp.columns.1.w - vp.columns.1.x, vp.columns.2.w - vp.columns.2.x, vp.columns.3.w - vp.columns.3.x)), // Right
            normalize_plane(SIMD4(vp.columns.0.w + vp.columns.0.y, vp.columns.1.w + vp.columns.1.y, vp.columns.2.w + vp.columns.2.y, vp.columns.3.w + vp.columns.3.y)), // Bottom
            normalize_plane(SIMD4(vp.columns.0.w - vp.columns.0.y, vp.columns.1.w - vp.columns.1.y, vp.columns.2.w - vp.columns.2.y, vp.columns.3.w - vp.columns.3.y)), // Top
            normalize_plane(SIMD4(vp.columns.0.w + vp.columns.0.z, vp.columns.1.w + vp.columns.1.z, vp.columns.2.w + vp.columns.2.z, vp.columns.3.w + vp.columns.3.z)), // Near
            normalize_plane(SIMD4(vp.columns.0.w - vp.columns.0.z, vp.columns.1.w - vp.columns.1.z, vp.columns.2.w - vp.columns.2.z, vp.columns.3.w - vp.columns.3.z))  // Far
        ))
        
        let commandBuffer = commandQueue.makeCommandBuffer()!
        
        // 1. DISPATCH CULLING COMPUTE
        if let cullingState = cullingPipelineState {
            let computeEncoder = commandBuffer.makeComputeCommandEncoder()!
            computeEncoder.setComputePipelineState(cullingState)
            
            for (pos, buffer) in chunkBuffers {
                guard let visibleBuffer = visibleInstanceBuffers[pos],
                      let countBuffer = countBuffers[pos],
                      let totalCount = chunkInstanceCounts[pos] else { continue }
                
                var zero: UInt32 = 0
                countBuffer.contents().copyMemory(from: &zero, byteCount: MemoryLayout<UInt32>.stride)
                
                computeEncoder.setBuffer(buffer, offset: 0, index: 0)
                computeEncoder.setBuffer(visibleBuffer, offset: 0, index: 1)
                computeEncoder.setBuffer(countBuffer, offset: 0, index: 2)
                computeEncoder.setBytes(&frustum, length: MemoryLayout<Frustum>.stride, index: 3)
                
                let threadsPerGrid = MTLSize(width: totalCount, height: 1, depth: 1)
                let threadsPerGroup = MTLSize(width: min(totalCount, cullingState.maxTotalThreadsPerThreadgroup), height: 1, depth: 1)
                computeEncoder.dispatchThreads(threadsPerGrid, threadsPerThreadgroup: threadsPerGroup)
            }
            computeEncoder.endEncoding()
        }
        
        // 2. G-BUFFER PASS
        let gBufferDescriptor = MTLRenderPassDescriptor()
        gBufferDescriptor.colorAttachments[GBufferIndices.albedo].texture = albedoTexture
        gBufferDescriptor.colorAttachments[GBufferIndices.albedo].loadAction = .clear
        gBufferDescriptor.colorAttachments[GBufferIndices.albedo].storeAction = .store
        gBufferDescriptor.colorAttachments[GBufferIndices.albedo].clearColor = MTLClearColor(red: 0, green: 0, blue: 0, alpha: 0)
        
        gBufferDescriptor.colorAttachments[GBufferIndices.normal].texture = normalTexture
        gBufferDescriptor.colorAttachments[GBufferIndices.normal].loadAction = .clear
        gBufferDescriptor.colorAttachments[GBufferIndices.normal].storeAction = .store
        
        gBufferDescriptor.colorAttachments[GBufferIndices.depth].texture = gDepthTexture
        gBufferDescriptor.colorAttachments[GBufferIndices.depth].loadAction = .clear
        gBufferDescriptor.colorAttachments[GBufferIndices.depth].storeAction = .store
        
        gBufferDescriptor.depthAttachment.texture = view.depthStencilTexture
        gBufferDescriptor.depthAttachment.loadAction = .clear
        gBufferDescriptor.depthAttachment.storeAction = .store
        
        let gEncoder = commandBuffer.makeRenderCommandEncoder(descriptor: gBufferDescriptor)!
        gEncoder.setRenderPipelineState(pipelineState)
        gEncoder.setDepthStencilState(depthState)
        gEncoder.setVertexBuffer(vBuffer, offset: 0, index: 0)
        
        var uniforms = Uniforms(modelViewProjectionMatrix: projectionMatrix * lookAt)
        gEncoder.setVertexBytes(&uniforms, length: MemoryLayout<Uniforms>.stride, index: 1)
        
        let textures = TextureManager.shared.getAllTextures()
        if !textures.isEmpty {
            gEncoder.setFragmentTextures(textures, range: 0..<textures.count)
        }
        
        for (pos, _) in chunkBuffers {
            guard let visibleBuffer = visibleInstanceBuffers[pos],
                  let countBuffer = countBuffers[pos] else { continue }
            
            let visibleCount = countBuffer.contents().assumingMemoryBound(to: UInt32.self).pointee
            if visibleCount > 0 {
                gEncoder.setVertexBuffer(visibleBuffer, offset: 0, index: 2)
                gEncoder.drawPrimitives(type: .triangle, vertexStart: 0, vertexCount: 6, instanceCount: Int(visibleCount))
            }
        }
        gEncoder.endEncoding()
        
        // 3. LIGHTING PASS
        let lEncoder = commandBuffer.makeRenderCommandEncoder(descriptor: descriptor)!
        if let lState = lightingPipelineState {
            lEncoder.setRenderPipelineState(lState)
            lEncoder.setFragmentTexture(albedoTexture, index: GBufferIndices.albedo)
            lEncoder.setFragmentTexture(normalTexture, index: GBufferIndices.normal)
            lEncoder.setFragmentTexture(gDepthTexture, index: GBufferIndices.depth)
            
            var light = Light(position: SIMD3(10, 50, 20), color: SIMD3(1, 1, 1), intensity: 1.0)
            lEncoder.setFragmentBytes(&light, length: MemoryLayout<Light>.stride, index: 0)
            
            lEncoder.drawPrimitives(type: .triangleStrip, vertexStart: 0, vertexCount: 4)
        }
        lEncoder.endEncoding()

        
        Profiler.shared.tick()
        
        commandBuffer.present(drawable)
        commandBuffer.commit()
    }
}

// Math Helpers
struct Frustum {
    var planes: (SIMD4<Float>, SIMD4<Float>, SIMD4<Float>, SIMD4<Float>, SIMD4<Float>, SIMD4<Float>)
}

func normalize_plane(_ p: SIMD4<Float>) -> SIMD4<Float> {
    let mag = sqrt(p.x * p.x + p.y * p.y + p.z * p.z)
    return p / mag
}

func matrix_float4x4_look_at(eye: SIMD3<Float>, target: SIMD3<Float>, up: SIMD3<Float>) -> matrix_float4x4 {
    let z = normalize(target - eye)
    let x = normalize(cross(up, z))
    let y = cross(z, x)
    
    return matrix_float4x4(columns: (
        SIMD4(x.x, y.x, z.x, 0),
        SIMD4(x.y, y.y, z.y, 0),
        SIMD4(x.z, y.z, z.z, 0),
        SIMD4(-dot(x, eye), -dot(y, eye), -dot(z, eye), 1)
    ))
}


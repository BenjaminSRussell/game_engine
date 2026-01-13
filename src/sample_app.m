/**
 * @file sample_app.m
 * @brief System Implementation
 * @description System implementation
 * @date 2026-01-13
 */

#import <Cocoa/Cocoa.h>
#import <Metal/Metal.h>
#import <MetalKit/MetalKit.h>
#import <QuartzCore/QuartzCore.h>
#include <simd/simd.h>
#include <stdio.h>
#include <stdlib.h>
#include <math.h>

// Include our new geometry system
// #include "engine/geometry/mesh.h"
// #include "engine/geometry/mesh_primitives.h"
// #include "engine/geometry/mesh_gpu.h"

// Simple vertex structure for testing
typedef struct {
    float position[3];
    float color[3];
    float normal[3];
    float uv[2];
} SimpleVertex;

// Simple mesh structure
typedef struct {
    SimpleVertex* vertices;
    int vertex_count;
    unsigned int* indices;
    int index_count;
} SimpleMesh;

typedef struct {
    vector_float3 position;
    vector_float3 color;
    vector_float3 normal;
    vector_float2 uv;
    vector_float4 tangent;
} Vertex;

@interface MetalView : MTKView <MTKViewDelegate>
@property (nonatomic, strong) id<MTLRenderPipelineState> pipelineState;
@property (nonatomic, strong) id<MTLBuffer> vertexBuffer;
@property (nonatomic, strong) id<MTLBuffer> indexBuffer;
@property (nonatomic, strong) id<MTLBuffer> uniformBuffer;
@property (nonatomic, assign) matrix_float4x4 projectionMatrix;
@property (nonatomic, assign) matrix_float4x4 viewMatrix;
@property (nonatomic, assign) matrix_float4x4 modelMatrix;
@property (nonatomic, assign) float rotationX;
@property (nonatomic, assign) float rotationY;
@property (nonatomic, assign) int vertexCount;
@property (nonatomic, assign) int indexCount;
// @property (nonatomic, assign) mesh_t* mesh;
@property (nonatomic, assign) SimpleMesh* simpleMesh;
@end

@implementation MetalView

- (instancetype)initWithFrame:(CGRect)frame {
    self = [super initWithFrame:frame device:MTLCreateSystemDefaultDevice()];
    if (self) {
        self.delegate = (id<MTKViewDelegate>)self;
        [self setupMetal];
        [self createGeometryFromMesh];
    }
    return self;
}

- (void)setupMetal {
    id<MTLDevice> device = self.device;
    id<MTLLibrary> library = [device newDefaultLibrary];

    id<MTLFunction> vertexFunction = [library newFunctionWithName:@"vertex_main"];
    id<MTLFunction> fragmentFunction = [library newFunctionWithName:@"fragment_main"];

    if (!vertexFunction || !fragmentFunction) {
        NSLog(@"Failed to load shaders from default library");
        library = [self createDefaultShaders];
        if (!library) {
            NSLog(@"Failed to create default shaders");
            return;
        }
        // Use the newly created library to get functions
        vertexFunction = [library newFunctionWithName:@"vertex_main"];
        fragmentFunction = [library newFunctionWithName:@"fragment_main"];
    }

    MTLRenderPipelineDescriptor *pipelineDescriptor = [[MTLRenderPipelineDescriptor alloc] init];
    pipelineDescriptor.vertexFunction = vertexFunction;
    pipelineDescriptor.fragmentFunction = fragmentFunction;
    pipelineDescriptor.colorAttachments[0].pixelFormat = self.colorPixelFormat;
    pipelineDescriptor.depthAttachmentPixelFormat = MTLPixelFormatDepth32Float;

    // Setup vertex descriptor for our enhanced vertex format
    MTLVertexDescriptor *vertexDescriptor = [[MTLVertexDescriptor alloc] init];
    
    // Position
    vertexDescriptor.attributes[0].format = MTLVertexFormatFloat3;
    vertexDescriptor.attributes[0].offset = 0;
    vertexDescriptor.attributes[0].bufferIndex = 0;
    
    // Color
    vertexDescriptor.attributes[1].format = MTLVertexFormatFloat3;
    vertexDescriptor.attributes[1].offset = sizeof(vector_float3);
    vertexDescriptor.attributes[1].bufferIndex = 0;
    
    // Normal
    vertexDescriptor.attributes[2].format = MTLVertexFormatFloat3;
    vertexDescriptor.attributes[2].offset = sizeof(vector_float3) * 2;
    vertexDescriptor.attributes[2].bufferIndex = 0;
    
    // UV
    vertexDescriptor.attributes[3].format = MTLVertexFormatFloat2;
    vertexDescriptor.attributes[3].offset = sizeof(vector_float3) * 3;
    vertexDescriptor.attributes[3].bufferIndex = 0;
    
    // Tangent
    vertexDescriptor.attributes[4].format = MTLVertexFormatFloat4;
    vertexDescriptor.attributes[4].offset = sizeof(vector_float3) * 3 + sizeof(vector_float2);
    vertexDescriptor.attributes[4].bufferIndex = 0;
    
    // Layout
    vertexDescriptor.layouts[0].stride = sizeof(Vertex);
    vertexDescriptor.layouts[0].stepRate = 1;
    vertexDescriptor.layouts[0].stepFunction = MTLVertexStepFunctionPerVertex;
    
    pipelineDescriptor.vertexDescriptor = vertexDescriptor;

    NSError *error = nil;
    self.pipelineState = [device newRenderPipelineStateWithDescriptor:pipelineDescriptor error:&error];
    if (!self.pipelineState) {
        NSLog(@"Pipeline creation error: %@", error);
    }

    [self setupMatrices];
}

- (id<MTLLibrary>)createDefaultShaders {
    id<MTLDevice> device = self.device;

    NSString *shaderSource = @"#include <metal_stdlib>\n"
    "using namespace metal;\n"
    "\n"
    "struct Vertex {\n"
    "    float3 position [[attribute(0)]];\n"
    "    float3 color [[attribute(1)]];\n"
    "    float3 normal [[attribute(2)]];\n"
    "    float2 uv [[attribute(3)]];\n"
    "    float4 tangent [[attribute(4)]];\n"
    "};\n"
    "\n"
    "struct Fragment {\n"
    "    float4 position [[position]];\n"
    "    float3 color;\n"
    "    float3 normal;\n"
    "    float2 uv;\n"
    "    float3 worldPos;\n"
    "};\n"
    "\n"
    "struct Uniforms {\n"
    "    float4x4 projection;\n"
    "    float4x4 view;\n"
    "    float4x4 model;\n"
    "    float3 lightDir;\n"
    "    float time;\n"
    "};\n"
    "\n"
    "vertex Fragment vertex_main(Vertex in [[stage_in]],\n"
    "                           constant Uniforms& uniforms [[buffer(0)]]) {\n"
    "    Fragment out;\n"
    "    float4 worldPos = uniforms.model * float4(in.position, 1.0);\n"
    "    out.position = uniforms.projection * uniforms.view * worldPos;\n"
    "    out.color = in.color;\n"
    "    out.normal = normalize((uniforms.model * float4(in.normal, 0.0)).xyz);\n"
    "    out.uv = in.uv;\n"
    "    out.worldPos = worldPos.xyz;\n"
    "    return out;\n"
    "}\n"
    "\n"
    "fragment float4 fragment_main(Fragment in [[stage_in]],\n"
    "                             constant Uniforms& uniforms [[buffer(0)]]) {\n"
    "    float3 lightDir = normalize(uniforms.lightDir);\n"
    "    float diff = max(dot(in.normal, lightDir), 0.0);\n"
    "    float3 ambient = in.color * 0.3;\n"
    "    float3 diffuse = in.color * diff * 0.7;\n"
    "    \n"
    "    // Add some UV-based color variation\n"
    "    float2 uvPattern = sin(in.uv * 10.0 + uniforms.time) * 0.5 + 0.5;\n"
    "    float3 finalColor = ambient + diffuse * (0.5 + uvPattern.x * 0.5);\n"
    "    \n"
    "    return float4(finalColor, 1.0);\n"
    "}\n";

    NSError *error = nil;
    id<MTLLibrary> library = [device newLibraryWithSource:shaderSource options:nil error:&error];
    if (!library) {
        NSLog(@"Shader compilation error: %@", error);
        return nil;
    }
    return library;
}

- (void)createGeometryFromMesh {
    // Create a simple triangle for testing
    self.simpleMesh = malloc(sizeof(SimpleMesh));
    self.simpleMesh->vertex_count = 3;
    self.simpleMesh->index_count = 3;
    
    self.simpleMesh->vertices = malloc(sizeof(SimpleVertex) * 3);
    self.simpleMesh->indices = malloc(sizeof(unsigned int) * 3);
    
    // Triangle vertices
    self.simpleMesh->vertices[0] = (SimpleVertex){{0.0f, 0.5f, 0.0f}, {1.0f, 0.0f, 0.0f}, {0.0f, 0.0f, 1.0f}, {0.5f, 0.0f}};
    self.simpleMesh->vertices[1] = (SimpleVertex){{-0.5f, -0.5f, 0.0f}, {0.0f, 1.0f, 0.0f}, {0.0f, 0.0f, 1.0f}, {0.0f, 1.0f}};
    self.simpleMesh->vertices[2] = (SimpleVertex){{0.5f, -0.5f, 0.0f}, {0.0f, 0.0f, 1.0f}, {0.0f, 0.0f, 1.0f}, {1.0f, 1.0f}};
    
    // Triangle indices
    self.simpleMesh->indices[0] = 0;
    self.simpleMesh->indices[1] = 1;
    self.simpleMesh->indices[2] = 2;
    
    self.vertexCount = self.simpleMesh->vertex_count;
    self.indexCount = self.simpleMesh->index_count;
    
    NSLog(@"Created simple triangle with %d vertices and %d indices", self.vertexCount, self.indexCount);
    
    // Create vertex buffer
    self.vertexBuffer = [self.device newBufferWithBytes:self.simpleMesh->vertices 
                                               length:self.vertexCount * sizeof(SimpleVertex) 
                                              options:MTLResourceStorageModeShared];
    
    // Create index buffer
    self.indexBuffer = [self.device newBufferWithBytes:self.simpleMesh->indices 
                                              length:self.indexCount * sizeof(unsigned int) 
                                             options:MTLResourceStorageModeShared];
}

- (void)setupMatrices {
    CGSize size = self.bounds.size;
    float aspect = (float)size.width / (float)size.height;

    matrix_float4x4 perspective = {0};
    float fovy = M_PI / 4.0f;
    float f = 1.0f / tanf(fovy * 0.5f);
    perspective.columns[0][0] = f / aspect;
    perspective.columns[1][1] = f;
    perspective.columns[2][2] = -1.0f;
    perspective.columns[2][3] = -1.0f;
    perspective.columns[3][2] = -0.2f;

    self.projectionMatrix = perspective;

    matrix_float4x4 view = {0};
    view.columns[0][0] = 1.0f;
    view.columns[1][1] = 1.0f;
    view.columns[2][2] = 1.0f;
    view.columns[3][2] = -3.0f;
    view.columns[3][3] = 1.0f;
    self.viewMatrix = view;

    self.rotationX = 0.0f;
    self.rotationY = 0.0f;
}

- (void)drawInMTKView:(MTKView *)view {
    @autoreleasepool {
        self.rotationX += 0.01f;
        self.rotationY += 0.015f;

        float cosX = cosf(self.rotationX);
        float sinX = sinf(self.rotationX);
        float cosY = cosf(self.rotationY);
        float sinY = sinf(self.rotationY);

        matrix_float4x4 model = matrix_identity_float4x4;
        model.columns[0][0] = cosY;
        model.columns[0][2] = sinY;
        model.columns[1][0] = sinX * sinY;
        model.columns[1][1] = cosX;
        model.columns[1][2] = -sinX * cosY;
        model.columns[2][0] = -sinY * cosX;
        model.columns[2][1] = sinX;
        model.columns[2][2] = cosX * cosY;
        self.modelMatrix = model;

        struct {
            matrix_float4x4 projection;
            matrix_float4x4 view;
            matrix_float4x4 model;
            vector_float3 lightDir;
            float time;
        } uniforms;
        uniforms.projection = self.projectionMatrix;
        uniforms.view = self.viewMatrix;
        uniforms.model = self.modelMatrix;
        uniforms.lightDir = (vector_float3){1.0f, 1.0f, 1.0f};
        uniforms.time = (float)CFAbsoluteTimeGetCurrent();

        self.uniformBuffer = [self.device newBufferWithBytes:&uniforms
                                                      length:sizeof(uniforms)
                                                     options:MTLResourceStorageModeShared];

        id<CAMetalDrawable> drawable = [self currentDrawable];
        if (!drawable) return;

        MTLRenderPassDescriptor *renderPass = [MTLRenderPassDescriptor renderPassDescriptor];
        renderPass.colorAttachments[0].texture = drawable.texture;
        renderPass.colorAttachments[0].clearColor = MTLClearColorMake(0.1, 0.2, 0.4, 1.0);
        renderPass.colorAttachments[0].loadAction = MTLLoadActionClear;
        renderPass.colorAttachments[0].storeAction = MTLStoreActionStore;

        id<MTLCommandQueue> commandQueue = [self.device newCommandQueue];
        id<MTLCommandBuffer> commandBuffer = [commandQueue commandBuffer];
        id<MTLRenderCommandEncoder> renderEncoder = [commandBuffer renderCommandEncoderWithDescriptor:renderPass];

        [renderEncoder setRenderPipelineState:self.pipelineState];
        
        // Set vertex and index buffers from our simple mesh
        [renderEncoder setVertexBuffer:self.vertexBuffer offset:0 atIndex:0];
        [renderEncoder setVertexBuffer:self.uniformBuffer offset:0 atIndex:1];
        [renderEncoder setFragmentBuffer:self.uniformBuffer offset:0 atIndex:0];

        [renderEncoder drawIndexedPrimitives:MTLPrimitiveTypeTriangle
                                  indexCount:self.indexCount
                                   indexType:MTLIndexTypeUInt32
                                 indexBuffer:self.indexBuffer
                           indexBufferOffset:0];
        [renderEncoder endEncoding];

        [commandBuffer presentDrawable:drawable];
        [commandBuffer commit];
    }
}

- (void)mtkView:(MTKView *)view drawableSizeWillChange:(CGSize)size {
    [self setupMatrices];
}

- (void)dealloc {
    if (self.simpleMesh) {
        free(self.simpleMesh->vertices);
        free(self.simpleMesh->indices);
        free(self.simpleMesh);
    }
}

@end

@interface AppDelegate : NSObject <NSApplicationDelegate, NSWindowDelegate>
@property (nonatomic, strong) NSWindow *window;
@property (nonatomic, strong) MetalView *metalView;
@end

@implementation AppDelegate

- (BOOL)applicationShouldTerminateAfterLastWindowClosed:(NSApplication *)sender {
    return YES;
}

- (void)applicationDidFinishLaunching:(NSNotification *)notification {
    NSRect screenRect = [[NSScreen mainScreen] frame];
    NSRect windowRect = NSMakeRect(0, 0, 1280, 720);
    windowRect.origin.x = (screenRect.size.width - windowRect.size.width) / 2;
    windowRect.origin.y = (screenRect.size.height - windowRect.size.height) / 2;

    self.window = [[NSWindow alloc] initWithContentRect:windowRect
                                               styleMask:NSWindowStyleMaskTitled | NSWindowStyleMaskClosable | NSWindowStyleMaskMiniaturizable | NSWindowStyleMaskResizable
                                                 backing:NSBackingStoreBuffered
                                                   defer:NO];
    self.window.title = @"Advanced Game Engine - Geometry System";
    self.window.delegate = self;
    self.window.backgroundColor = [NSColor blackColor];

    self.metalView = [[MetalView alloc] initWithFrame:windowRect];
    self.metalView.preferredFramesPerSecond = 60;

    [self.window setContentView:self.metalView];
    [self.window makeKeyAndOrderFront:nil];
    [NSApp activateIgnoringOtherApps:YES];
}

@end

int main(int argc, char *argv[]) {
    @autoreleasepool {
        NSApplication *app = [NSApplication sharedApplication];
        AppDelegate *delegate = [[AppDelegate alloc] init];
        [app setDelegate:delegate];
        [app run];
    }
    return 0;
}

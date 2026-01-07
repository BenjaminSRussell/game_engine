#import <Cocoa/Cocoa.h>
#import <Metal/Metal.h>
#import <MetalKit/MetalKit.h>
#import <QuartzCore/QuartzCore.h>
#include <simd/simd.h>
#include <stdio.h>
#include <stdlib.h>
#include <math.h>

typedef struct {
    vector_float3 position;
    vector_float3 color;
    vector_float3 normal;
} Vertex;

@interface MetalView : MTKView
@property (nonatomic, strong) id<MTLRenderPipelineState> pipelineState;
@property (nonatomic, strong) id<MTLBuffer> vertexBuffer;
@property (nonatomic, strong) id<MTLBuffer> uniformBuffer;
@property (nonatomic, assign) matrix_float4x4 projectionMatrix;
@property (nonatomic, assign) matrix_float4x4 viewMatrix;
@property (nonatomic, assign) matrix_float4x4 modelMatrix;
@property (nonatomic, assign) float rotationX;
@property (nonatomic, assign) float rotationY;
@property (nonatomic, assign) int vertexCount;
@end

@implementation MetalView

- (instancetype)initWithFrame:(CGRect)frame {
    self = [super initWithFrame:frame device:MTLCreateSystemDefaultDevice()];
    if (self) {
        self.delegate = self;
        [self setupMetal];
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
        [self createDefaultShaders];
        library = [device newDefaultLibrary];
        vertexFunction = [library newFunctionWithName:@"vertex_main"];
        fragmentFunction = [library newFunctionWithName:@"fragment_main"];
    }

    MTLRenderPipelineDescriptor *pipelineDescriptor = [[MTLRenderPipelineDescriptor alloc] init];
    pipelineDescriptor.vertexFunction = vertexFunction;
    pipelineDescriptor.fragmentFunction = fragmentFunction;
    pipelineDescriptor.colorAttachments[0].pixelFormat = self.colorPixelFormat;
    pipelineDescriptor.depthAttachmentPixelFormat = MTLPixelFormatDepth32Float;

    NSError *error = nil;
    self.pipelineState = [device newRenderPipelineStateWithDescriptor:pipelineDescriptor error:&error];
    if (!self.pipelineState) {
        NSLog(@"Pipeline creation error: %@", error);
    }

    [self createGeometry];
    [self setupMatrices];
}

- (void)createDefaultShaders {
    id<MTLDevice> device = self.device;

    NSString *shaderSource = @"#include <metal_stdlib>\n"
    "using namespace metal;\n"
    "\n"
    "struct Vertex {\n"
    "    float3 position [[attribute(0)]];\n"
    "    float3 color [[attribute(1)]];\n"
    "    float3 normal [[attribute(2)]];\n"
    "};\n"
    "\n"
    "struct Fragment {\n"
    "    float4 position [[position]];\n"
    "    float3 color;\n"
    "    float3 normal;\n"
    "    float3 worldPos;\n"
    "};\n"
    "\n"
    "struct Uniforms {\n"
    "    float4x4 projection;\n"
    "    float4x4 view;\n"
    "    float4x4 model;\n"
    "    float3 lightDir;\n"
    "};\n"
    "\n"
    "vertex Fragment vertex_main(Vertex in [[stage_in]],\n"
    "                           constant Uniforms& uniforms [[buffer(0)]]) {\n"
    "    Fragment out;\n"
    "    float4 worldPos = uniforms.model * float4(in.position, 1.0);\n"
    "    out.position = uniforms.projection * uniforms.view * worldPos;\n"
    "    out.color = in.color;\n"
    "    out.normal = normalize((uniforms.model * float4(in.normal, 0.0)).xyz);\n"
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
    "    return float4(ambient + diffuse, 1.0);\n"
    "}\n";

    NSError *error = nil;
    id<MTLLibrary> library = [device newLibraryWithSource:shaderSource options:nil error:&error];
    if (!library) {
        NSLog(@"Shader compilation error: %@", error);
    }
}

- (void)createGeometry {
    id<MTLDevice> device = self.device;

    Vertex vertices[] = {
        {{-0.5, -0.5, -0.5}, {1.0, 0.0, 0.0}, {-1.0, 0.0, 0.0}},
        {{ 0.5, -0.5, -0.5}, {1.0, 0.0, 0.0}, {-1.0, 0.0, 0.0}},
        {{ 0.5,  0.5, -0.5}, {1.0, 0.0, 0.0}, {-1.0, 0.0, 0.0}},
        {{-0.5,  0.5, -0.5}, {1.0, 0.0, 0.0}, {-1.0, 0.0, 0.0}},

        {{-0.5, -0.5,  0.5}, {0.0, 1.0, 0.0}, { 1.0, 0.0, 0.0}},
        {{ 0.5, -0.5,  0.5}, {0.0, 1.0, 0.0}, { 1.0, 0.0, 0.0}},
        {{ 0.5,  0.5,  0.5}, {0.0, 1.0, 0.0}, { 1.0, 0.0, 0.0}},
        {{-0.5,  0.5,  0.5}, {0.0, 1.0, 0.0}, { 1.0, 0.0, 0.0}},

        {{-0.5, -0.5, -0.5}, {0.0, 0.0, 1.0}, { 0.0,-1.0, 0.0}},
        {{ 0.5, -0.5, -0.5}, {0.0, 0.0, 1.0}, { 0.0,-1.0, 0.0}},
        {{ 0.5, -0.5,  0.5}, {0.0, 0.0, 1.0}, { 0.0,-1.0, 0.0}},
        {{-0.5, -0.5,  0.5}, {0.0, 0.0, 1.0}, { 0.0,-1.0, 0.0}},
    };

    self.vertexBuffer = [device newBufferWithBytes:vertices
                                           length:sizeof(vertices)
                                          options:MTLResourceStorageModeShared];
    self.vertexCount = 36;
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

        matrix_float4x4 model = {0};
        model.columns[0] = {cosY, 0, sinY, 0};
        model.columns[1] = {sinX * sinY, cosX, -sinX * cosY, 0};
        model.columns[2] = {-sinY * cosX, sinX, cosX * cosY, 0};
        model.columns[3] = {0, 0, 0, 1};
        self.modelMatrix = model;

        struct {
            matrix_float4x4 projection;
            matrix_float4x4 view;
            matrix_float4x4 model;
            vector_float3 lightDir;
        } uniforms = {
            self.projectionMatrix,
            self.viewMatrix,
            self.modelMatrix,
            {1.0f, 1.0f, 1.0f}
        };

        self.uniformBuffer = [self.device newBufferWithBytes:&uniforms
                                                      length:sizeof(uniforms)
                                                     options:MTLResourceStorageModeShared];

        id<CAMetalDrawable> drawable = [self currentDrawable];
        MTLRenderPassDescriptor *renderPass = [MTLRenderPassDescriptor renderPassDescriptor];
        renderPass.colorAttachments[0].texture = drawable.texture;
        renderPass.colorAttachments[0].clearColor = MTLClearColorMake(0.2, 0.3, 0.5, 1.0);
        renderPass.colorAttachments[0].loadAction = MTLLoadActionClear;
        renderPass.colorAttachments[0].storeAction = MTLStoreActionStore;

        id<MTLCommandBuffer> commandBuffer = [self.commandQueue commandBuffer];
        id<MTLRenderCommandEncoder> renderEncoder = [commandBuffer renderCommandEncoderWithDescriptor:renderPass];

        [renderEncoder setRenderPipelineState:self.pipelineState];
        [renderEncoder setVertexBuffer:self.vertexBuffer offset:0 atIndex:0];
        [renderEncoder setVertexBuffer:self.uniformBuffer offset:0 atIndex:1];
        [renderEncoder setFragmentBuffer:self.uniformBuffer offset:0 atIndex:0];

        [renderEncoder drawPrimitives:MTLPrimitiveTypeTriangle vertexStart:0 vertexCount:self.vertexCount];
        [renderEncoder endEncoding];

        [commandBuffer presentDrawable:drawable];
        [commandBuffer commit];
    }
}

- (void)mtkView:(MTKView *)view drawableSizeWillChange:(CGSize)size {
    [self setupMatrices];
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
    self.window.title = @"Sample Game Engine";
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

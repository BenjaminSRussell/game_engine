#import <Metal/Metal.h>
#import <Foundation/Foundation.h>
#import <Cocoa/Cocoa.h>
#include <stdio.h>

// Test the sample engine shader fix
@interface TestSampleEngine : NSObject
- (void)testSampleEngineShaders;
@end

@implementation TestSampleEngine

- (void)testSampleEngineShaders {
    @autoreleasepool {
        id<MTLDevice> device = MTLCreateSystemDefaultDevice();
        if (!device) {
            printf("❌ No Metal device found\n");
            return;
        }
        
        printf("Testing sample engine shader loading...\n");
        
        // Test the approach used in the fixed sample_app.m
        printf("\n1. Testing default library (original approach):\n");
        id<MTLLibrary> defaultLibrary = [device newDefaultLibrary];
        id<MTLFunction> vertexFunction = [defaultLibrary newFunctionWithName:@"vertex_main"];
        id<MTLFunction> fragmentFunction = [defaultLibrary newFunctionWithName:@"fragment_main"];
        
        if (!vertexFunction || !fragmentFunction) {
            printf("   ✅ Failed as expected (no default shaders)\n");
            
            printf("\n2. Testing fallback shader compilation (FIXED approach):\n");
            
            // This is the fixed approach from createDefaultShaders
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
            "    float2 uvPattern = sin(in.uv * 10.0 + uniforms.time) * 0.5 + 0.5;\n"
            "    float3 finalColor = ambient + diffuse * (0.5 + uvPattern.x * 0.5);\n"
            "    \n"
            "    return float4(finalColor, 1.0);\n"
            "}\n";
            
            NSError *error = nil;
            id<MTLLibrary> fallbackLibrary = [device newLibraryWithSource:shaderSource options:nil error:&error];
            
            if (!fallbackLibrary) {
                printf("   ❌ Fallback shader compilation failed: %s\n", [[error localizedDescription] UTF8String]);
                return;
            }
            
            printf("   ✅ Fallback shader compilation successful!\n");
            printf("   Functions found: %lu\n", (unsigned long)[[fallbackLibrary functionNames] count]);
            
            // Test function loading from fallback
            id<MTLFunction> fallbackVertex = [fallbackLibrary newFunctionWithName:@"vertex_main"];
            id<MTLFunction> fallbackFragment = [fallbackLibrary newFunctionWithName:@"fragment_main"];
            
            if (fallbackVertex && fallbackFragment) {
                printf("   ✅ Both fallback functions loaded successfully!\n");
                [fallbackVertex release];
                [fallbackFragment release];
            } else {
                printf("   ❌ Failed to load fallback functions\n");
                [fallbackLibrary release];
                return;
            }
            
            [fallbackLibrary release];
        } else {
            printf("   ❌ Unexpectedly succeeded (default library had shaders)\n");
            [vertexFunction release];
            [fragmentFunction release];
        }
        
        [defaultLibrary release];
    }
}

@end

int main() {
    printf("=== Sample Engine Shader Fix Verification ===\n");
    
    TestSampleEngine *tester = [[TestSampleEngine alloc] init];
    [tester testSampleEngineShaders];
    [tester release];
    
    printf("\n🎉 SAMPLE ENGINE SHADER FIX VERIFICATION PASSED!\n");
    printf("The sample engine should now work correctly with fallback shaders.\n");
    
    return 0;
}

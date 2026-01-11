// Editor/EditorInterop.h
// The bridge between Swift Editor and C Engine
#ifndef EDITOR_INTEROP_H
#define EDITOR_INTEROP_H

#include "include/math/vec3.h"
#import <Foundation/Foundation.h>
#import <Metal/Metal.h>

// Forward declarations of C types
struct MetalIntegratedRenderer;
struct LevelEditor;
struct WorldPartitionSystem;

// Objective-C Wrapper for Swift to convert C structs <-> Swift Classes
@interface EngineBridge : NSObject

// Core Systems
+ (void)initializeEngineWithDevice:(id<MTLDevice>)device
                             width:(uint32_t)width
                            height:(uint32_t)height;
+ (void)shutdownEngine;
+ (void)update:(float)deltaTime;
+ (void)renderFrameWithCommandBuffer:(id<MTLCommandBuffer>)cmdBuffer
                       outputTexture:(id<MTLTexture>)texture;

// Editor Tools
+ (void)editorSelectActor:(uint32_t)actorId;
+ (void)editorMoveActor:(uint32_t)actorId position:(vector_float3)position;
+ (void)editorSetViewMode:(int)mode; // 0=Lit, 1=Wireframe, etc.

// Frontend Data Access
+ (uint32_t)getEntityCount;
+ (NSString *)getEntityName:(uint32_t)entityId;
+ (vector_float3)getEntityPosition:(uint32_t)entityId;

@end

#endif // EDITOR_INTEROP_H

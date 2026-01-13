/**
 * =================================================================================================
 *                          METAL FAST I/O (DirectStorage-style)
 * =================================================================================================
 *
 * Handles asynchronous file loading directly to GPU memory where supported.
 * uses dispatch_io for high-throughput streaming.
 *
 * =================================================================================================
 */

#import <Foundation/Foundation.h>
#import <Metal/Metal.h>
#import <dispatch/dispatch.h>

// I/O operation structure
typedef struct {
    dispatch_io_t ioChannel;
    dispatch_queue_t ioQueue;
    id<MTLBuffer> metalBuffer;
    id<MTLTexture> metalTexture;
    void *destinationBuffer;
    size_t offset;
    size_t size;
    int priority;
    bool isCompressed;
    bool isSparse;
    void(^completion)(bool success, size_t bytesTransferred);
} IOOperation;

// I/O priority management
static NSMutableArray<IOOperation *> *g_activeOperations = nil;
static dispatch_queue_t g_ioQueue = nil;
static dispatch_queue_t g_highPriorityQueue = nil;
static dispatch_queue_t g_lowPriorityQueue = nil;

// Memory mapping for large assets
static NSMutableDictionary<NSString *, void *> *g_mappedFiles = nil;
static NSMutableDictionary<NSString *, size_t> *g_mappedSizes = nil;

// Initialize Metal I/O system
void metal_io_init(id<MTLDevice> device) {
    if (g_activeOperations == nil) {
        g_activeOperations = [[NSMutableArray alloc] init];
    }
    if (g_mappedFiles == nil) {
        g_mappedFiles = [[NSMutableDictionary alloc] init];
    }
    if (g_mappedSizes == nil) {
        g_mappedSizes = [[NSMutableDictionary alloc] init];
    }
    
    g_ioQueue = dispatch_queue_create("com.game.metalio", DISPATCH_QUEUE_CONCURRENT);
    g_highPriorityQueue = dispatch_queue_create("com.game.metalio.high", DISPATCH_QUEUE_SERIAL);
    g_lowPriorityQueue = dispatch_queue_create("com.game.metalio.low", DISPATCH_QUEUE_SERIAL);
    
    // Set target queues for priority management
    dispatch_set_target_queue(g_highPriorityQueue, 
                             dispatch_get_global_queue(DISPATCH_QUEUE_PRIORITY_HIGH, 0));
    dispatch_set_target_queue(g_lowPriorityQueue, 
                             dispatch_get_global_queue(DISPATCH_QUEUE_PRIORITY_LOW, 0));
}

// dispatch_io based loader
void metal_io_load_file_async(const char *filePath, id<MTLBuffer> buffer, size_t offset, size_t size, 
                             int priority, void(^completion)(bool success, size_t bytesTransferred)) {
    NSString *nsPath = [NSString stringWithUTF8String:filePath];
    
    IOOperation *operation = malloc(sizeof(IOOperation));
    operation->metalBuffer = buffer;
    operation->offset = offset;
    operation->size = size;
    operation->priority = priority;
    operation->completion = completion;
    
    // Choose queue based on priority
    dispatch_queue_t targetQueue = (priority > 5) ? g_highPriorityQueue : g_lowPriorityQueue;
    
    dispatch_async(targetQueue, ^{
        int fd = open(filePath, O_RDONLY);
        if (fd == -1) {
            completion(false, 0);
            free(operation);
            return;
        }
        
        operation->ioChannel = dispatch_io_create(DISPATCH_IO_STREAM, fd, g_ioQueue, ^(int error) {
            close(fd);
        });
        
        if (!operation->ioChannel) {
            close(fd);
            completion(false, 0);
            free(operation);
            return;
        }
        
        // Map Metal buffer for direct I/O
        void *bufferPtr = [buffer contents];
        void *targetPtr = (char *)bufferPtr + offset;
        
        dispatch_io_read(operation->ioChannel, 0, size, g_ioQueue, ^(bool done, dispatch_data_t data, int error) {
            if (error == 0 && data) {
                const void *bytes = dispatch_data_get_byte_ptr(data);
                size_t dataSize = dispatch_data_get_size(data);
                
                memcpy(targetPtr, bytes, dataSize);
                
                if (done) {
                    completion(true, size);
                    free(operation);
                }
            } else if (error != 0) {
                completion(false, 0);
                free(operation);
            }
        });
    });
}

// Texture streaming pipeline
void metal_io_stream_texture(const char *filePath, id<MTLTexture> texture, int mipLevel, 
                            int priority, void(^completion)(bool success)) {
    NSString *nsPath = [NSString stringWithUTF8String:filePath];
    
    // Get texture dimensions for this mip level
    NSUInteger width = [texture width] >> mipLevel;
    NSUInteger height = [texture height] >> mipLevel;
    NSUInteger bytesPerRow = width * 4; // Assuming RGBA8
    NSUInteger totalSize = bytesPerRow * height;
    
    // Create staging buffer
    id<MTLDevice> device = [texture device];
    id<MTLBuffer> stagingBuffer = [device newBufferWithLength:totalSize options:MTLResourceStorageModeShared];
    
    metal_io_load_file_async(filePath, stagingBuffer, 0, totalSize, priority, ^(bool success, size_t bytesTransferred) {
        if (success) {
            // Copy from staging buffer to texture
            id<MTLCommandQueue> commandQueue = [device newCommandQueue];
            id<MTLCommandBuffer> commandBuffer = [commandQueue commandBuffer];
            id<MTLBlitCommandEncoder> blitEncoder = [commandBuffer blitCommandEncoder];
            
            [blitEncoder copyFromBuffer:stagingBuffer
                             sourceOffset:0
                        sourceBytesPerRow:bytesPerRow
                      sourceBytesPerImage:totalSize
                               sourceSize:MTLSizeMake(width, height, 1)
                                toTexture:texture
                         destinationSlice:0
                         destinationLevel:mipLevel
                        destinationOrigin:MTLOriginMake(0, 0, 0)];
            
            [blitEncoder endEncoding];
            [commandBuffer commit];
            
            [commandBuffer addCompletedHandler:^(id<MTLCommandBuffer> buffer) {
                completion(true);
            }];
        } else {
            completion(false);
        }
    });
}

// Memory mapping fallback
void *metal_io_map_file(const char *filePath, size_t *outSize) {
    NSString *nsPath = [NSString stringWithUTF8String:filePath];
    
    // Check if already mapped
    void *mapped = g_mappedFiles[nsPath];
    if (mapped) {
        if (outSize) *outSize = [g_mappedSizes[nsPath] unsignedLongLongValue];
        return mapped;
    }
    
    int fd = open(filePath, O_RDONLY);
    if (fd == -1) return NULL;
    
    struct stat st;
    if (fstat(fd, &st) == -1) {
        close(fd);
        return NULL;
    }
    
    size_t size = st.st_size;
    void *addr = mmap(NULL, size, PROT_READ, MAP_PRIVATE, fd, 0);
    close(fd);
    
    if (addr == MAP_FAILED) return NULL;
    
    // Cache the mapping
    g_mappedFiles[nsPath] = [NSValue valueWithPointer:addr];
    g_mappedSizes[nsPath] = @(size);
    
    if (outSize) *outSize = size;
    return addr;
}

void metal_io_unmap_file(const char *filePath) {
    NSString *nsPath = [NSString stringWithUTF8String:filePath];
    NSValue *value = g_mappedFiles[nsPath];
    
    if (value) {
        void *addr = [value pointerValue];
        size_t size = [g_mappedSizes[nsPath] unsignedLongLongValue];
        
        munmap(addr, size);
        [g_mappedFiles removeObjectForKey:nsPath];
        [g_mappedSizes removeObjectForKey:nsPath];
    }
}

// I/O priority management
void metal_io_throttle_if_needed() {
    // Count active operations by priority
    int highPriorityCount = 0;
    int lowPriorityCount = 0;
    
    for (IOOperation *op in g_activeOperations) {
        if (op->priority > 5) {
            highPriorityCount++;
        } else {
            lowPriorityCount++;
        }
    }
    
    // If too many high priority operations, throttle low priority
    if (highPriorityCount > 3 && lowPriorityCount > 0) {
        dispatch_suspend(g_lowPriorityQueue);
        
        // Resume after a short delay
        dispatch_after(dispatch_time(DISPATCH_TIME_NOW, (int64_t)(0.016 * NSEC_PER_SEC)), g_ioQueue, ^{
            dispatch_resume(g_lowPriorityQueue);
        });
    }
}

// Compressed asset handling (LZFSE)
void metal_io_load_compressed_asset(const char *filePath, id<MTLBuffer> buffer, 
                                   size_t compressedSize, size_t decompressedSize,
                                   void(^completion)(bool success)) {
    // Load compressed data
    metal_io_load_file_async(filePath, buffer, 0, compressedSize, 5, ^(bool success, size_t bytesTransferred) {
        if (!success) {
            completion(false);
            return;
        }
        
        // Decompress in-place (simplified - would use LZFSE framework)
        void *compressedPtr = [buffer contents];
        void *decompressedPtr = malloc(decompressedSize);
        
        // This would use LZFSE decompression
        // size_t result = lzfse_decode_buffer(decompressedPtr, decompressedSize, compressedPtr, compressedSize, NULL);
        
        // For now, just copy (placeholder)
        memcpy(decompressedPtr, compressedPtr, decompressedSize);
        
        // Copy back to buffer
        memcpy(compressedPtr, decompressedPtr, decompressedSize);
        free(decompressedPtr);
        
        completion(true);
    });
}

// Sparse file support for virtual texturing
void metal_io_load_sparse_texture_page(const char *filePath, id<MTLTexture> texture, 
                                       int pageX, int pageY, int pageSize, 
                                       void(^completion)(bool success)) {
    // Calculate offset for this page in the sparse file
    size_t pageOffset = (pageY * pageSize + pageX) * pageSize * pageSize * 4; // RGBA8
    size_t pageSizeBytes = pageSize * pageSize * 4;
    
    // Create staging buffer for this page
    id<MTLDevice> device = [texture device];
    id<MTLBuffer> stagingBuffer = [device newBufferWithLength:pageSizeBytes options:MTLResourceStorageModeShared];
    
    metal_io_load_file_async(filePath, stagingBuffer, pageOffset, pageSizeBytes, 3, ^(bool success, size_t bytesTransferred) {
        if (success) {
            // Copy page to texture region
            id<MTLCommandQueue> commandQueue = [device newCommandQueue];
            id<MTLCommandBuffer> commandBuffer = [commandQueue commandBuffer];
            id<MTLBlitCommandEncoder> blitEncoder = [commandBuffer blitCommandEncoder];
            
            [blitEncoder copyFromBuffer:stagingBuffer
                             sourceOffset:0
                        sourceBytesPerRow:pageSize * 4
                      sourceBytesPerImage:pageSizeBytes
                               sourceSize:MTLSizeMake(pageSize, pageSize, 1)
                                toTexture:texture
                         destinationSlice:0
                         destinationLevel:0
                        destinationOrigin:MTLOriginMake(pageX * pageSize, pageY * pageSize, 0)];
            
            [blitEncoder endEncoding];
            [commandBuffer commit];
            
            [commandBuffer addCompletedHandler:^(id<MTLCommandBuffer> buffer) {
                completion(true);
            }];
        } else {
            completion(false);
        }
    });
}

// Cleanup
void metal_io_shutdown() {
    // Unmap all files
    for (NSString *path in g_mappedFiles.allKeys) {
        metal_io_unmap_file([path UTF8String]);
    }
    
    [g_mappedFiles removeAllObjects];
    [g_mappedSizes removeAllObjects];
    [g_activeOperations removeAllObjects];
    
    g_ioQueue = nil;
    g_highPriorityQueue = nil;
    g_lowPriorityQueue = nil;
}

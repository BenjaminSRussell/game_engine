#ifndef ENGINE_AI_CONTENT_GENERATION_API_H
#define ENGINE_AI_CONTENT_GENERATION_API_H

#include <stddef.h>
#include <stdbool.h>
#include <stdint.h>

typedef enum {
    CONTENT_TYPE_TEXTURE,
    CONTENT_TYPE_MODEL,
    CONTENT_TYPE_AUDIO,
    CONTENT_TYPE_TEXT
} ContentType;

typedef struct {
    ContentType type;
    const char* prompt;
    const char* style_ref; // Optional path to reference style
    void* user_data;
} ContentGenerationRequest;

typedef enum {
    GENERATION_STATUS_PENDING,
    GENERATION_STATUS_PROCESSING,
    GENERATION_STATUS_COMPLETED,
    GENERATION_STATUS_FAILED
} GenerationStatus;

typedef struct {
    uint64_t request_id;
    GenerationStatus status;
    const char* result_path; // Path to generated asset file
    const char* error_message;
    float progress; // 0.0 to 1.0
} GenerationResult;

// Callback type
typedef void (*GenerationCallback)(const GenerationResult* result);

// --- API ---

/**
 * Initialize the content generation subsystem.
 */
void ai_content_init(void);

/**
 * Request content generation.
 * Returns a request ID (handle) or 0 on failure.
 * The callback will be invoked on completion (success or failure).
 */
uint64_t ai_content_request_generation(const ContentGenerationRequest* request, GenerationCallback callback);

/**
 * Poll for status of a specific request.
 */
GenerationStatus ai_content_get_status(uint64_t request_id);

/**
 * Cancel a pending request.
 */
void ai_content_cancel_request(uint64_t request_id);

#endif // ENGINE_AI_CONTENT_GENERATION_API_H

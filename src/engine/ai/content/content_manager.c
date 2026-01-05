#include "generation_api.h"
#include <stdlib.h>
#include <string.h>
#include <stdio.h>

// Mock implementation for the content manager (Phase 1 Stub)

static uint64_t next_request_id = 1;
static bool initialized = false;

// Simulated store
#define MAX_REQUESTS 64
static GenerationResult requests[MAX_REQUESTS];
static int request_count = 0;

void ai_content_init(void) {
    if (initialized) return;
    memset(requests, 0, sizeof(requests));
    request_count = 0;
    initialized = true;
    printf("[AI Content] Subsystem initialized.\n");
}

uint64_t ai_content_request_generation(const ContentGenerationRequest* request, GenerationCallback callback) {
    if (!initialized) ai_content_init();
    if (request_count >= MAX_REQUESTS) return 0;
    
    uint64_t id = next_request_id++;
    
    // Store request (simplistic)
    int idx = -1;
    for (int i = 0; i < MAX_REQUESTS; i++) {
        if (requests[i].request_id == 0) {
            idx = i;
            break;
        }
    }
    
    if (idx == -1) return 0;
    
    requests[idx].request_id = id;
    requests[idx].status = GENERATION_STATUS_PENDING;
    requests[idx].progress = 0.0f;
    
    // In a real system, we'd dispatch a job.
    // Here we just log it.
    printf("[AI Content] Request %llu accepted: Type=%d, Prompt='%s'\n", 
        (unsigned long long)id, request->type, request->prompt);
        
    return id;
}

GenerationStatus ai_content_get_status(uint64_t request_id) {
    for (int i = 0; i < MAX_REQUESTS; i++) {
        if (requests[i].request_id == request_id) {
            return requests[i].status;
        }
    }
    return GENERATION_STATUS_FAILED;
}

void ai_content_cancel_request(uint64_t request_id) {
    for (int i = 0; i < MAX_REQUESTS; i++) {
        if (requests[i].request_id == request_id) {
            requests[i].status = GENERATION_STATUS_FAILED;
            requests[i].error_message = "Cancelled by user";
            printf("[AI Content] Request %llu cancelled.\n", (unsigned long long)request_id);
            return;
        }
    }
}

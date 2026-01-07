#include "networking/bandwidth/bandwidth_throttler.h"
#include <string.h>

typedef enum { PRIORITY_CRITICAL, PRIORITY_HIGH, PRIORITY_MEDIUM, PRIORITY_LOW } PacketPriority;

typedef struct {
    float tokens;           // Token bucket
    float tokens_per_sec;   // Refill rate
    float max_tokens;
    uint64_t bytes_sent;
    uint64_t bytes_dropped;
} BandwidthLimiter;

static BandwidthLimiter g_limiter = {
    .tokens = 1000.0f,
    .tokens_per_sec = 1000.0f,  // 1KB/s
    .max_tokens = 2000.0f
};

void bandwidth_throttler_init(float rate_kbps) {
    g_limiter.tokens_per_sec = rate_kbps * 1024.0f;
    g_limiter.max_tokens = g_limiter.tokens_per_sec * 2.0f;
    g_limiter.tokens = g_limiter.max_tokens;
}

void bandwidth_throttler_update(float dt) {
    // Refill token bucket
    g_limiter.tokens += g_limiter.tokens_per_sec * dt;
    if (g_limiter.tokens > g_limiter.max_tokens) {
        g_limiter.tokens = g_limiter.max_tokens;
    }
}

int bandwidth_can_send(int packet_size, PacketPriority priority) {
    // Critical always goes through
    if (priority == PRIORITY_CRITICAL) return 1;
    
    // Check token bucket
    if (g_limiter.tokens >= packet_size) {
        g_limiter.tokens -= packet_size;
        g_limiter.bytes_sent += packet_size;
        return 1;
    }
    
    // Drop low priority when bandwidth limited
    if (priority == PRIORITY_LOW) {
        g_limiter.bytes_dropped += packet_size;
        return 0;
    }
    
    return 0;
}

void bandwidth_get_stats(uint64_t *sent, uint64_t *dropped) {
    *sent = g_limiter.bytes_sent;
    *dropped = g_limiter.bytes_dropped;
}

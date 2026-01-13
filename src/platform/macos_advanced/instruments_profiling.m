/**
 * =================================================================================================
 *                          INSTRUMENTS PROFILING INTEGRATION
 * =================================================================================================
 *
 * Integration with Apple Instruments via os_signpost API.
 * Allows visualization of engine events in Time Profiler / Points of Interest.
 *
 * =================================================================================================
 */

#import <os/signpost.h>

// Signpost log handles for different subsystems
static os_log_t g_renderingLog = nil;
static os_log_t g_physicsLog = nil;
static os_log_t g_aiLog = nil;
static os_log_t g_frameLog = nil;
static os_log_t g_jobLog = nil;
static os_log_t g_waitLog = nil;

// Initialize profiling system
void instruments_init() {
    g_renderingLog = os_log_create("com.game.engine", "Rendering");
    g_physicsLog = os_log_create("com.game.engine", "Physics");
    g_aiLog = os_log_create("com.game.engine", "AI");
    g_frameLog = os_log_create("com.game.engine", "Frame");
    g_jobLog = os_log_create("com.game.engine", "Job");
    g_waitLog = os_log_create("com.game.engine", "Wait");
}

// Zone begin/end macros with color coding
#define INSTRUMENTS_ZONE_BEGIN(log, name, ...) \
    os_signpost_id_t zoneId = os_signpost_id_generate(log); \
    os_signpost_interval_begin(log, zoneId, name, ##__VA_ARGS__)

#define INSTRUMENTS_ZONE_END(log, name, ...) \
    os_signpost_interval_end(log, zoneId, name, ##__VA_ARGS__)

// Colored zone macros
#define INSTRUMENTS_ZONE_BEGIN_RED(log, name) \
    INSTRUMENTS_ZONE_BEGIN(log, name, "color", "red")

#define INSTRUMENTS_ZONE_BEGIN_GREEN(log, name) \
    INSTRUMENTS_ZONE_BEGIN(log, name, "color", "green")

#define INSTRUMENTS_ZONE_BEGIN_BLUE(log, name) \
    INSTRUMENTS_ZONE_BEGIN(log, name, "color", "blue")

#define INSTRUMENTS_ZONE_BEGIN_YELLOW(log, name) \
    INSTRUMENTS_ZONE_BEGIN(log, name, "color", "yellow")

// Event marker emission for instantaneous events
void instruments_emit_event(os_log_t log, const char *eventName, ...) {
    va_list args;
    va_start(args, eventName);
    os_signpost_event_emit(log, os_signpost_id_generate(log), eventName, va_get_string(args));
    va_end(args);
}

// Specific event emitters
void instruments_emit_jump_event(float height, float distance) {
    os_signpost_event_emit(g_physicsLog, os_signpost_id_generate(g_physicsLog), 
                           "Player Jump", "height", height, "distance", distance);
}

void instruments_emit_collision_event(const char *objectA, const char *objectB, float impact) {
    os_signpost_event_emit(g_physicsLog, os_signpost_id_generate(g_physicsLog),
                           "Collision", "obj1", objectA, "obj2", objectB, "impact", impact);
}

void instruments_emit_weapon_fire_event(const char *weaponType, int ammoCount) {
    os_signpost_event_emit(g_gameplayLog, os_signpost_id_generate(g_gameplayLog),
                           "Weapon Fire", "type", weaponType, "ammo", ammoCount);
}

// Frame boundary markers
static uint64_t g_frameCount = 0;

void instruments_mark_frame_begin() {
    os_signpost_interval_begin(g_frameLog, os_signpost_id_generate(g_frameLog), 
                               "Frame", "frame", g_frameCount);
}

void instruments_mark_frame_end() {
    os_signpost_interval_end(g_frameLog, os_signpost_id_generate(g_frameLog), 
                             "Frame", "frame", g_frameCount);
    g_frameCount++;
}

void instruments_mark_frame_boundary() {
    instruments_mark_frame_end();
    instruments_mark_frame_begin();
}

// Job system instrumentation
typedef struct {
    os_signpost_id_t signpostId;
    const char *jobName;
    uint64_t threadId;
} JobTraceData;

static NSMutableDictionary *g_jobTraces = nil;

void instruments_job_start(const char *jobName, void *jobPtr) {
    if (!g_jobTraces) {
        g_jobTraces = [[NSMutableDictionary alloc] init];
    }
    
    JobTraceData *trace = malloc(sizeof(JobTraceData));
    trace->signpostId = os_signpost_id_generate(g_jobLog);
    trace->jobName = jobName;
    trace->threadId = (uint64_t)[NSThread currentThread];
    
    [g_jobTraces setObject:[NSValue valueWithPointer:trace] forKey:[NSValue valueWithPointer:jobPtr]];
    
    os_signpost_interval_begin(g_jobLog, trace->signpostId, 
                               "Job Start", "name", jobName, 
                               "thread", trace->threadId);
}

void instruments_job_end(void *jobPtr) {
    NSValue *key = [NSValue valueWithPointer:jobPtr];
    NSValue *value = [g_jobTraces objectForKey:key];
    
    if (value) {
        JobTraceData *trace = [value pointerValue];
        os_signpost_interval_end(g_jobLog, trace->signpostId, 
                                 "Job End", "name", trace->jobName,
                                 "thread", trace->threadId);
        free(trace);
        [g_jobTraces removeObjectForKey:key];
    }
}

void instruments_job_wait_begin(const char *waitReason) {
    os_signpost_interval_begin(g_waitLog, os_signpost_id_generate(g_waitLog),
                               "Job Wait", "reason", waitReason);
}

void instruments_job_wait_end() {
    os_signpost_interval_end(g_waitLog, os_signpost_id_generate(g_waitLog), "Job Wait");
}

// Convenience macros for common operations
#define INSTRUMENTS_RENDER_ZONE(name) \
    INSTRUMENTS_ZONE_BEGIN(g_renderingLog, name); \
    /* code */ \
    INSTRUMENTS_ZONE_END(g_renderingLog, name)

#define INSTRUMENTS_PHYSICS_ZONE(name) \
    INSTRUMENTS_ZONE_BEGIN(g_physicsLog, name); \
    /* code */ \
    INSTRUMENTS_ZONE_END(g_physicsLog, name)

#define INSTRUMENTS_AI_ZONE(name) \
    INSTRUMENTS_ZONE_BEGIN(g_aiLog, name); \
    /* code */ \
    INSTRUMENTS_ZONE_END(g_aiLog, name)

// Cleanup
void instruments_shutdown() {
    // Clean up any remaining job traces
    if (g_jobTraces) {
        for (NSValue *value in g_jobTraces.allValues) {
            JobTraceData *trace = [value pointerValue];
            free(trace);
        }
        [g_jobTraces removeAllObjects];
        g_jobTraces = nil;
    }
    
    // OS logs don't need explicit cleanup
}

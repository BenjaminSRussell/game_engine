/*
 * asset_render_system skips entities that are hidden or missing an instance.
 * This target compiles that source directly and stubs the ECS query and
 * asset lookup so it does not link Engine.
 *
 * The periodic render log uses a static frame counter, so cases stay in order.
 */

#include "core/asset_manager.h"
#include "core/logging/unified_logger.h"
#include "ecs/components/asset_instance_component.h"
#include "ecs/components/transform.h"
#include "ecs/ecs.h"
#include "ecs/systems/asset_render_system.h"
#include "include/core/resource/asset_instance.h"

#include <stdarg.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#define INFO_CAP 8
#define STUB_ENTITIES 8

static World g_world;
static AssetManager g_manager;
static Asset g_asset;
static Query g_query;

static AssetInstanceComponent *g_assets[STUB_ENTITIES];
static TransformComponent *g_transforms[STUB_ENTITIES];
static int g_entity_count;
static int g_cursor;

static int g_fail_query;
static int g_bad_query;
static int g_creates;
static int g_destroys;
static World *g_create_world;
static World *g_destroy_world;
static Query *g_destroy_query;
static ComponentType g_comp0;
static ComponentType g_comp1;

static AssetInstance *g_instances[16];
static int g_lookups;
static AssetManager *g_lookup_manager;

static int g_info_count;
static int g_error_count;
static char g_infos[INFO_CAP][160];
static char g_errors[4][160];
static int g_info_overflow;

static void fail(const char *msg) {
    fprintf(stderr, "FAIL: %s\n", msg);
    exit(1);
}

static void expect(int cond, const char *msg) {
    if (!cond) {
        fail(msg);
    }
}

void unified_logger_log(LogLevel level, LogCategory category, const char *file,
                        int line, const char *function, const char *format, ...) {
    char buf[160];
    va_list ap;

    (void)file;
    (void)line;
    (void)function;
    expect(category == LOG_CAT_GENERAL, "log category");
    va_start(ap, format);
    vsnprintf(buf, sizeof(buf), format ? format : "", ap);
    va_end(ap);
    if (level == LOG_LEVEL_INFO) {
        if (g_info_count < INFO_CAP) {
            snprintf(g_infos[g_info_count], sizeof(g_infos[0]), "%s", buf);
        } else {
            g_info_overflow = 1;
        }
        g_info_count++;
    } else if (level == LOG_LEVEL_ERROR) {
        if (g_error_count < 4) {
            snprintf(g_errors[g_error_count], sizeof(g_errors[0]), "%s", buf);
        }
        g_error_count++;
    }
}

ECSComponentID asset_instance_component_get_id(World *world) {
    expect(world == &g_world, "asset id world");
    return 41;
}

ECSComponentID transform_component_get_id(World *world) {
    expect(world == &g_world, "transform id world");
    return 42;
}

Query *ecs_query_create(World *world, const QueryDesc *desc) {
    g_creates++;
    g_create_world = world;
    if (!desc || desc->all_count != 2 || !desc->all_components ||
        desc->any_count != 0 || desc->none_count != 0 || desc->any_components ||
        desc->none_components || desc->changed_only) {
        g_bad_query = 1;
    } else {
        g_comp0 = desc->all_components[0];
        g_comp1 = desc->all_components[1];
    }
    if (g_fail_query) {
        return NULL;
    }
    g_cursor = 0;
    return &g_query;
}

void ecs_query_destroy(World *world, Query *query) {
    g_destroys++;
    g_destroy_world = world;
    g_destroy_query = query;
}

bool ecs_query_next(Query *query, Entity *entity, void **components) {
    if (query != &g_query || !entity || !components || g_cursor >= g_entity_count) {
        return false;
    }
    entity->id = (u32)(g_cursor + 1);
    entity->generation = 1;
    components[0] = g_assets[g_cursor];
    components[1] = g_transforms[g_cursor];
    g_cursor++;
    return true;
}

AssetInstance *asset_manager_get_instance(AssetManager *manager, uint32_t instance_id) {
    g_lookups++;
    g_lookup_manager = manager;
    if (instance_id >= 16) {
        return NULL;
    }
    return g_instances[instance_id];
}

static void reset_case(void) {
    memset(g_assets, 0, sizeof(g_assets));
    memset(g_transforms, 0, sizeof(g_transforms));
    memset(g_instances, 0, sizeof(g_instances));
    g_entity_count = 0;
    g_cursor = 0;
    g_fail_query = 0;
    g_creates = 0;
    g_destroys = 0;
    g_create_world = NULL;
    g_destroy_world = NULL;
    g_destroy_query = NULL;
    g_lookups = 0;
    g_lookup_manager = NULL;
    g_info_count = 0;
    g_error_count = 0;
    g_info_overflow = 0;
    memset(g_infos, 0, sizeof(g_infos));
    memset(g_errors, 0, sizeof(g_errors));
}

static int info_is(const char *msg) {
    int i;
    for (i = 0; i < g_info_count && i < INFO_CAP; i++) {
        if (strcmp(g_infos[i], msg) == 0) {
            return 1;
        }
    }
    return 0;
}

static int rendered_logs(void) {
    int i;
    int n = 0;
    for (i = 0; i < g_info_count && i < INFO_CAP; i++) {
        if (strstr(g_infos[i], "Rendered") != NULL) {
            n++;
        }
    }
    return n;
}

static void run_updates(int count) {
    int i;
    for (i = 0; i < count; i++) {
        asset_render_system_update(0.016f);
    }
}

static AssetInstanceComponent comp_with(uint32_t instance_id, uint32_t flags) {
    AssetInstanceComponent comp;
    memset(&comp, 0, sizeof(comp));
    comp.asset_instance_id = instance_id;
    comp.render_flags = flags;
    return comp;
}

static void bind_instance(uint32_t id, Asset *source) {
    static AssetInstance storage[16];
    memset(&storage[id], 0, sizeof(storage[id]));
    storage[id].instance_id = id;
    storage[id].source_asset = source;
    storage[id].is_active = true;
    g_instances[id] = &storage[id];
}

int main(void) {
    AssetInstanceComponent visible;
    AssetInstanceComponent shadowed;
    AssetInstanceComponent shadow_only;
    AssetInstanceComponent missing;
    AssetInstanceComponent empty_source;
    AssetInstanceComponent hidden_slot;
    TransformComponent transforms[6];
    int creates_before;

    memset(&g_asset, 0, sizeof(g_asset));
    memset(transforms, 0, sizeof(transforms));
    strncpy(g_asset.id, "tree", sizeof(g_asset.id) - 1);

    reset_case();
    asset_render_system_update(1.0f);
    expect(g_creates == 0, "update before init queried");
    expect(g_info_count == 0 && g_error_count == 0, "update before init logged");
    printf("skip update before init\n");

    reset_case();
    asset_render_system_init(NULL, &g_manager);
    expect(info_is("[AssetRenderSystem] Initialized"), "null world init log");
    run_updates(1);
    expect(g_creates == 0, "null world queried");
    asset_render_system_shutdown();
    expect(info_is("[AssetRenderSystem] Shutdown"), "shutdown log");
    g_info_count = 0;
    asset_render_system_init(&g_world, NULL);
    run_updates(1);
    expect(g_creates == 0, "null manager queried");
    asset_render_system_shutdown();
    run_updates(1);
    expect(g_creates == 0, "update after shutdown queried");
    printf("skip null world, null manager, and shutdown\n");

    reset_case();
    asset_render_system_init(&g_world, &g_manager);
    g_info_count = 0;
    g_fail_query = 1;
    asset_render_system_update(0.016f);
    expect(g_creates == 1 && g_destroys == 0, "failed query destroyed");
    expect(g_create_world == &g_world, "failed query world");
    expect(g_comp0 == 41 && g_comp1 == 42, "query component ids");
    expect(!g_bad_query, "query description");
    expect(g_error_count == 1, "query failure log count");
    expect(strcmp(g_errors[0], "[AssetRenderSystem] Failed to create query") == 0,
           "query failure log");
    expect(g_lookups == 0, "failed query looked up an instance");
    expect(rendered_logs() == 0, "failed query rendered");
    printf("query create failure\n");

    g_fail_query = 0;
    g_error_count = 0;
    visible = comp_with(1, RENDER_FLAG_VISIBLE);
    bind_instance(1, &g_asset);
    g_assets[0] = &visible;
    g_transforms[0] = &transforms[0];
    g_entity_count = 1;
    g_creates = 0;
    g_destroys = 0;
    g_lookups = 0;
    run_updates(59);
    expect(g_creates == 59 && g_destroys == 59, "59 frame query balance");
    expect(g_destroy_world == &g_world && g_destroy_query == &g_query, "query destroy args");
    expect(g_lookup_manager == &g_manager, "lookup manager");
    expect(g_lookups == 59, "59 frame lookups");
    expect(rendered_logs() == 0, "logged before 60 frames");
    creates_before = g_creates;
    asset_render_system_update(0.016f);
    expect(g_creates == creates_before + 1, "60th frame query");
    expect(rendered_logs() == 1, "60th frame log count");
    expect(info_is("[AssetRenderSystem] Rendered 1 asset instances"), "rendered 1");
    printf("visible instance logs on frame 60\n");

    reset_case();
    visible = comp_with(1, RENDER_FLAG_VISIBLE);
    shadowed = comp_with(2, RENDER_FLAG_VISIBLE | RENDER_FLAG_CAST_SHADOW);
    shadow_only = comp_with(3, RENDER_FLAG_CAST_SHADOW);
    missing = comp_with(9, RENDER_FLAG_VISIBLE);
    empty_source = comp_with(4, RENDER_FLAG_VISIBLE);
    hidden_slot = comp_with(5, RENDER_FLAG_VISIBLE);
    bind_instance(1, &g_asset);
    bind_instance(2, &g_asset);
    bind_instance(3, &g_asset);
    bind_instance(4, NULL);
    g_assets[0] = &visible;
    g_assets[1] = &shadowed;
    g_assets[2] = &shadow_only;
    g_assets[3] = &missing;
    g_assets[4] = &empty_source;
    g_assets[5] = NULL;
    g_transforms[0] = &transforms[0];
    g_transforms[1] = &transforms[1];
    g_transforms[2] = &transforms[2];
    g_transforms[3] = &transforms[3];
    g_transforms[4] = &transforms[4];
    g_transforms[5] = &transforms[5];
    g_entity_count = 7;
    g_assets[6] = &hidden_slot;
    g_transforms[6] = NULL;
    run_updates(60);
    expect(g_creates == 60 && g_destroys == 60, "mixed query balance");
    expect(g_lookups == 60 * 5, "skipped null components");
    expect(g_lookup_manager == &g_manager, "mixed lookup manager");
    expect(rendered_logs() == 1, "mixed render log count");
    expect(info_is("[AssetRenderSystem] Rendered 2 asset instances"), "rendered 2");
    expect(!g_bad_query, "mixed query description");
    printf("counts visible instances only\n");

    reset_case();
    asset_render_system_shutdown();
    asset_render_system_update(0.016f);
    expect(g_creates == 0, "shutdown cleared the world");
    asset_render_system_init(&g_world, &g_manager);
    g_info_count = 0;
    visible = comp_with(1, RENDER_FLAG_VISIBLE);
    bind_instance(1, &g_asset);
    g_assets[0] = &visible;
    g_transforms[0] = &transforms[0];
    g_entity_count = 1;
    run_updates(60);
    expect(info_is("[AssetRenderSystem] Rendered 1 asset instances"), "reinit rendered 1");
    expect(g_lookups == 60, "reinit lookups");
    asset_render_system_shutdown();
    printf("shutdown then reinit\n");

    expect(!g_info_overflow, "info log overflow");
    expect(!g_bad_query, "query description");
    printf("asset_render_system: all cases passed\n");
    return 0;
}

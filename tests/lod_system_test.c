/*
 * lod_system_test.c
 * LOD registration, screen-size selection, bias, and forced level.
 * Compiles lod_system.c directly and does not link Engine.
 * LOD chain generation is stubbed; selection logic stays in lod_system.c.
 */

#include "geometry/lod/lod_system.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

static int g_simple_calls = 0;
static int g_advanced_calls = 0;
static int g_destroy_calls = 0;
static mesh_t g_simple_lods[4];
static mesh_t g_advanced_lods[4];

static lod_chain_t *chain_from_ratios(const mesh_t *source, const float *ratios,
                                      uint32_t count, mesh_t *storage) {
    lod_chain_t *chain;
    uint32_t i;

    if (!source || !ratios || count == 0) {
        return NULL;
    }

    chain = (lod_chain_t *)calloc(1, sizeof(*chain));
    if (!chain) {
        return NULL;
    }

    chain->lods[0] = (mesh_t *)source;
    chain->switch_distances[0] = 1.0f;
    chain->lod_count = 1;
    for (i = 0; i < count && i < 4 && chain->lod_count < 8; i++) {
        if (ratios[i] <= 0.0f || ratios[i] >= 1.0f) {
            continue;
        }
        memset(&storage[i], 0, sizeof(storage[i]));
        storage[i].vertex_count = 100u + i;
        storage[i].index_count = 12u;
        chain->lods[chain->lod_count] = &storage[i];
        chain->switch_distances[chain->lod_count] = ratios[i];
        chain->lod_count++;
    }
    return chain;
}

lod_chain_t *lod_generate_chain(const mesh_t *source, const float *ratios, uint32_t count) {
    g_simple_calls++;
    return chain_from_ratios(source, ratios, count, g_simple_lods);
}

lod_chain_t *lod_generate_chain_advanced(const mesh_t *source,
                                        const lod_generation_config_t *config) {
    g_advanced_calls++;
    if (!source || !config) {
        return NULL;
    }
    return chain_from_ratios(source, config->reduction_ratios, 4, g_advanced_lods);
}

void lod_chain_destroy(lod_chain_t *chain) {
    g_destroy_calls++;
    free(chain);
}

static void fail(const char *msg) {
    fprintf(stderr, "FAIL: %s\n", msg);
    exit(1);
}

static void expect_true(bool cond, const char *msg) {
    if (!cond) {
        fail(msg);
    }
}

static uint32_t g_print_id = 0;

static void print_stats_wrap(void) { lod_system_print_statistics(); }

static void print_mesh_wrap(void) { lod_system_print_mesh_info(g_print_id); }

static void capture_stdout(void (*fn)(void), char *buf, size_t buf_n) {
    int fds[2];
    int saved;
    ssize_t got;

    fflush(stdout);
    if (pipe(fds) != 0) {
        fail("pipe");
    }
    saved = dup(STDOUT_FILENO);
    if (saved < 0 || dup2(fds[1], STDOUT_FILENO) < 0) {
        fail("dup");
    }
    close(fds[1]);
    fn();
    fflush(stdout);
    if (dup2(saved, STDOUT_FILENO) < 0) {
        fail("restore");
    }
    close(saved);
    got = read(fds[0], buf, buf_n - 1);
    if (got < 0) {
        got = 0;
    }
    buf[got] = '\0';
    close(fds[0]);
}

static lod_chain_t *make_chain(mesh_t *lod0, mesh_t *lod1, mesh_t *lod2) {
    lod_chain_t *chain = (lod_chain_t *)calloc(1, sizeof(*chain));
    if (!chain) {
        fail("oom chain");
    }
    chain->lods[0] = lod0;
    chain->lods[1] = lod1;
    chain->lods[2] = lod2;
    chain->switch_distances[0] = 1.0f;
    chain->switch_distances[1] = 0.5f;
    chain->switch_distances[2] = 0.25f;
    chain->lod_count = 3;
    return chain;
}

int main(void) {
    mesh_t hero;
    mesh_t coarse;
    mesh_t coarsest;
    mesh_t prop;
    mesh_t named;
    mesh_t anon;
    mesh_t bulk;
    lod_chain_t *chain;
    lod_chain_t *replacement;
    lod_generation_config_t config;
    char captured[2048];
    char needle[64];
    char long_name[80];
    char truncated[64];
    float ratios[2];
    uint32_t total = 7;
    uint32_t with_lods = 7;
    uint32_t switches = 7;
    uint32_t hero_id;
    uint32_t prop_id;
    uint32_t named_id;
    uint32_t anon_id;
    uint32_t bulk_id = 0;
    uint32_t i;
    int simple_before;
    int advanced_before;
    int destroys_before;

    memset(&hero, 0, sizeof(hero));
    memset(&coarse, 0, sizeof(coarse));
    memset(&coarsest, 0, sizeof(coarsest));
    memset(&prop, 0, sizeof(prop));
    memset(&named, 0, sizeof(named));
    memset(&anon, 0, sizeof(anon));
    memset(&bulk, 0, sizeof(bulk));
    hero.vertex_count = 300;
    hero.index_count = 90;
    coarse.vertex_count = 120;
    coarse.index_count = 36;
    coarsest.vertex_count = 40;
    coarsest.index_count = 12;
    prop.vertex_count = 10;
    ratios[0] = 0.5f;
    ratios[1] = 0.25f;
    memset(&config, 0, sizeof(config));

    expect_true(lod_system_register_mesh(&hero, "hero") == UINT32_MAX, "register before init");
    expect_true(!lod_system_unregister_mesh(0), "unregister before init");
    expect_true(lod_system_select_lod(0, 1.0f) == NULL, "select before init");
    expect_true(lod_system_get_lod_chain(0) == NULL, "chain before init");
    expect_true(!lod_system_generate_lods(0, NULL), "advanced before init");
    expect_true(!lod_system_generate_lods_simple(0, ratios, 1), "simple before init");
    expect_true(!lod_system_set_lod_chain(0, NULL), "set chain before init");
    expect_true(lod_system_get_current_lod(0) == 0, "current before init");
    lod_system_get_statistics(&total, &with_lods, &switches);
    expect_true(total == 7 && with_lods == 7 && switches == 7, "stats unchanged before init");
    capture_stdout(print_stats_wrap, captured, sizeof(captured));
    expect_true(strstr(captured, "LOD System not initialized") != NULL, "print before init");
    lod_system_shutdown();

    expect_true(lod_system_init() == 0, "init");
    expect_true(lod_system_init() == 0, "second init");
    expect_true(lod_system_register_mesh(NULL, "none") == UINT32_MAX, "null mesh");

    hero_id = lod_system_register_mesh(&hero, "hero");
    expect_true(hero_id != UINT32_MAX, "register hero");
    expect_true(lod_system_init() == 0, "init does not reset");
    total = 0;
    with_lods = 9;
    switches = 9;
    lod_system_get_statistics(&total, &with_lods, &switches);
    expect_true(total == 1 && with_lods == 0 && switches == 0, "one mesh no lods");
    lod_system_get_statistics(NULL, NULL, NULL);

    expect_true(lod_system_select_lod(hero_id, 0.9f) == &hero, "no chain returns original");
    expect_true(lod_system_select_lod(999u, 0.9f) == NULL, "unknown select");
    expect_true(lod_system_get_current_lod(hero_id) == 0, "current stays 0");
    expect_true(lod_system_get_lod_chain(hero_id) == NULL, "no chain yet");
    expect_true(!lod_system_set_lod_chain(hero_id, NULL), "null chain");
    expect_true(!lod_system_unregister_mesh(999u), "unknown unregister");

    chain = make_chain(&hero, &coarse, &coarsest);
    destroys_before = g_destroy_calls;
    expect_true(lod_system_set_lod_chain(hero_id, chain), "set chain");
    expect_true(g_destroy_calls == destroys_before, "no prior chain to destroy");
    expect_true(lod_system_get_lod_chain(hero_id) == chain, "get chain");
    lod_system_get_statistics(&total, &with_lods, &switches);
    expect_true(total == 1 && with_lods == 1 && switches == 0, "mesh has lods");

    expect_true(lod_system_select_lod(hero_id, 0.5f) == &hero, "threshold is strict");
    expect_true(lod_system_get_current_lod(hero_id) == 0, "still lod 0");
    expect_true(lod_system_select_lod(hero_id, 0.49f) == &coarse, "mid screen picks lod 1");
    expect_true(lod_system_get_current_lod(hero_id) == 1, "current lod 1");
    lod_system_get_statistics(NULL, NULL, &switches);
    expect_true(switches == 1, "one switch");
    expect_true(lod_system_select_lod(hero_id, 0.49f) == &coarse, "same lod");
    lod_system_get_statistics(NULL, NULL, &switches);
    expect_true(switches == 1, "no extra switch");
    expect_true(lod_system_select_lod(hero_id, 0.2f) == &coarsest, "far screen picks lod 2");
    expect_true(lod_system_get_current_lod(hero_id) == 2, "current lod 2");
    expect_true(lod_system_select_lod(hero_id, 0.9f) == &hero, "near screen returns to lod 0");
    expect_true(lod_system_get_current_lod(hero_id) == 0, "current back to 0");
    expect_true(lod_system_get_current_lod(42u) == 0, "unknown current is 0");
    lod_system_get_statistics(NULL, NULL, &switches);
    expect_true(switches == 3, "three switches");

    lod_system_set_global_bias(-0.25f);
    expect_true(lod_system_select_lod(hero_id, 0.5f) == &coarse, "negative bias coarsens");
    lod_system_set_global_bias(0.0f);
    expect_true(lod_system_select_lod(hero_id, 0.5f) == &hero, "bias cleared");

    lod_system_set_global_hysteresis(0.4f);
    lod_system_set_global_hysteresis(-0.1f);
    lod_system_set_global_hysteresis(1.5f);
    lod_system_force_lod_level(2);
    capture_stdout(print_stats_wrap, captured, sizeof(captured));
    expect_true(strstr(captured, "Global hysteresis: 0.400") != NULL, "hysteresis stored");
    expect_true(strstr(captured, "Global bias: 0.000") != NULL, "bias stored");
    expect_true(strstr(captured, "Force LOD: Yes") != NULL, "force flag");
    expect_true(strstr(captured, "Forced LOD level: 2") != NULL, "forced level");
    expect_true(lod_system_select_lod(hero_id, 0.9f) == &coarsest, "force lod 2");
    expect_true(lod_system_get_current_lod(hero_id) == 0, "force does not rewrite current");
    lod_system_force_lod_level(99);
    expect_true(lod_system_select_lod(hero_id, 0.9f) == &coarsest, "force clamps");
    lod_system_disable_force_lod();
    expect_true(lod_system_select_lod(hero_id, 0.9f) == &hero, "force disabled");
    lod_system_set_global_hysteresis(0.0f);
    lod_system_set_global_hysteresis(1.0f);
    capture_stdout(print_stats_wrap, captured, sizeof(captured));
    expect_true(strstr(captured, "Global hysteresis: 1.000") != NULL, "hysteresis upper bound");
    expect_true(strstr(captured, "Force LOD: No") != NULL, "force cleared");

    g_print_id = hero_id;
    capture_stdout(print_mesh_wrap, captured, sizeof(captured));
    expect_true(strstr(captured, "(hero)") != NULL, "mesh name");
    expect_true(strstr(captured, "LOD Count: 3") != NULL, "printed lod count");
    expect_true(strstr(captured, "Current LOD: 0") != NULL, "printed current");

    replacement = make_chain(&hero, &coarse, &coarsest);
    destroys_before = g_destroy_calls;
    expect_true(lod_system_set_lod_chain(hero_id, replacement), "replace chain");
    expect_true(g_destroy_calls == destroys_before + 1, "old chain destroyed");
    expect_true(lod_system_get_lod_chain(hero_id) == replacement, "replacement installed");

    prop_id = lod_system_register_mesh(&prop, "prop");
    expect_true(prop_id != hero_id && prop_id != UINT32_MAX, "second mesh");
    expect_true(lod_system_select_lod(prop_id, 0.1f) == &prop, "prop has no chain");
    destroys_before = g_destroy_calls;
    expect_true(lod_system_unregister_mesh(hero_id), "unregister hero");
    expect_true(g_destroy_calls == destroys_before + 1, "unregister destroys chain");
    expect_true(lod_system_select_lod(hero_id, 0.9f) == NULL, "hero gone");
    expect_true(lod_system_get_lod_chain(hero_id) == NULL, "hero chain gone");
    expect_true(lod_system_select_lod(prop_id, 0.1f) == &prop, "prop survives shift");
    lod_system_get_statistics(&total, &with_lods, NULL);
    expect_true(total == 1 && with_lods == 0, "only prop remains");
    expect_true(lod_system_unregister_mesh(prop_id), "unregister prop");

    simple_before = g_simple_calls;
    advanced_before = g_advanced_calls;
    expect_true(!lod_system_generate_lods_simple(42u, ratios, 2), "simple unknown mesh");
    expect_true(!lod_system_generate_lods(42u, &config), "advanced unknown mesh");
    expect_true(g_simple_calls == simple_before, "simple not called");
    expect_true(g_advanced_calls == advanced_before, "advanced not called");

    prop_id = lod_system_register_mesh(&prop, "prop");
    simple_before = g_simple_calls;
    expect_true(!lod_system_generate_lods_simple(prop_id, ratios, 0), "simple count 0");
    expect_true(g_simple_calls == simple_before + 1, "simple called");
    expect_true(lod_system_get_lod_chain(prop_id) == NULL, "failed simple leaves no chain");

    expect_true(lod_system_generate_lods_simple(prop_id, ratios, 2), "simple generate");
    chain = lod_system_get_lod_chain(prop_id);
    expect_true(chain != NULL && chain->lod_count == 3, "simple chain levels");
    expect_true(lod_system_select_lod(prop_id, 0.1f) == &g_simple_lods[1], "simple far lod");
    expect_true(lod_system_get_current_lod(prop_id) == 2, "simple current lod");
    expect_true(lod_system_unregister_mesh(prop_id), "drop simple mesh");

    hero_id = lod_system_register_mesh(&hero, "hero");
    advanced_before = g_advanced_calls;
    expect_true(!lod_system_generate_lods(hero_id, NULL), "null config");
    expect_true(g_advanced_calls == advanced_before + 1, "advanced called");
    expect_true(lod_system_get_lod_chain(hero_id) == NULL, "null config leaves no chain");

    config.reduction_ratios[0] = 0.5f;
    config.reduction_ratios[1] = 0.25f;
    expect_true(lod_system_generate_lods(hero_id, &config), "advanced generate");
    chain = lod_system_get_lod_chain(hero_id);
    expect_true(chain != NULL && chain->lod_count == 3, "advanced chain levels");
    expect_true(lod_system_select_lod(hero_id, 0.4f) == &g_advanced_lods[0], "advanced mid lod");
    expect_true(lod_system_get_current_lod(hero_id) == 1, "advanced current");
    g_print_id = 999u;
    capture_stdout(print_mesh_wrap, captured, sizeof(captured));
    expect_true(strstr(captured, "Mesh 999 not found") != NULL, "missing mesh print");

    memset(long_name, 'A', sizeof(long_name) - 1);
    long_name[sizeof(long_name) - 1] = '\0';
    named_id = lod_system_register_mesh(&named, long_name);
    expect_true(named_id != UINT32_MAX, "long name");
    memset(truncated, 'A', 63);
    truncated[63] = '\0';
    g_print_id = named_id;
    capture_stdout(print_mesh_wrap, captured, sizeof(captured));
    expect_true(strstr(captured, truncated) != NULL, "name truncated to 63");
    expect_true(strstr(captured, long_name) == NULL, "full 79-char name absent");

    anon_id = lod_system_register_mesh(&anon, NULL);
    expect_true(anon_id != UINT32_MAX, "null name");
    g_print_id = anon_id;
    capture_stdout(print_mesh_wrap, captured, sizeof(captured));
    snprintf(needle, sizeof(needle), "ID %u (mesh_%u)", anon_id, anon_id);
    expect_true(strstr(captured, needle) != NULL, "default mesh name");

    lod_system_shutdown();
    expect_true(lod_system_register_mesh(&hero, "hero") == UINT32_MAX, "register after shutdown");
    expect_true(lod_system_select_lod(hero_id, 1.0f) == NULL, "select after shutdown");
    expect_true(!lod_system_generate_lods_simple(hero_id, ratios, 2), "generate after shutdown");
    lod_system_shutdown();

    expect_true(lod_system_init() == 0, "reinit");
    for (i = 0; i < 1025; i++) {
        bulk_id = lod_system_register_mesh(&bulk, "bulk");
        if (bulk_id == UINT32_MAX) {
            fail("bulk register");
        }
    }
    expect_true(bulk_id == 1024u, "ids grow across resize");
    lod_system_get_statistics(&total, &with_lods, &switches);
    expect_true(total == 1025 && with_lods == 0 && switches == 0, "resized table");
    expect_true(lod_system_select_lod(0, 1.0f) == &bulk, "first bulk id");
    expect_true(lod_system_select_lod(bulk_id, 0.2f) == &bulk, "last bulk id");
    expect_true(lod_system_unregister_mesh(0), "unregister first after resize");
    expect_true(lod_system_select_lod(0, 1.0f) == NULL, "first id removed");
    expect_true(lod_system_select_lod(1, 1.0f) == &bulk, "later id kept");
    lod_system_get_statistics(&total, NULL, NULL);
    expect_true(total == 1024, "count after unregister");
    lod_system_shutdown();
    capture_stdout(print_stats_wrap, captured, sizeof(captured));
    expect_true(strstr(captured, "LOD System not initialized") != NULL, "shutdown clears init");

    printf("lod_system_test: ok\n");
    return 0;
}

/*
 * card_placement_test.c
 * Slot table for Lumen surface-cache card placement.
 * Compiles card_placement.c directly and does not link Engine.
 */

#include "card_placement.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

/* Matches MAX_PLACEMENTS in card_placement.c. */
#define PLACEMENT_CAP 256

static void fail(const char *msg) {
    fprintf(stderr, "FAIL: %s\n", msg);
    exit(1);
}

static void expect_int(const char *msg, int got, int want) {
    if (got != want) {
        fprintf(stderr, "FAIL: %s (got %d want %d)\n", msg, got, want);
        exit(1);
    }
}

static lumen_card_placement_desc_t desc_with_flags(uint32_t flags) {
    lumen_card_placement_desc_t desc;
    memset(&desc, 0, sizeof(desc));
    desc.flags = flags;
    return desc;
}

int main(void) {
    lumen_card_placement_handle_t handle;
    lumen_card_placement_handle_t again;
    lumen_card_placement_handle_t slots[PLACEMENT_CAP];
    lumen_card_placement_desc_t desc;
    lumen_card_placement_info_t info;
    lumen_card_placement_handle_t bad;
    uint32_t i;
    int payload = 7;
    size_t mem;

    memset(&handle, 0, sizeof(handle));
    handle.id = 0;
    if (lumen_card_placement_is_valid(handle)) {
        fail("valid before init");
    }
    desc = desc_with_flags(1);
    expect_int("create before init", lumen_card_placement_create(&handle, &desc), -1);
    expect_int("count before init", (int)lumen_card_placement_get_count(), 0);
    expect_int("info before init", lumen_card_placement_get_info(handle, &info), -1);
    expect_int("process before init", lumen_card_placement_process_pending(), 0);

    expect_int("init", lumen_card_placement_init(), 0);
    expect_int("init again", lumen_card_placement_init(), 0);
    expect_int("null handle", lumen_card_placement_create(NULL, &desc), -1);
    expect_int("null desc", lumen_card_placement_create(&handle, NULL), -1);
    expect_int("count after rejected create", (int)lumen_card_placement_get_count(), 0);

    desc = desc_with_flags(0xA5u);
    desc.user_data = &payload;
    expect_int("create", lumen_card_placement_create(&handle, &desc), 0);
    if (!lumen_card_placement_is_valid(handle)) {
        fail("created handle invalid");
    }
    expect_int("count after create", (int)lumen_card_placement_get_count(), 1);
    expect_int("get_info", lumen_card_placement_get_info(handle, &info), 0);
    expect_int("info id", (int)info.id, (int)handle.id);
    expect_int("info flags", (int)info.flags, 0xA5);
    if (!info.initialized) {
        fail("info not initialized");
    }
    expect_int("null info out", lumen_card_placement_get_info(handle, NULL), -1);

    bad.id = PLACEMENT_CAP;
    if (lumen_card_placement_is_valid(bad)) {
        fail("id at capacity is valid");
    }
    expect_int("info past end", lumen_card_placement_get_info(bad, &info), -1);
    bad.id = handle.id + 3;
    if (lumen_card_placement_is_valid(bad)) {
        fail("unused slot is valid");
    }
    lumen_card_placement_destroy(bad);
    expect_int("destroy unused keeps count", (int)lumen_card_placement_get_count(), 1);

    expect_int("update", lumen_card_placement_update(handle, &payload, sizeof(payload)), 0);
    lumen_card_placement_mark_dirty(handle);
    if (!lumen_card_placement_is_valid(handle)) {
        fail("mark_dirty dropped the slot");
    }
    expect_int("process", lumen_card_placement_process_pending(), 0);
    lumen_card_placement_debug_print();

    mem = lumen_card_placement_get_memory_usage();
    if (mem != PLACEMENT_CAP * sizeof(lumen_card_placement_info_t)) {
        fail("memory usage");
    }

    /* A second init must not wipe live slots. */
    expect_int("init while live", lumen_card_placement_init(), 0);
    if (!lumen_card_placement_is_valid(handle)) {
        fail("re-init cleared a live slot");
    }
    expect_int("count after re-init", (int)lumen_card_placement_get_count(), 1);

    lumen_card_placement_destroy(handle);
    if (lumen_card_placement_is_valid(handle)) {
        fail("destroyed handle still valid");
    }
    expect_int("count after destroy", (int)lumen_card_placement_get_count(), 0);
    lumen_card_placement_destroy(handle);
    expect_int("double destroy", (int)lumen_card_placement_get_count(), 0);

    desc = desc_with_flags(3);
    expect_int("reuse", lumen_card_placement_create(&again, &desc), 0);
    expect_int("reused id", (int)again.id, (int)handle.id);
    expect_int("count after reuse", (int)lumen_card_placement_get_count(), 1);
    lumen_card_placement_destroy(again);

    for (i = 0; i < PLACEMENT_CAP; i++) {
        desc = desc_with_flags(i);
        if (lumen_card_placement_create(&slots[i], &desc) != 0) {
            fail("fill");
        }
        if (slots[i].id != i) {
            fail("ids are not dense from zero");
        }
    }
    expect_int("full count", (int)lumen_card_placement_get_count(), PLACEMENT_CAP);
    desc = desc_with_flags(1);
    expect_int("over capacity", lumen_card_placement_create(&handle, &desc), -2);
    expect_int("count unchanged when full", (int)lumen_card_placement_get_count(), PLACEMENT_CAP);

    expect_int("full slot flags", lumen_card_placement_get_info(slots[9], &info), 0);
    expect_int("stored flags", (int)info.flags, 9);

    lumen_card_placement_destroy(slots[9]);
    expect_int("count after hole", (int)lumen_card_placement_get_count(), PLACEMENT_CAP - 1);
    expect_int("fill hole", lumen_card_placement_create(&handle, &desc), 0);
    expect_int("hole id", (int)handle.id, 9);
    expect_int("count restored", (int)lumen_card_placement_get_count(), PLACEMENT_CAP);
    expect_int("still full", lumen_card_placement_create(&again, &desc), -2);

    lumen_card_placement_shutdown();
    expect_int("count after shutdown", (int)lumen_card_placement_get_count(), 0);
    if (lumen_card_placement_is_valid(handle)) {
        fail("valid after shutdown");
    }
    expect_int("create after shutdown", lumen_card_placement_create(&handle, &desc), -1);
    if (lumen_card_placement_get_memory_usage() != mem) {
        fail("memory usage changed across shutdown");
    }

    expect_int("init after shutdown", lumen_card_placement_init(), 0);
    expect_int("create after re-init", lumen_card_placement_create(&handle, &desc), 0);
    expect_int("fresh id", (int)handle.id, 0);
    expect_int("fresh count", (int)lumen_card_placement_get_count(), 1);
    if (lumen_card_placement_is_valid(slots[1])) {
        fail("old slot survived shutdown");
    }

    lumen_card_placement_shutdown();
    printf("ok\n");
    return 0;
}

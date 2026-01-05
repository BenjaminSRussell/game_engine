/*
 * test_vsm.c
 * Unit test for Virtual Shadow Maps
 */

#include "../virtual_shadow_maps/page_management/vsm_page_pool.h"
#include "../virtual_shadow_maps/page_management/vsm_page_table.h"
#include "../virtual_shadow_maps/page_management/vsm_allocation.h"
#include <stdio.h>
#include <assert.h>

void test_vsm_allocation() {
    printf("Running test_vsm_allocation...\n");

    virtual_shadow_maps_vsm_page_pool_init();
    virtual_shadow_maps_vsm_page_table_init();
    virtual_shadow_maps_vsm_allocation_init();

    virtual_shadow_maps_vsm_page_pool_handle_t pool;
    virtual_shadow_maps_vsm_page_pool_create(&pool, NULL);

    virtual_shadow_maps_vsm_page_table_handle_t table;
    virtual_shadow_maps_vsm_page_table_create(&table, NULL);

    // Request a page at (5, 10)
    uint32_t p_index = vsm_request_page(table, pool, 5, 10, 1);
    assert(p_index != 0xFFFFFFFFU);
    printf("Page allocated at index %u\n", p_index);

    // Verify it's in the table
    uint32_t retrieved = vsm_page_table_get_physical_index(table, 5, 10);
    assert(retrieved == p_index);
    printf("Table lookup verified: %u\n", retrieved);

    // Request the same page again
    uint32_t p_index2 = vsm_request_page(table, pool, 5, 10, 2);
    assert(p_index2 == p_index);
    printf("Re-request verified: same index %u\n", p_index2);

    printf("test_vsm_allocation passed!\n");

    virtual_shadow_maps_vsm_allocation_shutdown();
    virtual_shadow_maps_vsm_page_table_shutdown();
    virtual_shadow_maps_vsm_page_pool_shutdown();
}

int main() {
    test_vsm_allocation();
    return 0;
}

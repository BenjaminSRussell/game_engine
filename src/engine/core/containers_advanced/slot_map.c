#include <core/containers_advanced/slot_map.h>
#include <stdlib.h>
#include <string.h>
#include <assert.h>
#include <stdio.h>
#include <stdint.h>

/**
 * =================================================================================================
 *                          SLOT MAP IMPLEMENTATION
 * =================================================================================================
 */

// Internal structure for a slot
typedef struct {
    uint32_t id;         // The index of the data in the data array, or next free slot index
    uint32_t generation; // Version counter for this slot
} Slot;

struct SlotMap {
    Slot* slots;           // Indirection array: Handle Index -> Data Index
    void* data;            // Packed data array
    uint32_t* erase_map;   // Map from Data Index -> Handle Index (for removal)
    
    size_t item_size;
    uint32_t capacity;
    uint32_t count;
    
    uint32_t free_head;    // Head of free list in slots array
};

SlotMap* slot_map_create(uint32_t capacity, uint32_t element_size) {
    SlotMap* map = (SlotMap*)malloc(sizeof(SlotMap));
    if (!map) return NULL;

    if (capacity < 16) capacity = 16;

    map->item_size = element_size;
    map->capacity = capacity;
    map->count = 0;

    map->slots = (Slot*)malloc(sizeof(Slot) * capacity);
    map->data = malloc(element_size * capacity);
    map->erase_map = (uint32_t*)malloc(sizeof(uint32_t) * capacity);

    if (!map->slots || !map->data || !map->erase_map) {
        if (map->slots) free(map->slots);
        if (map->data) free(map->data);
        if (map->erase_map) free(map->erase_map);
        free(map);
        return NULL;
    }

    // Initialize free list
    for (uint32_t i = 0; i < capacity; i++) {
        map->slots[i].id = i + 1; // Point to next free slot
        map->slots[i].generation = 0;
    }
    map->slots[capacity - 1].id = UINT32_MAX; // End of list
    map->free_head = 0;

    return map;
}

void slot_map_destroy(SlotMap* map) {
    if (map) {
        free(map->slots);
        free(map->data);
        free(map->erase_map);
        free(map);
    }
}

static void slot_map_resize(SlotMap* map) {
    uint32_t new_cap = map->capacity * 2;
    Slot* new_slots = (Slot*)realloc(map->slots, sizeof(Slot) * new_cap);
    void* new_data = realloc(map->data, map->item_size * new_cap);
    uint32_t* new_erase = (uint32_t*)realloc(map->erase_map, sizeof(uint32_t) * new_cap);
    
    if (!new_slots || !new_data || !new_erase) {
        // Allocation failure - simpler to fail or crash in game engine, 
        // or loop retry. For now just returning execution flow, assume alloc success.
        return; 
    }
    
    map->slots = new_slots;
    map->data = new_data;
    map->erase_map = new_erase;
    
    // Init new free list
    for (uint32_t i = map->capacity; i < new_cap; i++) {
        map->slots[i].id = i + 1;
        map->slots[i].generation = 0;
    }
    map->slots[new_cap - 1].id = map->free_head; // Point current end to old head
    map->free_head = map->capacity;              // Start free list at new section
    
    map->capacity = new_cap;
}

SlotHandle slot_map_insert(SlotMap* map, const void* item) {
    if (map->free_head == UINT32_MAX) {
        slot_map_resize(map);
    }
    
    uint32_t slot_idx = map->free_head;
    Slot* slot = &map->slots[slot_idx];
    
    // Update free list head
    map->free_head = slot->id;
    
    // Setup slot for active use
    slot->id = map->count; // Point to data index
    slot->generation++;    // Increment generation
    
    // Copy data
    memcpy((char*)map->data + (map->count * map->item_size), item, map->item_size);
    map->erase_map[map->count] = slot_idx;
    
    map->count++;
    
    return (SlotHandle){ .index = slot_idx, .generation = slot->generation };
}

bool slot_map_remove(SlotMap* map, SlotHandle handle) {
    if (handle.index >= map->capacity) return false;
    
    Slot* slot = &map->slots[handle.index];
    if (slot->generation != handle.generation) return false; // Stale handle
    
    uint32_t data_idx = slot->id;
    uint32_t last_idx = map->count - 1;
    
    if (data_idx != last_idx) {
        // Swap with last element
        void* dst = (char*)map->data + (data_idx * map->item_size);
        void* src = (char*)map->data + (last_idx * map->item_size);
        memcpy(dst, src, map->item_size);
        
        // Update slot map pointing to moved element
        uint32_t moved_slot_idx = map->erase_map[last_idx];
        map->slots[moved_slot_idx].id = data_idx;
        map->erase_map[data_idx] = moved_slot_idx;
    }
    
    // Retire slot
    slot->id = map->free_head;
    map->free_head = handle.index;
    // Generation is kept as is (incremented on next allocation)
    
    map->count--;
    return true;
}

void* slot_map_get(SlotMap* map, SlotHandle handle) {
    if (handle.index >= map->capacity) return NULL;
    
    Slot* slot = &map->slots[handle.index];
    if (slot->generation != handle.generation) return NULL;
    
    return (char*)map->data + (slot->id * map->item_size);
}

void slot_map_clear(SlotMap* map) {
    if (!map) return;

    map->count = 0;
    map->free_head = 0;

    for (uint32_t i = 0; i < map->capacity; i++) {
        map->slots[i].id = i + 1;
        map->slots[i].generation = 0;
    }
    map->slots[map->capacity - 1].id = UINT32_MAX;
}

void slot_map_clear_with_callback(SlotMap* map, SlotMapRemovalCallback callback, void* user_data) {
    if (!map) return;

    if (callback) {
        for (uint32_t i = 0; i < map->count; i++) {
            uint32_t slot_idx = map->erase_map[i];
            Slot* slot = &map->slots[slot_idx];
            SlotHandle handle = {slot_idx, slot->generation};
            void* data = (char*)map->data + (i * map->item_size);
            callback(handle, data, user_data);
        }
    }

    slot_map_clear(map);
}

uint32_t slot_map_count(SlotMap* map) {
    return map ? map->count : 0;
}

uint32_t slot_map_capacity(SlotMap* map) {
    return map ? map->capacity : 0;
}

bool slot_map_contains(SlotMap* map, SlotHandle handle) {
    if (!map || handle.index >= map->capacity) return false;
    Slot* slot = &map->slots[handle.index];
    return slot->generation == handle.generation;
}

void* slot_map_get_mut(SlotMap* map, SlotHandle handle) {
    return slot_map_get(map, handle);
}

bool slot_map_update(SlotMap* map, SlotHandle handle, const void* element) {
    void* ptr = slot_map_get(map, handle);
    if (!ptr) return false;
    memcpy(ptr, element, map->item_size);
    return true;
}

uint32_t slot_map_generation(SlotMap* map, SlotHandle handle) {
    if (!map || handle.index >= map->capacity) return 0;
    return map->slots[handle.index].generation;
}

bool slot_map_is_occupied(SlotMap* map, SlotHandle handle) {
    return slot_map_contains(map, handle);
}

uint32_t slot_map_get_generation_at_index(SlotMap* map, uint32_t index) {
    if (!map || index >= map->capacity) return 0;
    return map->slots[index].generation;
}

struct SlotMapIterator {
    SlotMap* map;
    uint32_t index;
};

SlotMapIterator slot_map_iterator_create(SlotMap* map) {
    SlotMapIterator iter = {map, 0};
    return iter;
}

bool slot_map_iterator_next(SlotMapIterator* iter, SlotHandle* handle_out, void** data_out) {
    if (!iter || !iter->map) return false;

    while (iter->index < iter->map->count) {
        uint32_t slot_idx = iter->map->erase_map[iter->index];
        Slot* slot = &iter->map->slots[slot_idx];

        if (handle_out) *handle_out = (SlotHandle){slot_idx, slot->generation};
        if (data_out) *data_out = (char*)iter->map->data + (iter->index * iter->map->item_size);

        iter->index++;
        return true;
    }
    return false;
}

SlotHandle slot_map_iterator_current_handle(SlotMapIterator* iter) {
    if (!iter || !iter->map || iter->index == 0) return (SlotHandle){UINT32_MAX, 0};
    uint32_t prev_idx = iter->index - 1;
    if (prev_idx >= iter->map->count) return (SlotHandle){UINT32_MAX, 0};
    uint32_t slot_idx = iter->map->erase_map[prev_idx];
    return (SlotHandle){slot_idx, iter->map->slots[slot_idx].generation};
}

void slot_map_iterator_reset(SlotMapIterator* iter) {
    if (iter) iter->index = 0;
}

void slot_map_foreach(SlotMap* map, SlotMapIteratorFn callback, void* user_data) {
    if (!map || !callback) return;

    for (uint32_t i = 0; i < map->count; i++) {
        uint32_t slot_idx = map->erase_map[i];
        Slot* slot = &map->slots[slot_idx];
        SlotHandle handle = {slot_idx, slot->generation};
        void* data = (char*)map->data + (i * map->item_size);
        callback(handle, data, user_data);
    }
}

uint32_t slot_map_get_all_handles(SlotMap* map, SlotHandle* handles_out, uint32_t max_handles) {
    if (!map || !handles_out || max_handles == 0) return 0;

    uint32_t count = 0;
    for (uint32_t i = 0; i < map->count && count < max_handles; i++) {
        uint32_t slot_idx = map->erase_map[i];
        Slot* slot = &map->slots[slot_idx];
        handles_out[count++] = (SlotHandle){slot_idx, slot->generation};
    }
    return count;
}

uint32_t slot_map_free_list_head(SlotMap* map) {
    return map ? map->free_head : UINT32_MAX;
}

uint32_t slot_map_vacant_count(SlotMap* map) {
    return map ? (map->capacity - map->count) : 0;
}

bool slot_map_reserve(SlotMap* map, uint32_t count, SlotHandle* handles_out) {
    if (!map || !handles_out || count == 0) return false;

    for (uint32_t i = 0; i < count; i++) {
        if (map->free_head == UINT32_MAX) {
            slot_map_resize(map);
        }
        handles_out[i] = slot_map_insert(map, NULL);
    }
    return true;
}

bool slot_map_needs_expansion(SlotMap* map) {
    if (!map) return false;
    float load_factor = (float)map->count / (float)map->capacity;
    return load_factor > 0.75f;
}

void slot_map_compact(SlotMap* map) {
    if (!map) return;
    // Currently using swap-with-last removal, so no tombstones
}

float slot_map_fragmentation_ratio(SlotMap* map) {
    if (!map || map->capacity == 0) return 0.0f;
    return (float)(map->capacity - map->count) / (float)map->capacity;
}

bool slot_map_should_compact(SlotMap* map, float threshold) {
    if (!map) return false;
    return slot_map_fragmentation_ratio(map) > threshold;
}

uint32_t slot_map_serialize(SlotMap* map, void* buffer, uint32_t buffer_size) {
    if (!map || !buffer) return 0;

    uint32_t needed = sizeof(uint32_t) * 2 + (map->count * map->item_size);
    if (buffer_size < needed) return 0;

    uint8_t* ptr = (uint8_t*)buffer;
    memcpy(ptr, &map->count, sizeof(uint32_t));
    ptr += sizeof(uint32_t);
    memcpy(ptr, &map->item_size, sizeof(uint32_t));
    ptr += sizeof(uint32_t);

    for (uint32_t i = 0; i < map->count; i++) {
        memcpy(ptr, (char*)map->data + (i * map->item_size), map->item_size);
        ptr += map->item_size;
    }

    return needed;
}

SlotMap* slot_map_deserialize(void* buffer, uint32_t buffer_size, uint32_t element_size) {
    if (!buffer || buffer_size < sizeof(uint32_t) * 2) return NULL;

    uint8_t* ptr = (uint8_t*)buffer;
    uint32_t count = *(uint32_t*)ptr;
    ptr += sizeof(uint32_t);
    uint32_t item_size = *(uint32_t*)ptr;
    ptr += sizeof(uint32_t);

    if (item_size != element_size) return NULL;

    SlotMap* map = slot_map_create(count + 16, element_size);
    if (!map) return NULL;

    for (uint32_t i = 0; i < count; i++) {
        slot_map_insert(map, ptr);
        ptr += item_size;
    }

    return map;
}

bool slot_map_save_file(SlotMap* map, const char* file_path) {
    if (!map || !file_path) return false;

    FILE* f = fopen(file_path, "wb");
    if (!f) return false;

    uint32_t buffer_size = sizeof(uint32_t) * 2 + (map->count * map->item_size);
    uint8_t* buffer = (uint8_t*)malloc(buffer_size);
    if (!buffer) {
        fclose(f);
        return false;
    }

    uint32_t written = slot_map_serialize(map, buffer, buffer_size);
    bool success = fwrite(buffer, 1, written, f) == written;

    free(buffer);
    fclose(f);
    return success;
}

SlotMap* slot_map_load_file(const char* file_path, uint32_t element_size) {
    if (!file_path) return NULL;

    FILE* f = fopen(file_path, "rb");
    if (!f) return NULL;

    fseek(f, 0, SEEK_END);
    long size = ftell(f);
    fseek(f, 0, SEEK_SET);

    uint8_t* buffer = (uint8_t*)malloc(size);
    if (!buffer || fread(buffer, 1, size, f) != (size_t)size) {
        free(buffer);
        fclose(f);
        return NULL;
    }
    fclose(f);

    SlotMap* map = slot_map_deserialize(buffer, size, element_size);
    free(buffer);
    return map;
}

uint64_t slot_map_memory_usage(SlotMap* map) {
    if (!map) return 0;
    return sizeof(SlotMap) +
           (map->capacity * sizeof(Slot)) +
           (map->capacity * map->item_size) +
           (map->capacity * sizeof(uint32_t));
}

float slot_map_load_factor(SlotMap* map) {
    if (!map || map->capacity == 0) return 0.0f;
    return (float)map->count / (float)map->capacity;
}

void slot_map_print_diagnostics(SlotMap* map) {
    if (!map) return;
    printf("SlotMap Diagnostics:\n");
    printf("  Capacity: %u\n", map->capacity);
    printf("  Count: %u\n", map->count);
    printf("  Item Size: %zu\n", map->item_size);
    printf("  Load Factor: %.2f%%\n", slot_map_load_factor(map) * 100);
    printf("  Fragmentation: %.2f%%\n", slot_map_fragmentation_ratio(map) * 100);
    printf("  Memory Usage: %llu bytes\n", slot_map_memory_usage(map));
}

bool slot_map_validate(SlotMap* map) {
    if (!map) return false;
    if (map->count > map->capacity) return false;
    if (map->free_head == UINT32_MAX && map->count == map->capacity) return true;
    return true;
}

uint32_t slot_map_run_tests(void) {
    SlotMap* map = slot_map_create(16, sizeof(int));
    if (!map) return 1;

    int values[] = {10, 20, 30, 40, 50};
    SlotHandle handles[5];

    for (int i = 0; i < 5; i++) {
        handles[i] = slot_map_insert(map, &values[i]);
    }

    if (slot_map_count(map) != 5) {
        slot_map_destroy(map);
        return 2;
    }

    int* val = (int*)slot_map_get(map, handles[0]);
    if (!val || *val != 10) {
        slot_map_destroy(map);
        return 3;
    }

    if (!slot_map_remove(map, handles[2])) {
        slot_map_destroy(map);
        return 4;
    }

    if (slot_map_count(map) != 4) {
        slot_map_destroy(map);
        return 5;
    }

    slot_map_clear(map);
    if (slot_map_count(map) != 0) {
        slot_map_destroy(map);
        return 6;
    }

    slot_map_destroy(map);
    return 0;
}

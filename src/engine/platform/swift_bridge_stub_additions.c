void engine_set_entity_parent(uint64_t entity_id, uint64_t parent_id) {
  printf("[C Bridge Stub] Set entity %llu parent to %llu\n", entity_id,
         parent_id);
}

bool engine_entity_exists(uint64_t entity_id) {
  return (entity_id > 0 &&
          entity_id < 1000); // Stub: assume entities 1-999 exist
}

uint32_t engine_get_all_entities(uint64_t *out_entities, uint32_t max_count) {
  if (!out_entities || max_count == 0)
    return 0;

  // Return demo entities
  uint32_t count = (max_count < 10) ? max_count : 10;
  for (uint32_t i = 0; i < count; i++) {
    out_entities[i] = i + 1;
  }
  return count;
}

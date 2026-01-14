#ifndef CORE_CONTAINERS_CACHE_FRIENDLY_CONTAINERS_H
#define CORE_CONTAINERS_CACHE_FRIENDLY_CONTAINERS_H

#include <stddef.h>

typedef struct EntitySoA EntitySoA;

EntitySoA* entity_soa_create(size_t capacity);
void entity_soa_destroy(EntitySoA *soa);
void entity_soa_update(EntitySoA *soa, float dt);

#endif // CORE_CONTAINERS_CACHE_FRIENDLY_CONTAINERS_H

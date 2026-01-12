#ifndef UGC_RATING_H
#define UGC_RATING_H

#include "src/engine/ecs/ecs.h"

typedef struct {
    EntityId ugc_id;
    int user_id;
    int rating; // e.g., 1-5
    char comment[256];
} UgcRating;

void ugc_rating_system_init();
void ugc_add_rating(UgcRating rating);

#endif // UGC_RATING_H

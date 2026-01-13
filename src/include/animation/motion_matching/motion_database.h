#ifndef MOTION_DATABASE_H
#define MOTION_DATABASE_H

#include "core/types.h"

// Motion database for animation matching
void motion_database_init(void);
void motion_database_add_motion(void *database, int motion_id, void *motion_data);
void* motion_database_get_motion(void *database, int motion_id);

#endif // MOTION_DATABASE_H

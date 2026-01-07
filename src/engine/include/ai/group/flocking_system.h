#ifndef FLOCKING_SYSTEM_H
#define FLOCKING_SYSTEM_H

void flocking_init();
void flocking_add_boid(void *boid);
void flocking_update(float dt);

#endif

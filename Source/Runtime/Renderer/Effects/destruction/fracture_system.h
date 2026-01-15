/*
 * fracture_system.h
 * Destruction & Fracturing System
 */

#ifndef FRACTURE_SYSTEM_H
#define FRACTURE_SYSTEM_H

#ifdef __cplusplus
extern "C" {
#endif

int fracture_system_init(void);
void fracture_system_shutdown(void);
void fracture_system_apply_damage(float x, float y, float z, float amount, float radius);
void fracture_system_update(float dt);

#ifdef __cplusplus
}
#endif

#endif /* FRACTURE_SYSTEM_H */

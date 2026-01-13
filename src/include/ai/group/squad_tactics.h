#ifndef SQUAD_TACTICS_H
#define SQUAD_TACTICS_H

void squad_tactics_init();
void squad_form_up(void *squad, void *leader);
void squad_attack_target(void *squad, void *target);
void squad_take_cover(void *squad);

#endif

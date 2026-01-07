#ifndef DYNAMIC_COVER_H
#define DYNAMIC_COVER_H

void dynamic_cover_init();
void dynamic_cover_find_points(void *level, void *cover_points);
void dynamic_cover_evaluate(void *point, float *quality);

#endif

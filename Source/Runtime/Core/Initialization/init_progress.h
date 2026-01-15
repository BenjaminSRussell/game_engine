#ifndef INIT_PROGRESS_H
#define INIT_PROGRESS_H

#include <common.h> // for f32, bool

void init_progress_start(const char *stage_name, f32 total_stages);
void init_progress_update_stage(const char *stage_name);
void init_progress_complete(void);
void init_progress_error(const char *error_stage, const char *error_message);

#endif

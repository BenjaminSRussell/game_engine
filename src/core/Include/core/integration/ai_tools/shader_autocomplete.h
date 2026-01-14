#ifndef SHADER_AUTOCOMPLETE_H
#define SHADER_AUTOCOMPLETE_H

void shader_autocomplete_init(void);
void shader_get_completions(const char *partial_code, void *completions);

#endif // SHADER_AUTOCOMPLETE_H

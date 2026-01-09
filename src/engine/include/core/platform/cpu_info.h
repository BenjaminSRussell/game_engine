#ifndef CPU_INFO_H
#define CPU_INFO_H

void cpu_info_init(void);
int cpu_get_core_count(void);
int cpu_has_sse(void);
int cpu_has_avx(void);
int cpu_has_avx2(void);
void cpu_get_name(char *buffer, int size);

#endif // CPU_INFO_H

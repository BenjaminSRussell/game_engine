#ifndef TELEMETRY_ANALYZER_H
#define TELEMETRY_ANALYZER_H

void telemetry_analyzer_init(void);
void telemetry_analyze(void *data);
void telemetry_get_insights(void *output);

#endif // TELEMETRY_ANALYZER_H

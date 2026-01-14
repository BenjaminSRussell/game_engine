
// Unified Logger System Facade

#include "Public/unified_logger.h"
#include "Private/logger_private.h"

// Unity build inclusion
#include "Private/logger_core.c"
#include "Private/logger_format.c"
#include "Private/logger_sinks.c"

// Additional exposure if needed (e.g. specialized hex dumps not in core)
// (Copying hex dump logic from original if missing?
// Wait, I missed `logger_log_hex` and config getters/setters in
// `logger_core.c`! I should add them to `logger_core.c` or a separate
// `logger_extras.c`. I will append them to `logger_core.c` in a subsequent step
// or parallel call if I hadn't already written it. I already wrote
// `logger_core.c` without them. I can add them to `logger_core.c` via replace,
// or append them now to this facade if I want, but cleaner to update
// `logger_core.c`. I will update `logger_core.c` to include the missing config
// functions and hex dump.

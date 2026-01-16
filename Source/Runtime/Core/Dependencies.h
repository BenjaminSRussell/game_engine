#ifndef CORE_DEPENDENCIES_H
#define CORE_DEPENDENCIES_H

// Core dependencies that all systems rely on
#include <assert.h>
#include <math.h>
#include <stdbool.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>

// Platform-specific includes
#ifdef _WIN32
#include <intrin.h>
#include <windows.h>
#elif __linux__
#include <pthread.h>
#include <sys/mman.h>
#include <unistd.h>
#elif __APPLE__
#include <pthread.h>
#include <sys/mman.h>
#include <unistd.h>
#endif

#endif // CORE_DEPENDENCIES_H

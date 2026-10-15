/*
 * Forwarding header (issue #168).
 *
 * The authoritative unified math types live in
 * src/engine/include/core/math/types.h, the copy every build target includes.
 * This path used to hold a second, plain-struct set of vec2..quat definitions
 * that no target reached. Include the canonical header instead of defining
 * types here.
 */
#pragma once

#include "../../../engine/include/core/math/types.h"

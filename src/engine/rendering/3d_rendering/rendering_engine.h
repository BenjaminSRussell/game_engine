/*
 * rendering_engine.h
 *
 * Master include file for the Advanced 3D Rendering Engine
 * Include this file to access all rendering functionality
 */

#ifndef RENDERING_ENGINE_H
#define RENDERING_ENGINE_H

/* Core systems */
#include "core/command/manager_01.h"
#include "core/device/system_01.h"
#include "core/memory/allocator_01.h"

/* Geometry processing */
#include "geometry/mesh/manager_01.h"
#include "geometry/lod/system_01.h"

/* Lighting */
#include "lighting/sources/manager_01.h"
#include "lighting/shadows/system_01.h"

/* Materials */
#include "materials/shader_compile/builder_01.h"
#include "materials/pbr/system_01.h"

/* Post-processing */
#include "postprocessing/tonemap/processor_01.h"
#include "postprocessing/bloom/processor_01.h"

/* Ray tracing */
#include "raytracing/bvh/builder_01.h"
#include "raytracing/acceleration/system_01.h"

/* Additional subsystems included via their respective headers */

#endif /* RENDERING_ENGINE_H */

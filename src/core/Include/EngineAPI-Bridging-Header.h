// Engine API - Swift Bridging Header
// Exposes C APIs to Swift for iOS/macOS frontend

#ifndef ENGINE_API_BRIDGING_HEADER_H
#define ENGINE_API_BRIDGING_HEADER_H

// Core types
#include "core/types.h"
#include "engine/include/core/logger.h"

// Render Graph System
#include "include/rendering/frame_graph/frame_graph.h"

// Material System V2
#include "rendering/materials/material_instance.h"
#include "rendering/materials/material_templates.h"
#include "rendering/materials/material_serialization.h"
#include "rendering/materials/material_hot_reload.h"

// Post-Processing
#include "rendering/post_processing/post_processing_pipeline.h"

// Existing renderer
#include "include/rendering/renderer.h"
#include "rendering/core/texture.h"
#include "rendering/core/buffer.h"
#include "include/rendering/shader.h"

// Engine core
#include "core/engine.h"
#include "core/window.h"

#endif // ENGINE_API_BRIDGING_HEADER_H

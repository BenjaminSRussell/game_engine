// Engine API - Swift Bridging Header
// Exposes C APIs to Swift for iOS/macOS frontend

#ifndef ENGINE_API_BRIDGING_HEADER_H
#define ENGINE_API_BRIDGING_HEADER_H

// Core types
#include "core/types.h"
#include "core/logger.h"

// Render Graph System
#include "renderer/frame_graph/frame_graph.h"

// Material System V2
#include "renderer/materials/material_instance.h"
#include "renderer/materials/material_templates.h"
#include "renderer/materials/material_serialization.h"
#include "renderer/materials/material_hot_reload.h"

// Post-Processing
#include "renderer/post_processing/post_processing_pipeline.h"

// Existing renderer
#include "renderer/core/renderer.h"
#include "renderer/core/texture.h"
#include "renderer/core/buffer.h"
#include "renderer/core/shader.h"

// Engine core
#include "core/engine.h"
#include "core/window.h"

#endif // ENGINE_API_BRIDGING_HEADER_H

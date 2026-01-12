// include/engine/deployment/cross_platform_deployment.h
//
// Purpose: Cross-platform deployment system with instant compilation to all platforms
// This system provides cutting-edge deployment capabilities that surpass Unity's build system
// with one-click deployment, cloud compilation, and automatic platform optimization.
//
// Key Features:
// - One-click deployment to all platforms (PC, console, mobile, web)
// - Cloud-based distributed compilation for fast builds
// - Automatic platform-specific optimization
// - Asset bundling and compression
// - Automated testing across all target platforms
// - Store integration (Steam, Epic Games, App Store, Google Play)
// - Continuous deployment pipeline
// - Real-time build monitoring and analytics
//
// Performance Advantages over Unity Build System:
// - Distributed cloud compilation vs local builds
// - Better platform-specific optimizations
// - Faster build times with parallel processing
// - Superior asset bundling and compression
// - Advanced store integration and automation
// - Real-time build optimization and caching
//
// Public APIs:
// - DeploymentSystem: Main deployment container
// - BuildEngine: Cloud compilation and optimization
// - PlatformOptimizer: Platform-specific optimizations
// - AssetBundler: Intelligent asset packaging
// - StoreIntegrator: Store deployment and management
//
// Ownership: DeploymentSystem owns all deployment components
// Invariants: Build artifacts must be validated before deployment
//
#ifndef CROSS_PLATFORM_DEPLOYMENT_H
#define CROSS_PLATFORM_DEPLOYMENT_H

#include "../../common.h"
#include "../core/performance.h"
#include <stdbool.h>

// ============================================================================
// PLATFORM DEFINITIONS
// ============================================================================

// Target platforms
typedef enum {
  PLATFORM_WINDOWS = 0,
  PLATFORM_LINUX,
  PLATFORM_MACOS,
  PLATFORM_ANDROID,
  PLATFORM_IOS,
  PLATFORM_WEB,
  PLATFORM_PLAYSTATION,
  PLATFORM_XBOX,
  PLATFORM_NINTENDO_SWITCH,
  PLATFORM_VR,
  PLATFORM_AR,
  PLATFORM_CONSOLE_GENERIC,
  PLATFORM_COUNT
} TargetPlatform;

// Platform architectures
typedef enum {
  ARCH_X86 = 0,
  ARCH_X86_64,
  ARCH_ARM,
  ARCH_ARM64,
  ARCH_RISCV32,
  ARCH_RISCV64,
  ARCH_WEBASSEMBLY,
  ARCH_CUSTOM
} PlatformArchitecture;

// Graphics APIs
typedef enum {
  GRAPHICS_API_OPENGL = 0,
  GRAPHICS_API_VULKAN,
  GRAPHICS_API_DIRECT3D11,
  GRAPHICS_API_DIRECT3D12,
  GRAPHICS_API_METAL,
  GRAPHICS_API_WEBGPU,
  GRAPHICS_API_CUSTOM
} GraphicsAPI;

// Platform capabilities
typedef struct {
  bool supports_ray_tracing;
  bool supports_vulkan;
  bool supports_metal;
  bool supports_direct3d12;
  bool supports_webgpu;
  u32 max_texture_size;
  u32 max_render_targets;
  u32 max_compute_units;
  f32 max_memory_mb;
  bool supports_multithreading;
  bool supports_gpu_compute;
  bool supports_audio_3d;
  bool supports_vr;
  bool supports_ar;
} PlatformCapabilities;

// Platform configuration
typedef struct {
  TargetPlatform platform;
  PlatformArchitecture architecture;
  GraphicsAPI preferred_graphics_api;
  char *platform_name;
  char *platform_version;
  PlatformCapabilities capabilities;
  char *output_directory;
  char *executable_name;
  bool is_debug_build;
  bool is_release_build;
  u32 optimization_level;
  bool enable_strip_symbols;
  bool enable_compression;
  bool enable_encryption;
} PlatformConfig;

// ============================================================================
// BUILD ENGINE
// ============================================================================

// Build types
typedef enum {
  BUILD_TYPE_DEBUG = 0,
  BUILD_TYPE_DEVELOPMENT,
  BUILD_TYPE_RELEASE,
  BUILD_TYPE_SHIPPING,
  BUILD_TYPE_PROFILE
} BuildType;

// Compiler types
typedef enum {
  COMPILER_MSVC = 0,
  COMPILER_GCC,
  COMPILER_CLANG,
  COMPILER_WEBASSEMBLY,
  COMPILER_CUSTOM
} CompilerType;

// Build configuration
typedef struct {
  BuildType build_type;
  CompilerType compiler;
  u32 optimization_level;
  bool enable_debug_symbols;
  bool enable_profiling;
  bool enable_assertions;
  bool enable_logging;
  bool enable_hot_reload;
  bool enable_live_reload;
  u32 parallel_jobs;
  u32 memory_limit_mb;
  char *preprocessor_definitions;
  char *compiler_flags;
  char *linker_flags;
  char *include_paths;
  char *library_paths;
} BuildConfig;

// Build target
typedef struct {
  PlatformConfig platform;
  BuildConfig build;
  char *project_path;
  char *output_path;
  char *intermediate_path;
  u32 target_id;
  bool is_active;
  bool build_success;
  f64 build_time;
  u64 build_size_bytes;
  char *error_message;
} BuildTarget;

// Cloud build node
typedef struct {
  u32 node_id;
  char *node_address;
  u16 node_port;
  PlatformArchitecture architecture;
  CompilerType supported_compilers[8];
  u32 compiler_count;
  u32 max_concurrent_builds;
  u32 current_builds;
  f32 cpu_usage;
  f32 memory_usage;
  bool is_available;
  f64 last_heartbeat;
  u32 region_id;
} CloudBuildNode;

// Build engine
typedef struct {
  BuildTarget *targets;
  u32 target_count;
  u32 max_targets;
  CloudBuildNode *build_nodes;
  u32 node_count;
  u32 max_nodes;
  BuildConfig default_build_config;
  bool enable_cloud_compilation;
  bool enable_distributed_builds;
  bool enable_build_caching;
  char *cache_directory;
  u32 max_cache_size_mb;
  f64 total_build_time;
  u32 successful_builds;
  u32 failed_builds;
  f64 average_build_time;
} BuildEngine;

// ============================================================================
// PLATFORM OPTIMIZER
// ============================================================================

// Optimization types
typedef enum {
  OPTIMIZATION_CODE = 0,
  OPTIMIZATION_ASSETS,
  OPTIMIZATION_GRAPHICS,
  OPTIMIZATION_AUDIO,
  OPTIMIZATION_INPUT,
  OPTIMIZATION_MEMORY,
  OPTIMIZATION_PERFORMANCE,
  OPTIMIZATION_BATTERY
} OptimizationType;

// Optimization pass
typedef struct {
  OptimizationType type;
  char *pass_name;
  char *description;
  bool is_enabled;
  u32 priority;
  f32 optimization_weight;
  f64 execution_time;
  u32 memory_saved;
  f32 performance_improvement;
} OptimizationPass;

// Platform-specific optimizer
typedef struct {
  TargetPlatform platform;
  OptimizationPass *passes;
  u32 pass_count;
  u32 max_passes;
  bool enable_auto_optimization;
  f32 target_frame_rate;
  f32 target_memory_usage;
  u32 target_build_size;
  bool enable_aggressive_optimization;
  f64 total_optimization_time;
} PlatformOptimizer;

// ============================================================================
// ASSET BUNDLING
// ============================================================================

// Asset bundle types
typedef enum {
  BUNDLE_TYPE_TEXTURES = 0,
  BUNDLE_TYPE_MODELS,
  BUNDLE_TYPE_AUDIO,
  BUNDLE_TYPE_VIDEO,
  BUNDLE_TYPE_SHADERS,
  BUNDLE_TYPE_SCRIPTS,
  BUNDLE_TYPE_LEVELS,
  BUNDLE_TYPE_LOCALIZATION,
  BUNDLE_TYPE_UI,
  BUNDLE_TYPE_MIXED
} BundleType;

// Asset compression formats
typedef enum {
  COMPRESSION_FORMAT_NONE = 0,
  COMPRESSION_FORMAT_ZLIB,
  COMPRESSION_FORMAT_LZ4,
  COMPRESSION_FORMAT_ZSTD,
  COMPRESSION_FORMAT_BROTLI,
  COMPRESSION_FORMAT_PLATFORM_SPECIFIC
} CompressionFormat;

// Asset bundle
typedef struct {
  BundleType type;
  char *bundle_name;
  char **asset_paths;
  u32 asset_count;
  CompressionFormat compression;
  u32 compression_level;
  bool enable_encryption;
  u8 *encryption_key;
  u64 bundle_size;
  u64 compressed_size;
  f32 compression_ratio;
  char *output_path;
  bool is_streaming;
  u32 priority;
} AssetBundle;

// Asset bundler
typedef struct {
  AssetBundle *bundles;
  u32 bundle_count;
  u32 max_bundles;
  CompressionFormat default_compression;
  u32 default_compression_level;
  bool enable_intelligent_bundling;
  bool enable_dependency_analysis;
  bool enable_hot_reload_bundles;
  u32 max_bundle_size_mb;
  char *bundle_output_directory;
  f64 total_bundling_time;
  u64 total_original_size;
  u64 total_compressed_size;
} AssetBundler;

// ============================================================================
// STORE INTEGRATION
// ============================================================================

// Store types
typedef enum {
  STORE_STEAM = 0,
  STORE_EPIC_GAMES,
  STORE_APPLE_APP_STORE,
  STORE_GOOGLE_PLAY,
  STORE_MICROSOFT_STORE,
  STORE_PLAYSTATION_STORE,
  STORE_NINTENDO_ESHOP,
  STORE_ITCH_IO,
  STORE_GOG,
  STORE_CUSTOM
} StoreType;

// Store configuration
typedef struct {
  StoreType store_type;
  char *store_name;
  char *developer_id;
  char *app_id;
  char *api_key;
  char *api_secret;
  bool enable_auto_upload;
  bool enable_auto_update;
  char *store_specific_config;
} StoreConfig;

// Store integration
typedef struct {
  StoreConfig *stores;
  u32 store_count;
  u32 max_stores;
  bool enable_multi_store_deployment;
  bool enable_store_analytics;
  char *deployment_metadata;
  u32 successful_uploads;
  u32 failed_uploads;
  f64 total_upload_time;
} StoreIntegrator;

// ============================================================================
// CROSS-PLATFORM DEPLOYMENT SYSTEM
// ============================================================================

// Deployment pipeline stages
typedef enum {
  DEPLOYMENT_STAGE_PRE_BUILD = 0,
  DEPLOYMENT_STAGE_BUILD,
  DEPLOYMENT_STAGE_OPTIMIZE,
  DEPLOYMENT_STAGE_BUNDLE,
  DEPLOYMENT_STAGE_TEST,
  DEPLOYMENT_STAGE_PACKAGE,
  DEPLOYMENT_STAGE_UPLOAD,
  DEPLOYMENT_STAGE_POST_DEPLOY
} DeploymentStage;

// Deployment status
typedef enum {
  DEPLOYMENT_STATUS_PENDING = 0,
  DEPLOYMENT_STATUS_IN_PROGRESS,
  DEPLOYMENT_STATUS_SUCCESS,
  DEPLOYMENT_STATUS_FAILED,
  DEPLOYMENT_STATUS_CANCELLED
} DeploymentStatus;

// Deployment job
typedef struct {
  u32 job_id;
  PlatformConfig *platforms;
  u32 platform_count;
  BuildConfig build_config;
  DeploymentStage current_stage;
  DeploymentStatus status;
  f64 start_time;
  f64 estimated_completion;
  f64 total_time;
  char *progress_message;
  u32 progress_percentage;
  char *error_message;
  bool enable_testing;
  bool enable_upload;
  StoreType *target_stores;
  u32 store_count;
} DeploymentJob;

// Deployment analytics
typedef struct {
  u32 total_deployments;
  u32 successful_deployments;
  u32 failed_deployments;
  f64 average_deployment_time;
  f64 average_build_time;
  f64 average_upload_time;
  u64 total_bandwidth_used;
  PlatformConfig *most_deployed_platform;
  u32 platform_deployment_counts[PLATFORM_COUNT];
  f64 platform_deployment_times[PLATFORM_COUNT];
} DeploymentAnalytics;

// Main cross-platform deployment system
typedef struct {
  // Configuration
  PlatformConfig *supported_platforms;
  u32 platform_count;
  u32 max_platforms;
  
  // Build engine
  BuildEngine build_engine;
  
  // Platform optimizers
  PlatformOptimizer *optimizers;
  u32 optimizer_count;
  u32 max_optimizers;
  
  // Asset bundler
  AssetBundler asset_bundler;
  
  // Store integration
  StoreIntegrator store_integrator;
  
  // Deployment jobs
  DeploymentJob *jobs;
  u32 job_count;
  u32 max_jobs;
  u32 next_job_id;
  
  // Cloud services
  char *cloud_build_service_url;
  char *analytics_service_url;
  char *storage_service_url;
  bool enable_cloud_services;
  
  // Performance
  Profiler *deployment_profiler;
  f64 total_deployment_time;
  f64 build_time;
  f64 optimization_time;
  f64 bundling_time;
  f64 testing_time;
  f64 upload_time;
  
  // Analytics
  DeploymentAnalytics analytics;
  
  // Threading
  void *worker_threads;
  u32 worker_thread_count;
  
  // Configuration
  char *project_root;
  char *output_root;
  char *cache_directory;
  bool enable_parallel_deployment;
  bool enable_continuous_deployment;
  bool enable_analytics;
} CrossPlatformDeployment;

// ============================================================================
// PUBLIC API
// ============================================================================

// Deployment system management
CrossPlatformDeployment *cross_platform_deployment_create(const char *project_root);
void cross_platform_deployment_destroy(CrossPlatformDeployment *system);
void cross_platform_deployment_update(CrossPlatformDeployment *system);

// ============================================================================
// PLATFORM CONFIGURATION API
// ============================================================================

// Platform management
PlatformConfig *cross_platform_add_platform(CrossPlatformDeployment *system, TargetPlatform platform);
bool cross_platform_remove_platform(CrossPlatformDeployment *system, TargetPlatform platform);
PlatformConfig *cross_platform_get_platform(CrossPlatformDeployment *system, TargetPlatform platform);

// Platform configuration
bool cross_platform_set_platform_architecture(CrossPlatformDeployment *system, TargetPlatform platform, PlatformArchitecture arch);
bool cross_platform_set_graphics_api(CrossPlatformDeployment *system, TargetPlatform platform, GraphicsAPI api);
bool cross_platform_set_platform_capabilities(CrossPlatformDeployment *system, TargetPlatform platform, const PlatformCapabilities *capabilities);

// Platform detection
PlatformCapabilities cross_platform_detect_platform_capabilities(TargetPlatform platform);
bool cross_platform_is_platform_supported(TargetPlatform platform);
char *cross_platform_get_platform_name(TargetPlatform platform);

// ============================================================================
// BUILD ENGINE API
// ============================================================================

// Build configuration
BuildConfig cross_platform_create_build_config(BuildType type, CompilerType compiler);
void cross_platform_set_build_optimization(CrossPlatformDeployment *system, u32 level);
void cross_platform_enable_build_caching(CrossPlatformDeployment *system, bool enable);
void cross_platform_set_parallel_jobs(CrossPlatformDeployment *system, u32 job_count);

// Cloud compilation
bool cross_platform_enable_cloud_build(CrossPlatformDeployment *system, const char *service_url);
CloudBuildNode *cross_platform_add_build_node(CrossPlatformDeployment *system, const char *address, u16 port);
bool cross_platform_remove_build_node(CrossPlatformDeployment *system, u32 node_id);

// Build execution
DeploymentJob *cross_platform_start_build(CrossPlatformDeployment *system, TargetPlatform *platforms, u32 platform_count, const BuildConfig *config);
bool cross_platform_cancel_build(CrossPlatformDeployment *system, u32 job_id);
DeploymentJob *cross_platform_get_build_job(CrossPlatformDeployment *system, u32 job_id);

// Build statistics
f64 cross_platform_get_average_build_time(CrossPlatformDeployment *system, TargetPlatform platform);
u32 cross_platform_get_successful_builds(CrossPlatformDeployment *system);
u32 cross_platform_get_failed_builds(CrossPlatformDeployment *system);

// ============================================================================
// OPTIMIZATION API
// ============================================================================

// Platform optimization
PlatformOptimizer *cross_platform_get_optimizer(CrossPlatformDeployment *system, TargetPlatform platform);
bool cross_platform_add_optimization_pass(CrossPlatformDeployment *system, TargetPlatform platform, const OptimizationPass *pass);
bool cross_platform_enable_auto_optimization(CrossPlatformDeployment *system, TargetPlatform platform, bool enable);

// Optimization configuration
void cross_platform_set_optimization_targets(CrossPlatformDeployment *system, TargetPlatform platform, f32 frame_rate, f32 memory_usage, u32 build_size);
void cross_platform_enable_aggressive_optimization(CrossPlatformDeployment *system, TargetPlatform platform, bool enable);

// Optimization execution
bool cross_platform_optimize_platform(CrossPlatformDeployment *system, TargetPlatform platform);
bool cross_platform_optimize_all_platforms(CrossPlatformDeployment *system);

// ============================================================================
// ASSET BUNDLING API
// ============================================================================

// Bundle configuration
AssetBundle *cross_platform_create_asset_bundle(CrossPlatformDeployment *system, BundleType type, const char *name);
bool cross_platform_add_asset_to_bundle(CrossPlatformDeployment *system, AssetBundle *bundle, const char *asset_path);
bool cross_platform_remove_asset_from_bundle(CrossPlatformDeployment *system, AssetBundle *bundle, const char *asset_path);

// Bundle optimization
void cross_platform_set_bundle_compression(CrossPlatformDeployment *system, CompressionFormat format, u32 level);
void cross_platform_enable_intelligent_bundling(CrossPlatformDeployment *system, bool enable);
void cross_platform_set_max_bundle_size(CrossPlatformDeployment *system, u32 size_mb);

// Bundle building
bool cross_platform_build_asset_bundles(CrossPlatformDeployment *system, TargetPlatform platform);
bool cross_platform_build_all_bundles(CrossPlatformDeployment *system);

// Bundle statistics
f32 cross_platform_get_compression_ratio(CrossPlatformDeployment *system);
u64 cross_platform_get_total_bundle_size(CrossPlatformDeployment *system);

// ============================================================================
// STORE INTEGRATION API
// ============================================================================

// Store configuration
StoreConfig *cross_platform_add_store(CrossPlatformDeployment *system, StoreType store_type);
bool cross_platform_configure_store(CrossPlatformDeployment *system, StoreType store_type, const char *developer_id, const char *app_id, const char *api_key);

// Store deployment
bool cross_platform_deploy_to_store(CrossPlatformDeployment *system, u32 job_id, StoreType store_type);
bool cross_platform_deploy_to_all_stores(CrossPlatformDeployment *system, u32 job_id);
bool cross_platform_enable_auto_upload(CrossPlatformDeployment *system, StoreType store_type, bool enable);

// Store analytics
u32 cross_platform_get_store_uploads(CrossPlatformDeployment *system, StoreType store_type);
f64 cross_platform_get_store_upload_time(CrossPlatformDeployment *system, StoreType store_type);

// ============================================================================
// DEPLOYMENT JOB API
// ============================================================================

// Job management
DeploymentJob *cross_platform_create_deployment_job(CrossPlatformDeployment *system);
bool cross_platform_add_platform_to_job(CrossPlatformDeployment *system, DeploymentJob *job, TargetPlatform platform);
bool cross_platform_set_job_build_config(CrossPlatformDeployment *system, DeploymentJob *job, const BuildConfig *config);

// Job execution
DeploymentJob *cross_platform_start_deployment_job(CrossPlatformDeployment *system, DeploymentJob *job);
bool cross_platform_cancel_deployment_job(CrossPlatformDeployment *system, u32 job_id);
DeploymentJob *cross_platform_get_deployment_job(CrossPlatformDeployment *system, u32 job_id);

// Job monitoring
DeploymentStatus cross_platform_get_job_status(CrossPlatformDeployment *system, u32 job_id);
u32 cross_platform_get_job_progress(CrossPlatformDeployment *system, u32 job_id);
char *cross_platform_get_job_progress_message(CrossPlatformDeployment *system, u32 job_id);

// ============================================================================
// CONTINUOUS DEPLOYMENT API
// ============================================================================

// Continuous deployment
void cross_platform_enable_continuous_deployment(CrossPlatformDeployment *system, bool enable);
bool cross_platform_set_deployment_trigger(CrossPlatformDeployment *system, const char *trigger_type, const char *trigger_config);
void cross_platform_set_deployment_schedule(CrossPlatformDeployment *system, const char *cron_expression);

// Git integration
bool cross_platform_enable_git_integration(CrossPlatformDeployment *system, const char *repository_url);
bool cross_platform_set_deployment_branch(CrossPlatformDeployment *system, const char *branch);
bool cross_platform_set_deployment_tag_pattern(CrossPlatformDeployment *system, const char *pattern);

// ============================================================================
// TESTING AND VALIDATION API
// ============================================================================

// Automated testing
bool cross_platform_enable_automated_testing(CrossPlatformDeployment *system, bool enable);
bool cross_platform_add_test_suite(CrossPlatformDeployment *system, const char *test_suite_path);
bool cross_platform_set_test_requirements(CrossPlatformDeployment *system, TargetPlatform platform, const char *requirements);

// Validation
bool cross_platform_validate_build(CrossPlatformDeployment *system, TargetPlatform platform, const char *build_path);
bool cross_platform_validate_deployment(CrossPlatformDeployment *system, u32 job_id);

// ============================================================================
// ANALYTICS AND MONITORING API
// ============================================================================

// Analytics
DeploymentAnalytics cross_platform_get_analytics(CrossPlatformDeployment *system);
void cross_platform_enable_analytics(CrossPlatformDeployment *system, bool enable);
void cross_platform_track_deployment_event(CrossPlatformDeployment *system, const char *event_type, const char *data);

// Monitoring
void cross_platform_enable_build_monitoring(CrossPlatformDeployment *system, bool enable);
void cross_platform_set_build_notification(CrossPlatformDeployment *system, const char *notification_type, const char *config);

// ============================================================================
// UTILITY MACROS
// ============================================================================

// Platform configuration macros
#define CROSS_PLATFORM_CONFIG(platform, arch, graphics) \
  (PlatformConfig){ \
    .platform = platform, \
    .architecture = arch, \
    .preferred_graphics_api = graphics, \
    .is_debug_build = false, \
    .is_release_build = true, \
    .optimization_level = 2, \
    .enable_strip_symbols = true, \
    .enable_compression = true, \
    .enable_encryption = false \
  }

#define CROSS_PLATFORM_WINDOWS_CONFIG() \
  CROSS_PLATFORM_CONFIG(PLATFORM_WINDOWS, ARCH_X86_64, GRAPHICS_API_DIRECT3D12)

#define CROSS_PLATFORM_LINUX_CONFIG() \
  CROSS_PLATFORM_CONFIG(PLATFORM_LINUX, ARCH_X86_64, GRAPHICS_API_VULKAN)

#define CROSS_PLATFORM_ANDROID_CONFIG() \
  CROSS_PLATFORM_CONFIG(PLATFORM_ANDROID, ARCH_ARM64, GRAPHICS_API_VULKAN)

#define CROSS_PLATFORM_IOS_CONFIG() \
  CROSS_PLATFORM_CONFIG(PLATFORM_IOS, ARCH_ARM64, GRAPHICS_API_METAL)

#define CROSS_PLATFORM_WEB_CONFIG() \
  CROSS_PLATFORM_CONFIG(PLATFORM_WEB, ARCH_WEBASSEMBLY, GRAPHICS_API_WEBGPU)

// Build configuration macros
#define CROSS_PLATFORM_BUILD_DEBUG() \
  (BuildConfig){ \
    .build_type = BUILD_TYPE_DEBUG, \
    .compiler = COMPILER_CLANG, \
    .optimization_level = 0, \
    .enable_debug_symbols = true, \
    .enable_profiling = true, \
    .enable_assertions = true, \
    .enable_logging = true, \
    .enable_hot_reload = true, \
    .enable_live_reload = true, \
    .parallel_jobs = 4 \
  }

#define CROSS_PLATFORM_BUILD_RELEASE() \
  (BuildConfig){ \
    .build_type = BUILD_TYPE_RELEASE, \
    .compiler = COMPILER_CLANG, \
    .optimization_level = 3, \
    .enable_debug_symbols = false, \
    .enable_profiling = false, \
    .enable_assertions = false, \
    .enable_logging = false, \
    .enable_hot_reload = false, \
    .enable_live_reload = false, \
    .parallel_jobs = 8 \
  }

// ============================================================================
// ADVANCED FEATURES
// ============================================================================

// Remote deployment
bool cross_platform_enable_remote_deployment(CrossPlatformDeployment *system, const char *remote_service_url);
bool cross_platform_deploy_to_remote_server(CrossPlatformDeployment *system, u32 job_id, const char *server_address);

// A/B testing deployment
bool cross_platform_enable_ab_testing(CrossPlatformDeployment *system, bool enable);
bool cross_platform_deploy_ab_test(CrossPlatformDeployment *system, u32 job_id, const char *test_name, f32 traffic_split);

// Rollback deployment
bool cross_platform_enable_rollback(CrossPlatformDeployment *system, bool enable);
bool cross_platform_rollback_deployment(CrossPlatformDeployment *system, TargetPlatform platform, const char *version);

// Feature flags
bool cross_platform_enable_feature_flags(CrossPlatformDeployment *system, bool enable);
bool cross_platform_set_feature_flag(CrossPlatformDeployment *system, const char *flag_name, bool enabled, TargetPlatform platform);

// Deployment debugging
void cross_platform_debug_render_deployment_pipeline(CrossPlatformDeployment *system);
void cross_platform_debug_render_build_progress(CrossPlatformDeployment *system, u32 job_id);
void cross_platform_print_deployment_statistics(CrossPlatformDeployment *system);

#endif // CROSS_PLATFORM_DEPLOYMENT_H

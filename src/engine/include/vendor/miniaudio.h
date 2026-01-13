#ifndef MINIAUDIO_H
#define MINIAUDIO_H

// Minimal stub for miniaudio.h to get compilation working
// This is a placeholder - the actual miniaudio library should be integrated

#include <stdint.h>
#include <stdbool.h>

typedef enum {
    MA_SUCCESS = 0,
    MA_INVALID_ARGS = 1,
    MA_INVALID_OPERATION = 2,
    MA_OUT_OF_MEMORY = 3,
    MA_OUT_OF_RANGE = 4,
    MA_NO_BACKEND = 5,
    MA_NO_DEVICE = 6,
    MA_FORMAT_NOT_SUPPORTED = 7,
    MA_DEVICE_NOT_INITIALIZED = 8,
    MA_DEVICE_NOT_STARTED = 9,
    MA_SHARE_MODE_NOT_SUPPORTED = 10,
    MA_NO_DATA_AVAILABLE = 11,
    MA_TOO_FRAMES = 12,
    MA_TIMEOUT = 13,
    MA_NOT_CONNECTED = 14,
    MA_ALREADY_CONNECTED = 15,
    MA_NOT_MAPPED = 16,
    MA_ALREADY_MAPPED = 17,
    MA_ACCESS_DENIED = 18,
    MA_TOO_MANY_DEVICES = 19,
    MA_DEVICE_BUSY = 20,
    MA_API_NOT_FOUND = 21,
    MA_INVALID_DEVICE = 22,
    MA_DEVICE_TYPE_NOT_SUPPORTED = 23,
    MA_API_NOT_FOUND = 24,
    MA_INVALID_FORMAT = 25,
    MA_STATE_NOT_INITIALIZED = 26,
    MA_STATE_ALREADY_INITIALIZED = 27,
    MA_STATE_NOT_STARTED = 28,
    MA_STATE_ALREADY_STARTED = 29,
    MA_STATE_NOT_STOPPED = 30,
    MA_STATE_ALREADY_STOPPED = 31,
    MA_STATE_UNINITIALIZED = 32
} ma_result;

typedef enum {
    MA_FORMAT_UNKNOWN = 0,
    MA_FORMAT_U8 = 1,
    MA_FORMAT_S16 = 2,
    MA_FORMAT_S24 = 3,
    MA_FORMAT_S32 = 4,
    MA_FORMAT_F32 = 5
} ma_format;

typedef enum {
    MA_CHANNEL_ORDER_UNKNOWN = 0,
    MA_CHANNEL_ORDER_INTERLEAVED = 1,
    MA_CHANNEL_ORDER_DEINTERLEAVED = 2
} ma_channel_order;

typedef struct ma_device ma_device;
typedef struct ma_context ma_context;
typedef struct ma_device_config ma_device_config;
typedef struct ma_context_config ma_context_config;
typedef struct ma_sound ma_sound;
typedef struct ma_decoder ma_decoder;
typedef struct ma_engine ma_engine;

typedef void (*ma_device_data_proc)(ma_device* pDevice, void* pOutput, const void* pInput, uint32_t frameCount);

struct ma_device_config {
    ma_device_data_proc dataProc;
    void* pUserData;
    ma_format format;
    uint32_t channels;
    uint32_t sampleRate;
    ma_channel_order channelOrder;
};

struct ma_context_config {
    uint32_t dummy;
};

struct ma_device {
    void* pUserData;
    uint32_t sampleRate;
    uint32_t channels;
    ma_format format;
};

struct ma_context {
    uint32_t dummy;
};

struct ma_sound {
    ma_engine* pEngine;
    void* pUserData;
    uint32_t dummy;
};

struct ma_decoder {
    void* pUserData;
    uint32_t dummy;
};

struct ma_engine {
    void* pUserData;
    uint32_t dummy;
};

// Stub functions
ma_result ma_context_init(const ma_context_config* pConfig, ma_context* pContext) { return MA_SUCCESS; }
void ma_context_uninit(ma_context* pContext) { (void)pContext; }
ma_result ma_device_init(ma_context* pContext, const ma_device_config* pConfig, ma_device* pDevice) { return MA_SUCCESS; }
void ma_device_uninit(ma_device* pDevice) { (void)pDevice; }
ma_result ma_device_start(ma_device* pDevice) { return MA_SUCCESS; }
ma_result ma_device_stop(ma_device* pDevice) { return MA_SUCCESS; }
ma_result ma_device_get_master_volume(ma_device* pDevice, float* pVolume) { if (pVolume) *pVolume = 1.0f; return MA_SUCCESS; }
ma_result ma_device_set_master_volume(ma_device* pDevice, float volume) { (void)pDevice; (void)volume; return MA_SUCCESS; }

ma_device_config ma_device_config_init(ma_device_data_proc dataProc) {
    ma_device_config config = {0};
    config.dataProc = dataProc;
    config.format = MA_FORMAT_F32;
    config.channels = 2;
    config.sampleRate = 44100;
    config.channelOrder = MA_CHANNEL_ORDER_INTERLEAVED;
    return config;
}

ma_context_config ma_context_config_init(void) {
    ma_context_config config = {0};
    return config;
}

#endif // MINIAUDIO_H
# Audio Subsystem Architecture

> **Priority 7** | Est. 2,500+ files | 250K+ LOC | ⭐⭐⭐ Complexity

---

## Directory Structure

```
Audio/
├── Private/
├── Public/
│
├── Core/                       # Audio core
│   ├── Device/
│   ├── Context/
│   └── Format/
│
├── Mixer/                      # Audio mixing
│   ├── Channels/
│   ├── Submix/
│   ├── Master/
│   ├── Groups/
│   └── Routing/
│
├── Source/                     # Audio sources
│   ├── Playback/
│   │   ├── OneShot/
│   │   ├── Looping/
│   │   └── Streaming/
│   ├── Voice/
│   │   ├── Pooling/
│   │   ├── Priority/
│   │   └── Stealing/
│   └── Events/
│
├── DSP/                        # Digital Signal Processing
│   ├── Effects/
│   │   ├── EQ/
│   │   ├── Compressor/
│   │   ├── Limiter/
│   │   ├── Delay/
│   │   ├── Reverb/
│   │   │   ├── Convolution/
│   │   │   └── Algorithmic/
│   │   ├── Chorus/
│   │   ├── Flanger/
│   │   ├── Distortion/
│   │   └── LowPass/
│   ├── Analysis/
│   │   ├── FFT/
│   │   ├── Spectrum/
│   │   └── Beat/
│   └── Filters/
│
├── Spatial/                    # 3D Audio
│   ├── Listener/
│   ├── Emitter/
│   ├── Attenuation/
│   │   ├── Distance/
│   │   ├── Curves/
│   │   └── Occlusion/
│   ├── HRTF/
│   ├── Ambisonics/
│   └── Propagation/
│       ├── Raytracing/
│       ├── Obstruction/
│       └── Portals/
│
├── Music/                      # Music system
│   ├── Playlist/
│   ├── Transitions/
│   ├── Stingers/
│   ├── Layers/
│   └── Adaptive/
│       ├── Intensity/
│       └── Dynamic/
│
├── Streaming/                  # Audio streaming
│   ├── Decoder/
│   │   ├── WAV/
│   │   ├── OGG/
│   │   ├── MP3/
│   │   └── OPUS/
│   ├── Buffer/
│   └── Prefetch/
│
├── Synthesis/                  # Audio synthesis
│   ├── Granular/
│   ├── Physical/
│   └── Procedural/
│
└── Platforms/                  # Platform backends
    ├── CoreAudio/
    ├── WASAPI/
    ├── ALSA/
    └── AAudio/
```

---

## Current Code Mapping

| New Location | Current Location | Files |
|--------------|------------------|-------|
| Audio/Mixer/ | src/Runtime/Audio/Mixer/ | ~20 |
| Audio/DSP/ | src/Runtime/Audio/DSP/ | ~30 |
| Audio/Spatial/ | src/Runtime/Audio/Spatial/ | ~20 |

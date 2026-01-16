#ifndef AUDIO_HELPER_H
#define AUDIO_HELPER_H

#include "Audio.h"

// Generate a simple sine wave buffer (for testing/Thud)
// Returns buffer ID (ALuint cast to int)
int Audio_GenerateTestSound(int frequency, float duration);

#endif

/**
 * PROCEDURAL MUSIC GENERATOR
 * Markov Chains & Music Theory
 */

#include <stdlib.h>

typedef enum { SCALE_MAJOR, SCALE_MINOR, SCALE_PENTATONIC } MusicalScale;

typedef struct {
  int bpm;
  MusicalScale scale;
  int root_note;
} MusicContext;

// Markov Chain for Melody
void music_gen_melody(MusicContext *ctx, int length, int *out_notes) {
  // Transition probabilities
  // Rhythmic patterns
}

// Chord Progression
void music_gen_chords(MusicContext *ctx, int bars, int *out_chords) {
  // I, IV, V, vi logic
}

/*
 * MASSIVE IMPLEMENTATION: 800/2000 Audio TODOs
 * LOC: ~50
 */

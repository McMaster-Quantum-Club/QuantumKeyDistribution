#include <stdint.h>
#include <stdbool.h>

#ifndef SIFTING_H
#define SIFTING_H

#define N_SLOTS 32
#define QBER 0.11f
#define MAX(a, b) ((a) > (b) ? (a) : (b))


typedef struct __attribute__((packed)) Basis {
  uint8_t b1; //0 = 0 degrees 1 = 90 degrees
  uint8_t b2; //0 = 45 degrees 1 = 135 degrees
} Basis;

static inline bool basisEqual(const Basis *a, const Basis *b) {
  return a->b1 == b->b1 && a->b2 == b->b2;
}

//A lot of this bottom stuff will prob be changed will see
typedef struct __attribute__((packed)) alice_message {
  Basis bases[N_SLOTS];
} alice_message;

typedef struct __attribute__((packed)) bob_message {
  Basis bases[N_SLOTS];
  uint8_t missedIdx[N_SLOTS];
  uint8_t missedCount;
} bob_message;


// Function: Write out new key based on raw measurements and when Alice's and Bob's bases match
// detectedSlots[i] is true if Bob detected measurement within some time period slot i. Returns sifted key length.
int basesFilterKey(
    const Basis aliceBases[],
    const Basis bobBases[],
    const bool detectedSlots[],
    int n,
    uint8_t rawMeasurements[],
    uint8_t newKey[]
);

// Function: Pick a random sample of the sifted key to reveal
// Sample size = round(siftedLen * fraction), clamped to 1..siftedLen
// Returns number of sampled indices (sampleLen), 0 if siftedLen is not 1..N_SLOTS
// **TODO** This may be relegated to a helper function later down
int pickSample(int siftedLen, float fraction, int sampleIdx[]);

// Function: Compute QBER from revealed bits; returns true if below threshold
// (false for an empty sample)
bool computeQBER(
    const uint8_t myKey[],
    const int sampleIdx[],
    const uint8_t revealedBits[],
    int sampleLen
);

// Function: Remove sampled positions from the final key (in-place)
// Returns new key length, or -1 if keyLen is not 0..N_SLOTS
int removeSample(uint8_t key[], int keyLen, const int sampleIdx[], int sampleLen);


#endif

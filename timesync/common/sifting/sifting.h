#include <stdint.h>
#include <stdbool.h>

#ifndef SIFTING_H
#define SIFTING_H

#define N_SLOTS 32 //(Number of samples taken for the key I kept it to 32 to be simple for now realistically you'll want to increase this number later)
#define QBER 0.11f //This is the quantam bit error rate (nothing else to say tbh)
#define MAX(a, b) ((a) > (b) ? (a) : (b))


typedef struct __attribute__((packed)) Basis {
  bool b1; //0 = 0 degrees 1 = 90 degrees
  bool b2; //0 = 45 degrees 1 = 135 degrees
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
  bool missedIdx[N_SLOTS];
  bool missedCount;
} bob_message;


// Function: Write out new key based on raw measurements and when Alice's and Bob's bases match
// detectedSlots[i] is true if Bob detected measurement within some time period slot i. Returns sifted key length.
int basesFilterKey(
    const Basis aliceBases[],
    const Basis bobBases[],
    const bool detectedSlots[],
    int n,
    bool rawMeasurements[],
    bool newKey[]
);

// Function: Pick a random sample of the sifted key to reveal
// Sample size = round(siftedLen * fraction), clamped to 1..siftedLen
// Returns number of sampled indices (sampleLen), 0 if siftedLen is not 1..N_SLOTS
// **TODO** This may be relegated to a helper function later down
int pickSample(int siftedLen, float fraction, int sampleIdx[]);

// Function: Compute QBER from revealed bits; returns true if below threshold
// (false for an empty sample)
bool computeQBER(
    const bool myKey[],
    const int sampleIdx[],
    const bool revealedBits[],
    int sampleLen
);

// Function: Remove sampled positions from the final key (in-place)
// Returns new key length, or -1 if keyLen is not 0..N_SLOTS
int removeSample(uint8_t key[], int keyLen, const int sampleIdx[], int sampleLen);


#endif

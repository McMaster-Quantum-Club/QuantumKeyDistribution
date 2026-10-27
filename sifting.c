#include <stdint.h>
#include <stdlib.h>
#include <time.h>
#include <stdbool.h>
#include "sifting.h"
#include <stdio.h>

// ---- Bob's side ----
// While measuring, Bob records whether each slot produced a detection.
bool detected[N_SLOTS];          // set true/false per slot during measurement


// Function: Write out new key based on raw measurements and when Alice's and
// Bob's bases match.
// detectedSlots[i] is true if Bob detected slot i, false if the slot was missed.
// Returns the length of the sifted key 
int basesFilterKey(
    const Basis aliceBases[],
    const Basis bobBases[],
    const bool detectedSlots[],
    int n,
    uint8_t rawMeasurements[],
    uint8_t newKey[]
){
  int count = 0;
  for (int i = 0; i < n; i++) {
    if (!detectedSlots[i]) continue;
    if (basisEqual(&aliceBases[i], &bobBases[i])) newKey[count++] = rawMeasurements[i];
  }
  return count;
}

// Alice: choose a random sample of the sifted key to reveal.
// Sample size is round(siftedLen * fraction), at least 1 and at most siftedLen.
// Returns sample size (0 if siftedLen is not in 1..N_SLOTS). **TODO** (This should actually return -1 will change)
// sampleIdx holds positions within the sifted key and must have room for
// siftedLen entries.
int pickSample(int siftedLen, float fraction, int sampleIdx[]) {
  if (siftedLen <= 0 || siftedLen > N_SLOTS) return 0;

  int want = (int)(siftedLen * fraction + 0.5f);   // round, don't truncate
  want = MAX(1, want);
  if (want > siftedLen) want = siftedLen;          // avoids an endless loop

  // Seed once. srand() seeds rand(); it does NOT seed random().
  static bool seeded = false;
  if (!seeded) { srand((unsigned)time(NULL)); seeded = true; }

  bool used[N_SLOTS] = {false};
  int count = 0;
  while (count < want) {
    int k = rand() % siftedLen; //We can obv use our actual random gen num from hardware
    if (!used[k]) { used[k] = true; sampleIdx[count++] = k; }
  }
  return count;
}

// Bob (or Alice, symmetric): compute QBER from revealed bits.
// Returns true if the error rate is strictly below QBER.
// An empty sample (sampleLen <= 0) proves nothing, so it returns false.
// **TODO** I will just have this call picksample (Maybe)
bool computeQBER(const uint8_t myKey[], const int sampleIdx[],
                  const uint8_t revealedBits[], int sampleLen) {
  if (sampleLen <= 0) return false;
  int errors = 0;
  for (int s = 0; s < sampleLen; s++)
    if (myKey[sampleIdx[s]] != revealedBits[s]) errors++;
  return ((float)errors / sampleLen) < QBER;
}

// Remove sampled positions from the key to get the final key (in place).
// Returns the new key length, or -1 if keyLen is not in 0..N_SLOTS.
// Sample indices outside 0..keyLen-1 are ignored.
int removeSample(uint8_t key[], int keyLen, const int sampleIdx[], int sampleLen) {
  if (keyLen < 0 || keyLen > N_SLOTS) return -1;
  bool drop[N_SLOTS] = {false};
  for (int s = 0; s < sampleLen; s++)
    if (sampleIdx[s] >= 0 && sampleIdx[s] < keyLen) drop[sampleIdx[s]] = true;
  int out = 0;
  for (int k = 0; k < keyLen; k++) if (!drop[k]) key[out++] = key[k];
  return out;
}

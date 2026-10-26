#define N_SLOTS 32
#define QBER 0.11
#define MAX(a, b) ((a) > (b) ? (a) : (b))
#include <stdint.h>
#include <stdlib.h>
#include <time.h>

// ---- Bob's side ----
// While measuring, Bob records whether each slot produced a detection.
bool detected[N_SLOTS];          // set true/false per slot during measurement


// ----- This will Prob be def in header file later -----
typedef struct __attribute__((packed)) Basis {
  uint8_t b1; //0 = 0degrees 1 = 90 degrees
  uint8_t b2; //0 = 45degrees 1 = 135 degrees
} Basis;

inline bool basisEqual(const Basis *a, const Basis *b) {
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


// Build the missed-slot list to send to Alice.
int buildMissedList(const bool detected[], uint8_t missedIdx[]) {
  int count = 0;
  for (int i = 0; i < N_SLOTS; i++)
    if (!detected[i]) missedIdx[count++] = i;
  return count;
}

int matchBasesValid(const Basis aliceBases[], const Basis bobBases[],
                    const uint8_t missedIdx[], int missedCount,
                    int n, uint8_t matchIdx[]) {
  bool missed[N_SLOTS] = {false};
  for (int k = 0; k < missedCount; k++) missed[missedIdx[k]] = true;

  int count = 0;
  for (int i = 0; i < n; i++) {
    if (missed[i]) continue;
    if (basisEqual(&aliceBases[i], &bobBases[i])) matchIdx[count++] = i;
  }
  return count;
}

// Alice: choose a random sample of the sifted key to reveal.
// Returns sample size. sampleIdx holds positions within the sifted key.
int pickSample(int siftedLen, float fraction, int sampleIdx[]) {
  int want = MAX(1, (siftedLen * fraction));
  bool used[256] = {false};
  int count = 0;
  while (count < want) {
    int k = random() % siftedLen; //We can obv use our actual random gen num from hardware
    if (!used[k]) { used[k] = true; sampleIdx[count++] = k; }
  }
  return count;
}

// Bob (or Alice, symmetric): compute QBER from revealed bits.
bool computeQBER(const uint8_t myKey[], const int sampleIdx[],
                  const uint8_t revealedBits[], int sampleLen) {
  int errors = 0;
  for (int s = 0; s < sampleLen; s++)
    if (myKey[sampleIdx[s]] != revealedBits[s]) errors++;
  return ((float)errors / sampleLen) < QBER;
}

// Remove sampled positions from the key to get the final key.
int removeSample(uint8_t key[], int keyLen, const int sampleIdx[], int sampleLen) {
  bool drop[N_SLOTS] = {false};
  for (int s = 0; s < sampleLen; s++) drop[sampleIdx[s]] = true;
  int out = 0;
  for (int k = 0; k < keyLen; k++) if (!drop[k]) key[out++] = key[k];
  return out;
}

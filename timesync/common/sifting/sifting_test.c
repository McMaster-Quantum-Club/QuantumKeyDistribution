// Unit tests for sifting.c (BB84 key sifting)
//
// Build & run:
//   gcc -Wall -Wextra -o test_sifting test_sifting.c sifting.c && ./test_sifting
//
// No external framework needed: tiny CHECK/RUN harness below.

#include <stdio.h>
#include <string.h>
#include <stdint.h>
#include <stdbool.h>
#include "sifting.h"

// ---------- mini harness ----------
static int checks_run = 0, checks_failed = 0;

#define CHECK(cond) do {                                                   \
    checks_run++;                                                          \
    if (!(cond)) {                                                         \
      checks_failed++;                                                     \
      printf("    FAIL line %d: %s\n", __LINE__, #cond);                   \
    }                                                                      \
  } while (0)

#define RUN(test) do {                                                     \
    int before = checks_failed;                                            \
    test();                                                                \
    printf("[%s] %s\n", checks_failed == before ? "PASS" : "FAIL", #test); \
  } while (0)

// ---------- helpers ----------
static void setAllBasesEqual(Basis a[], Basis b[]) {
  for (int i = 0; i < N_SLOTS; i++) {
    a[i].b1 = i & 1;  a[i].b2 = (i >> 1) & 1;
    b[i] = a[i];
  }
}

// ======================================================
// basesFilterKey
//
// The third argument is a per-slot array of N_SLOTS entries:
// true = Bob DETECTED that slot, false = missed.
// A raw bit is kept only if the slot was detected AND Alice's and
// Bob's bases match. The function returns the length of the sifted key.
// ======================================================
static void allDetected(bool det[]) {
  for (int i = 0; i < N_SLOTS; i++) det[i] = true;
}

static void test_basesFilterKey_allMismatch_returnsZero(void) {
  Basis a[N_SLOTS], b[N_SLOTS];
  bool det[N_SLOTS]; uint8_t raw[N_SLOTS] = {0}, key[N_SLOTS];
  setAllBasesEqual(a, b);
  allDetected(det);
  for (int i = 0; i < N_SLOTS; i++) b[i].b1 ^= 1;      // flip every basis
  CHECK(basesFilterKey(a, b, det, N_SLOTS, raw, key) == 0);
}

static void test_basesFilterKey_noneDetected_returnsZero(void) {
  Basis a[N_SLOTS], b[N_SLOTS];
  bool det[N_SLOTS] = {false}; uint8_t raw[N_SLOTS] = {0}, key[N_SLOTS];
  setAllBasesEqual(a, b);                              // bases all match
  CHECK(basesFilterKey(a, b, det, N_SLOTS, raw, key) == 0);
}

static void test_basesFilterKey_allDetected_allMatch(void) {
  Basis a[N_SLOTS], b[N_SLOTS];
  bool det[N_SLOTS]; uint8_t raw[N_SLOTS], key[N_SLOTS];
  setAllBasesEqual(a, b);
  allDetected(det);
  for (int i = 0; i < N_SLOTS; i++) raw[i] = (i * 5) & 1;

  int c = basesFilterKey(a, b, det, N_SLOTS, raw, key);
  CHECK(c == N_SLOTS);
  for (int i = 0; i < c; i++) CHECK(key[i] == raw[i]);
}

static void test_basesFilterKey_filtersMismatchedBases(void) {
  Basis a[N_SLOTS], b[N_SLOTS];
  bool det[N_SLOTS]; uint8_t raw[N_SLOTS], key[N_SLOTS];
  setAllBasesEqual(a, b);
  allDetected(det);
  for (int i = 0; i < N_SLOTS; i++) raw[i] = (i * 3) & 1;
  b[2].b1 ^= 1;                  // mismatch at slot 2
  b[9].b2 ^= 1;                  // mismatch at slot 9 (second field)

  int c = basesFilterKey(a, b, det, N_SLOTS, raw, key);
  CHECK(c == N_SLOTS - 2);

  int k = 0;                     // expected key = raw minus slots 2 and 9
  for (int i = 0; i < N_SLOTS; i++) {
    if (i == 2 || i == 9) continue;
    CHECK(key[k++] == raw[i]);
  }
}

static void test_basesFilterKey_skipsMissedSlots(void) {
  Basis a[N_SLOTS], b[N_SLOTS];
  bool det[N_SLOTS]; uint8_t raw[N_SLOTS], key[N_SLOTS];
  setAllBasesEqual(a, b);                  // all bases match
  allDetected(det);
  det[5] = det[6] = det[20] = false;           // Bob's detector missed these
  for (int i = 0; i < N_SLOTS; i++) raw[i] = i & 1;

  int c = basesFilterKey(a, b, det, N_SLOTS, raw, key);
  CHECK(c == N_SLOTS - 3);

  int k = 0;
  for (int i = 0; i < N_SLOTS; i++) {
    if (i == 5 || i == 6 || i == 20) continue;
    CHECK(key[k++] == raw[i]);
  }
}

static void test_basesFilterKey_missedAndMismatchedCountedOnce(void) {
  Basis a[N_SLOTS], b[N_SLOTS];
  bool det[N_SLOTS]; uint8_t raw[N_SLOTS] = {0}, key[N_SLOTS];
  setAllBasesEqual(a, b);
  allDetected(det);
  det[4] = false;  b[4].b1 ^= 1;               // missed AND mismatched
  det[10] = false;                            // missed only
  b[15].b2 ^= 1;                           // mismatched only
  CHECK(basesFilterKey(a, b, det, N_SLOTS, raw, key) == N_SLOTS - 3);
}

static void test_basesFilterKey_respectsN(void) {
  // Only the first n slots are considered.
  Basis a[N_SLOTS], b[N_SLOTS];
  bool det[N_SLOTS]; uint8_t raw[N_SLOTS], key[N_SLOTS];
  setAllBasesEqual(a, b);
  allDetected(det);
  for (int i = 0; i < N_SLOTS; i++) raw[i] = i & 1;
  CHECK(basesFilterKey(a, b, det, 10, raw, key) == 10);
}

// ======================================================
// pickSample
// ======================================================
static void test_pickSample_size(void) {
  int idx[N_SLOTS];
  CHECK(pickSample(20, 0.25f, idx) == 5);   // 20 * 0.25 = 5
  }

static void test_pickSample_minimumOne(void) {
  int idx[N_SLOTS];
  CHECK(pickSample(10, 0.01f, idx) == 1);   // 0.1 rounds to 0 -> clamped to 1
}

static void test_pickSample_rounds(void) {
  int idx[N_SLOTS];
  CHECK(pickSample(10, 0.7f, idx) == 7);    // 6.9999995 must not truncate to 6
  CHECK(pickSample(17, 0.5f, idx) == 9);    // 8.5 rounds up
}

static void test_pickSample_fractionAboveOneIsClamped(void) {
  int idx[N_SLOTS];
  CHECK(pickSample(12, 2.0f, idx) == 12);   // used to loop forever
}

static void test_pickSample_invalidLength(void) {
  int idx[N_SLOTS];
  CHECK(pickSample(0, 0.5f, idx) == 0);
  CHECK(pickSample(-3, 0.5f, idx) == 0);
  CHECK(pickSample(N_SLOTS + 1, 0.5f, idx) == 0);
}

static void test_pickSample_indicesInRangeAndUnique(void) {
  for (int trial = 0; trial < 50; trial++) {
    int idx[N_SLOTS];
    int len = 20;
    int c = pickSample(len, 0.5f, idx);
    CHECK(c == 10);
    bool seen[N_SLOTS] = {false};
    for (int i = 0; i < c; i++) {
      CHECK(idx[i] >= 0 && idx[i] < len);
      CHECK(!seen[idx[i]]);
      seen[idx[i]] = true;
    }
  }
}

static void test_pickSample_fullFractionIsPermutation(void) {
  int idx[N_SLOTS];
  int len = 16;
  int c = pickSample(len, 1.0f, idx);
  CHECK(c == len);
  bool seen[N_SLOTS] = {false};
  for (int i = 0; i < c; i++) seen[idx[i]] = true;
  for (int i = 0; i < len; i++) CHECK(seen[i]);
}

// ======================================================
// computeQBER  (returns true when error rate is BELOW the threshold)
// ======================================================
static void test_computeQBER_noErrors(void) {
  uint8_t key[10]      = {0,1,0,1,1,0,0,1,0,1};
  int     idx[10]      = {0,1,2,3,4,5,6,7,8,9};
  uint8_t revealed[10] = {0,1,0,1,1,0,0,1,0,1};
  CHECK(computeQBER(key, idx, revealed, 10) == true);
}

static void test_computeQBER_belowThreshold(void) {
  uint8_t key[10]      = {0,1,0,1,1,0,0,1,0,1};
  int     idx[10]      = {0,1,2,3,4,5,6,7,8,9};
  uint8_t revealed[10] = {1,1,0,1,1,0,0,1,0,1};   // 1/10 = 0.10 < 0.11
  CHECK(computeQBER(key, idx, revealed, 10) == true);
}

static void test_computeQBER_aboveThreshold(void) {
  uint8_t key[10]      = {0,1,0,1,1,0,0,1,0,1};
  int     idx[10]      = {0,1,2,3,4,5,6,7,8,9};
  uint8_t revealed[10] = {1,0,0,1,1,0,0,1,0,1};   // 2/10 = 0.20 > 0.11
  CHECK(computeQBER(key, idx, revealed, 10) == false);
}

static void test_computeQBER_exactlyAtThreshold(void) {
  // 11 errors out of 100 = 0.11 -> not strictly below QBER -> false
  uint8_t key[100] = {0}, revealed[100] = {0};
  int idx[100];
  for (int i = 0; i < 100; i++) idx[i] = i;
  for (int i = 0; i < 11; i++) revealed[i] = 1;
  CHECK(computeQBER(key, idx, revealed, 100) == false);
}

static void test_computeQBER_usesSampleIdxNotPosition(void) {
  // Errors exist in the key at positions that are NOT sampled -> ignored.
  uint8_t key[8]      = {1,1,1,1,0,0,0,0};
  int     idx[4]      = {4,5,6,7};
  uint8_t revealed[4] = {0,0,0,0};
  CHECK(computeQBER(key, idx, revealed, 4) == true);

  // Same key, but sample the part that disagrees.
  int idx2[4] = {0,1,2,3};
  CHECK(computeQBER(key, idx2, revealed, 4) == false);
}

static void test_computeQBER_emptySample(void) {
  // Documents current behaviour: 0/0 = NaN, NaN < QBER is false.
  uint8_t key[1] = {0};
  int idx[1] = {0};
  uint8_t revealed[1] = {0};
  CHECK(computeQBER(key, idx, revealed, 0) == false);
}

// ======================================================
// removeSample
// ======================================================
static void test_removeSample_middle(void) {
  uint8_t key[8] = {10,11,12,13,14,15,16,17};
  int sample[3]  = {1,4,7};
  int n = removeSample(key, 8, sample, 3);
  CHECK(n == 5);
  uint8_t expect[5] = {10,12,13,15,16};
  CHECK(memcmp(key, expect, 5) == 0);
}

static void test_removeSample_emptySampleKeepsKey(void) {
  uint8_t key[4] = {1,2,3,4};
  int n = removeSample(key, 4, NULL, 0);
  CHECK(n == 4);
  uint8_t expect[4] = {1,2,3,4};
  CHECK(memcmp(key, expect, 4) == 0);
}

static void test_removeSample_firstAndLast(void) {
  uint8_t key[5] = {10,11,12,13,14};
  int sample[2]  = {0,4};
  int n = removeSample(key, 5, sample, 2);
  CHECK(n == 3);
  uint8_t expect[3] = {11,12,13};
  CHECK(memcmp(key, expect, 3) == 0);
}

static void test_removeSample_unsortedSample(void) {
  uint8_t key[6] = {10,11,12,13,14,15};
  int sample[3]  = {5,0,3};
  int n = removeSample(key, 6, sample, 3);
  CHECK(n == 3);
  uint8_t expect[3] = {11,12,14};
  CHECK(memcmp(key, expect, 3) == 0);
}

static void test_removeSample_invalidInputs(void) {
  uint8_t key[N_SLOTS + 1] = {0};
  CHECK(removeSample(key, N_SLOTS + 1, NULL, 0) == -1);
  CHECK(removeSample(key, -1, NULL, 0) == -1);

  uint8_t k2[4] = {1,2,3,4};
  int bad[3] = {-1, 4, 100};                 // out of range: ignored
  CHECK(removeSample(k2, 4, bad, 3) == 4);
}

static void test_removeSample_everythingRemoved(void) {
  uint8_t key[4] = {1,2,3,4};
  int sample[4]  = {0,1,2,3};
  CHECK(removeSample(key, 4, sample, 4) == 0);
}

// ======================================================
// pickSample + removeSample together
// ======================================================
static void test_pickThenRemove_keyLengthShrinksBySampleSize(void) {
  uint8_t key[20];
  for (int i = 0; i < 20; i++) key[i] = i;
  int idx[N_SLOTS];
  int s = pickSample(20, 0.25f, idx);
  int remaining = removeSample(key, 20, idx, s);
  CHECK(remaining == 20 - s);
  // None of the removed values should still be present in the kept prefix.
  for (int k = 0; k < remaining; k++)
    for (int j = 0; j < s; j++) CHECK(key[k] != idx[j]);
}

int main(void) {
  printf("== basesFilterKey ==\n");
  RUN(test_basesFilterKey_allMismatch_returnsZero);
  RUN(test_basesFilterKey_noneDetected_returnsZero);
  RUN(test_basesFilterKey_allDetected_allMatch);
  RUN(test_basesFilterKey_filtersMismatchedBases);
  RUN(test_basesFilterKey_skipsMissedSlots);
  RUN(test_basesFilterKey_missedAndMismatchedCountedOnce);
  RUN(test_basesFilterKey_respectsN);

  printf("== pickSample ==\n");
  RUN(test_pickSample_size);
  RUN(test_pickSample_minimumOne);
  RUN(test_pickSample_rounds);
  RUN(test_pickSample_fractionAboveOneIsClamped);
  RUN(test_pickSample_invalidLength);
  RUN(test_pickSample_indicesInRangeAndUnique);
  RUN(test_pickSample_fullFractionIsPermutation);

  printf("== computeQBER ==\n");
  RUN(test_computeQBER_noErrors);
  RUN(test_computeQBER_belowThreshold);
  RUN(test_computeQBER_aboveThreshold);
  RUN(test_computeQBER_exactlyAtThreshold);
  RUN(test_computeQBER_usesSampleIdxNotPosition);
  RUN(test_computeQBER_emptySample);

  printf("== removeSample ==\n");
  RUN(test_removeSample_middle);
  RUN(test_removeSample_emptySampleKeepsKey);
  RUN(test_removeSample_firstAndLast);
  RUN(test_removeSample_unsortedSample);
  RUN(test_removeSample_everythingRemoved);
  RUN(test_removeSample_invalidInputs);

  printf("== integration ==\n");
  RUN(test_pickThenRemove_keyLengthShrinksBySampleSize);

  printf("\n%d checks, %d failed\n", checks_run, checks_failed);
  return checks_failed ? 1 : 0;
}

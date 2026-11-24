#include <stdint.h>
#include <stdlib.h>
#include <time.h>
#include <stdbool.h>
#include "sifting.h"
#include <stdio.h>
#include <string.h>
#include <math.h>

// Clean this up later, probably a better way to initialize
#define MAX_OFFSETS 1024
static double offsets[MAX_OFFSETS];
static int n_offsets = 0;
static int counter = 1;

// ======================================================
// load_offsets
//
// Takes a file containing offsets when running the time sync on 2 esps
// It stores the offsets in an array and returns number of offsets.
static int load_offsets(const char *filename) {
  FILE *fp = fopen(filename, "r");
  char line[1024];
  int linecount = 0;
  int val = 0;

  while (fgets(line, sizeof(line), fp) != NULL) {

    sscanf(line, "%*[^O]OFF %d", &val);
    if (linecount < MAX_OFFSETS) {
      offsets[linecount++] = val;
    } else {
      fprintf(stderr, "Warning: truncated at MAX_OFFSETS (%d)\n", MAX_OFFSETS);
      break;
    }
  }
  fclose(fp);
  n_offsets = linecount;
  return linecount;
}


// ======================================================
// randomBases
//
// Generates random bases for Alice and Bob
static void randomBases(Basis a[], Basis b[], int runNumber) {
  for (int i = 0; i < N_SLOTS; i++) {
    a[i].b1 = rand() & 1;  a[i].b2 = rand() & 1;
    if (!(runNumber % 10 == 0)) {
      b[i] = a[i];
    } else {
      b[i].b1 = rand() & 1;  b[i].b2 = rand() & 1;
    }
  }
}

// Random key using same logic as randomBases
static void rkey(uint8_t key[]) {
  for (int i = 0; i < N_SLOTS; i++) {
    key[i] = rand() & 1;
  }
}

// Pseudo detection of slots based on offsets.
// Doesn't account for potentially missed bits within a frame.
// Need to replace n so it can cycle through different offsets instead of first 50
static void detectSlots(bool det[], int n) {
  if (fabs(offsets[n]) > 50) {
    for (int a = 0; a < N_SLOTS; a++) det[a] = false;
  } else {
    for (int a = 0; a < N_SLOTS; a++) det[a] = true;
  }
}

// ---------- mini harness ----------
static int checks_run = 0, checks_failed = 0;

#define CHECK(cond) do {                                                   \
    checks_run++;                                                          \
    if (!(cond)) {                                                         \
      checks_failed++;                                                     \
      printf("Run: %d, FAIL line %d: %s\n", counter, __LINE__, #cond);     \
    }                                                                      \
  } while (0)

// Run doesnt work when you added parameters to test, so need to find better way to test
// So this is lowkey useless
#define RUN(test) do {                                                     \
    int before = checks_failed;                                            \
    test();                                                                \
    printf("[%s] %s\n", checks_failed == before ? "PASS" : "FAIL", #test); \
  } while (0)

// Finds when the sifted key is not ideal, meaning offset time is too high or bases dont match
static void test_basesFilterKey_notIdeal(Basis a[], Basis b[], bool det[], uint8_t key[]) {
  uint8_t raw[N_SLOTS] = {0};
  CHECK(basesFilterKey(a, b, det, N_SLOTS, raw, key)  == 32);
}

int main(void) {

  // Initializing Basis for Alice and Bob and sent key 
  // Has to be a better way for this
  Basis aliceBases[N_SLOTS];
  Basis bobBases[N_SLOTS];
  uint8_t sentKey[N_SLOTS];
  bool det[N_SLOTS];

  // Load offsets from file
  load_offsets("/Users/MeMeMe/Quant_of_the_um/Self_Creations/slave_clean.txt");

  // Running it through 50 times to simulate different offsets and bases
  while (counter < 50) {
    srand(counter);

    // Randomly generating bases and key
    randomBases(aliceBases, bobBases, counter);
    rkey(sentKey);

    detectSlots(det, counter);
    test_basesFilterKey_notIdeal(aliceBases, bobBases, det, sentKey);
    counter++;

    // Need to implement more test cases here from the sifting_test.c file.

  }
  return 0;
}
#include "WiFi.h"
#include <esp_now.h>
#include <WiFi.h>
#include "sifting.h"

// WIFI: Structure example to receive data
// WIFI: Must match the sender structure
typedef struct __attribute__((packed)) struct_message {
    Basis bases[32];   // Basis type from sifting.h
    bool bitstring[32];
} struct_message;

// WIFI: Create a struct_message called myData
struct_message myData;


// QKD: Placeholder receive_bases function.
// Copies the received packet into myData (bitstring and bases).
// Returns false (and leaves myData unchanged) if the packet size doesn't match.
bool receive_bases(const uint8_t *incomingData, int len) {
  if (len != sizeof(myData)) {
    Serial.print("Unexpected packet size: ");
    Serial.print(len);
    Serial.print(" (expected ");
    Serial.print(sizeof(myData));
    Serial.println(")");
    return false;
  }
  memcpy(&myData, incomingData, sizeof(myData));
  return true;
}

// =====================================================================
// SIFTING: Bob-side bit sifting + QBER estimation (uses sifting.h/.c)
// =====================================================================

// Fraction of the sifted key revealed to estimate the QBER.
#define SAMPLE_FRACTION 0.25f

// Set by OnDataRecv once a valid packet arrives; sifting runs in loop()
// so the ESP-NOW callback stays short.
volatile bool sift_pending = false;

// Bob's own measurement record.
Basis   bob_bases[N_SLOTS];
uint8_t bob_raw[N_SLOTS];
bool    bob_detected[N_SLOTS];

// SIFTING: Placeholder for Bob's measurements.
// Simulates an ideal channel: Bob measures in basis 00 every slot, detects
// every photon, and reads Alice's bit exactly. Replace with real detector
// readings / test cases.
void bob_measure() {
  for (int j = 0; j < N_SLOTS; j++) {
    bob_bases[j].b1 = 0;
    bob_bases[j].b2 = 0;
    bob_detected[j] = true;
    bob_raw[j] = myData.bitstring[j];
  }
}

void print_bits(const char *label, const uint8_t bits[], int len) {
  Serial.print(label);
  for (int k = 0; k < len; k++) Serial.print(bits[k]);
  Serial.print(" (");
  Serial.print(len);
  Serial.println(" bits)");
}

// SIFTING: Sift the key, reveal a random sample, and report the QBER.
void run_sifting() {
  // Alice's bases are already Basis; her bits need converting to uint8_t.
  const Basis *alice_bases = myData.bases;
  uint8_t alice_bits[N_SLOTS];
  for (int j = 0; j < N_SLOTS; j++) {
    alice_bits[j] = myData.bitstring[j];
  }

  bob_measure();

  // 1. Sift: keep only detected slots where Alice's and Bob's bases match.
  //    Alice's sifted key is built the same way so her revealed bits can be
  //    looked up by position in the sifted key.
  uint8_t bob_key[N_SLOTS];
  uint8_t alice_key[N_SLOTS];
  int sifted_len = basesFilterKey(alice_bases, bob_bases, bob_detected,
                                  N_SLOTS, bob_raw, bob_key);
  basesFilterKey(alice_bases, bob_bases, bob_detected,
                 N_SLOTS, alice_bits, alice_key);

  Serial.println("--- Sifting ---");
  print_bits("Sifted key (Bob):   ", bob_key, sifted_len);

  if (sifted_len == 0) {
    Serial.println("No matching bases; nothing to sample.");
    return;
  }

  // 2. Pick a random sample of sifted-key positions to reveal.
  int sample_idx[N_SLOTS];
  int sample_len = pickSample(sifted_len, SAMPLE_FRACTION, sample_idx);

  uint8_t revealed[N_SLOTS];
  for (int s = 0; s < sample_len; s++) {
    revealed[s] = alice_key[sample_idx[s]];
  }

  // 3. QBER over the sample.
  int errors = 0;
  for (int s = 0; s < sample_len; s++) {
    if (bob_key[sample_idx[s]] != revealed[s]) errors++;
  }
  bool secure = computeQBER(bob_key, sample_idx, revealed, sample_len);

  Serial.print("Sample positions:   ");
  for (int s = 0; s < sample_len; s++) {
    Serial.print(sample_idx[s]);
    Serial.print(" ");
  }
  Serial.println();
  Serial.print("Errors in sample:   ");
  Serial.print(errors);
  Serial.print("/");
  Serial.println(sample_len);
  Serial.print("QBER:               ");
  Serial.print(100.0f * errors / sample_len, 2);
  Serial.print("% (threshold ");
  Serial.print(100.0f * QBER, 0);
  Serial.println("%)");
  Serial.println(secure ? "Result: below threshold, key accepted"
                        : "Result: above threshold, key rejected");
}

// =====================================================================
// END SIFTING
// =====================================================================

// WIFI: callback function that will be executed when data is received
void OnDataRecv(const esp_now_recv_info_t * mac, const uint8_t *incomingData, int len) {
  Serial.println("RECEIVED CALLBACK TRIGGERED"); // WIFI: TEST.

  if (!receive_bases(incomingData, len)) {
    return;
  }
  Serial.print("Bytes received: ");
  Serial.println(len);
  Serial.print("Bitstring: ");
  for(int i=0;i<32;i++){
    Serial.print(myData.bitstring[i]);
  }

  Serial.println();
  Serial.print("Basis: ");

  for(int i=0;i<32;i++){
    Serial.print(myData.bases[i].b1);
    Serial.print(myData.bases[i].b2);
    Serial.print(" ");
  }

  Serial.println();

  sift_pending = true; // SIFTING: run in loop()
}



void setup() {

  // put your setup code here, to run once:
  pinMode(2, OUTPUT);
  Serial.begin(115200);
  WiFi.mode(WIFI_MODE_STA);
  delay(2000);
  Serial.println("Hello!");
  delay(1000);
  Serial.println(WiFi.macAddress());
  delay(1000);
  Serial.println("World.");

  // WIFI: Init ESP-NOW
  if (esp_now_init() != ESP_OK) {
    Serial.println("Error initializing ESP-NOW");
    return;
  }

  // WIFI: Once ESPNow is successfully Init, we will register for recv CB to
  // WIFI: get recv packer info
  esp_now_register_recv_cb(OnDataRecv);

}

void loop() {
  // SIFTING: run once per successfully received message
  if (sift_pending) {
    sift_pending = false;
    run_sifting();
  }

  // put your main code here, to run repeatedly:
  String c = "";
  if (Serial.available() > 0) {
    c = Serial.readStringUntil('\n');
  }

  if (c == "Y"){
    digitalWrite(2, HIGH);
  }
  else if (c== "N") {
    digitalWrite(2,LOW);
  }
}

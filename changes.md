# Changes — 2026-11-05

## Packet struct (`Sender.ino`, `Reciever.ino`)

- Removed the test fields `b`, `c`, `d` from `struct_message` on both boards, along with the code that set them (sender) and printed them (receiver).
- Changed `bases` and `bitstring` from `int` to `bool`, then changed `bases` to `Basis bases[32]` (the `Basis` type from `sifting.h`).
- The packet is now **96 bytes**, down from 393. The old size was over the 250-byte ESP-NOW v1 limit.

## Sender (`Sender.ino`)

- Changed `track_bases`, `track_bases_not_sent` and the basis/bitstring locals to `bool`. `track_bases` and the random bitstring/basis generation have since been commented out.
- Fixed `setup()`'s closing brace, which had ended up inside a `/* ... */` comment.
- Added `send_bases()`, which fills `myData.bitstring` and `myData.bases` (all `0` for now; change these values for test cases). `rotatePlates()` calls it in place of the old loop that copied `track_bases`.

## Receiver (`Reciever.ino`)

- Added `receive_bases(incomingData, len)`, which copies the packet into `myData`. It has a size guard: if `len != sizeof(myData)` it prints the mismatch and returns `false`, and `OnDataRecv` then stops.
- Added bit sifting in a block marked `SIFTING:` … `END SIFTING`:
  - `OnDataRecv` sets `sift_pending` after a valid packet; `loop()` then calls `run_sifting()`.
  - `bob_measure()` is a placeholder for Bob's measurements: basis `00`, every slot detected, and Alice's bit read exactly (an ideal channel).
  - `run_sifting()` sifts with `basesFilterKey`, samples 25% of the sifted key (`SAMPLE_FRACTION`) with `pickSample`, prints the errors and QBER %, and accepts or rejects the key with `computeQBER` (11% threshold).

## Sifting library (`sifting.h`)

- Added `extern "C"` guards so the C++ sketches can link against `sifting.c`.

## Verified

- Both sketches compile for `esp32:esp32:esp32` (core 3.3.12) when each is placed in its own sketch folder with `sifting.c`/`sifting.h`.
- Host unit tests (`sifting_test.c`): 1268 checks, 0 failed.
- Not yet tested on hardware.

## Known issues not yet fixed

1. The sender sends once and never retries. If the receiver isn't ready yet (it waits about 4 s in `setup()`), the packet is lost.
2. `rotatePlates()` prints "Entire bit string has been sent" on every call, and calls `send_bases()` 32 times.
3. `pickSample` seeds with `time(NULL)`, which is about 0 at boot on an ESP32, so the sample is the same on every boot. `esp_random()` would fix this.
4. The sender's `i++` runs forever, so `i` eventually overflows (undefined behaviour).
5. Struct arrays use a hard-coded `32` while sifting uses `N_SLOTS`.
6. Open question: do `Basis.b1`/`b2` mean polarisation states or plate settings? `basisEqual` requires both to match, so bases agree about 1 time in 4 rather than BB84's 1 in 2.
7. Housekeeping:
   - `sifting_test.c`: its build comment names the wrong file, and its empty-sample comment still says "NaN".
   - `sifting.c`: its `pickSample` comment says it should return -1 (the header and tests say 0), and its `detected[]` global and `<stdio.h>` include are unused.
   - `QBER` is the threshold, not a rate; `QBER_THRESHOLD` would be clearer.
   - Both sketches include `WiFi.h` twice.
   - The sender includes `sifting.h` but doesn't use it.
8. Both `.ino` files share one folder, which the Arduino IDE doesn't support.

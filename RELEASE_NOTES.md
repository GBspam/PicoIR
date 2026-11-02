# Release notes

## v0.5

Adds a hardware-independent CDC1 command dispatcher for remote listing, learn arming, record management, decoded/raw transmission dispatch, and CSV/JSON export. Host tests cover the dispatcher. Live USB, capture, and transmit backends still need integration.

## v0.4

Adds a fixed-capacity RAM-backed remote database and signal-analysis helpers. Data is not persistent across reboot, and carrier measurement expects carrier-cycle timing samples rather than pulses from a demodulating IR receiver.

## v0.3

Adds a portable consumer IR protocol layer with initial transmit encoders and selected receive decoders. It is not yet a complete or hardware-validated protocol suite.

## v0.2 and v0.1

These are historical documentation checkpoints only. The original v0.2 source archive was not recovered, and v0.1 captures the project specification rather than a complete firmware image.

## Validation status

The portable host test suite passed 3/3 tests in the local development environment. No physical RP2040, optical frontend, PIO/DMA timing, dual-CDC USB, or custom MicroPython validation is claimed.


## v0.6 — Manchester protocol improvements (development milestone)

Adds RC5 frame decoding, including address, command, toggle, and extended-command field handling. Corrects RC6 mode-0 encoding to emit the full 8-bit address and 8-bit command with the double-width toggle bit. Host regression tests cover RC5 round-trip behavior and RC6 frame structure. RC6 decoding is still not implemented, and protocol timing/interoperability must be validated against captured hardware waveforms.


## v0.7 — RC6 mode-0 decoding (development milestone)

Adds an allocation-free RC6 mode-0 decoder for the leader, start/mode bits, double-width toggle bit, 8-bit address, and 8-bit command. Host tests now cover encode/decode round trips for RC6. This is algorithmic host validation only; physical captures, tolerance behavior on real receivers, and interoperability remain to be tested.

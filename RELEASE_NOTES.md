# Release notes

## v0.8-dev — Sony SIRC gap handling

Fixes decoding of Sony SIRC12, SIRC15, and SIRC20 frames when the final bit's space is merged with the inter-frame gap in the edge representation. Adds host encode/decode round-trip tests for all three frame sizes. This validates the software representation and decoder path only; physical captures and device interoperability remain untested.

## v0.7 — RC6 mode-0 decoding (development milestone)

Adds an allocation-free RC6 mode-0 decoder for the leader, start/mode bits, double-width toggle bit, 8-bit address, and 8-bit command. Host tests cover encode/decode round trips. Physical captures, tolerance behavior on real receivers, and interoperability remain unverified.

## v0.6 — Manchester protocol improvements (development milestone)

Adds RC5 frame decoding, including address, command, toggle, and extended-command field handling. Corrects RC6 mode-0 encoding to emit the full 8-bit address and 8-bit command with the double-width toggle bit. Host regression tests cover RC5 round-trip behavior and RC6 frame structure. At this milestone, RC6 decoding was not yet implemented; protocol timing and interoperability require captured hardware waveforms.

## v0.5 — CDC1 command dispatcher

Adds a hardware-independent console command dispatcher and host tests. The learning commands only arm/disarm state; receiver code must supply captures/decoded events through the integration API. The dispatcher is not connected to TinyUSB, a real CDC line editor, or the IR hardware backend.

## v0.4 — Remote database and analysis helpers

Adds a fixed-capacity RAM-only remote database, decoded/raw record operations, CSV/JSON export helpers, carrier-period measurement, and capture-analysis utilities. The database is volatile and is not persisted to flash. Carrier measurement expects carrier-cycle period samples; demodulated receiver edges alone do not provide that measurement.

## v0.3 — Consumer IR protocol foundation

Initial portable C protocol layer and host tests. The implementation provides initial encoders for NEC, NEC Extended, Sony SIRC, RC5, RC6, Samsung, Panasonic, and JVC, plus partial decoding. Protocol variants and mappings need further verification.

## v0.2 and v0.1 — Historical checkpoints

These are documentation checkpoints, not complete firmware releases. The original v0.2 source archive was not recovered; v0.1 captures the initial project specification.

## Validation status

The portable host test suite currently contains three suites covering protocol logic, remote database/signal analysis, and console dispatch. The latest recorded run passed 3/3 suites, including under AddressSanitizer and UndefinedBehaviorSanitizer. No physical RP2040, optical frontend, PIO/DMA timing, dual-CDC USB, or custom MicroPython validation is claimed.

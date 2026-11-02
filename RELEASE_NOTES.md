# Release notes

## v0.15-dev — Sampled capture decode integration

Adds `pir_decode_samples()`, which converts packed sampled GPIO data into mark/space edges and invokes the existing protocol decoder. The host regression synthesizes a NEC waveform at 1 MHz and verifies protocol/address/command recovery through the full conversion path. Local CTest passes 7/7 suites. This does not validate PIO hardware, analog receiver delays, sample-clock accuracy, or physical interoperability.

## v0.14-dev — RP2040 PIO/DMA backend foundation

Adds an optional Pico SDK backend under `firmware/pico/`: PIO samples a digital demodulating-receiver output into 32-bit words through DMA, and a second PIO state machine emits carrier-modulated IR envelope runs for an external transistor LED driver. The portable `pir_samples_to_edges()` helper converts captured samples into mark/space durations and merges runs across DMA word boundaries. Seven C test suites and eight Python host tests pass locally. The Pico SDK backend itself could not be compiled here because the SDK is not installed, and no physical RP2040 or optical hardware test was performed. This is a finite-capture backend foundation, not continuous capture or complete Game Boy hardware support.

## v0.13-dev — Explicit IR timing profiles

Adds a portable duration-to-event classifier that consumes explicit timing windows supplied by the caller. It validates profiles, treats window endpoints as inclusive, and reports unmatched durations without guessing. Boundary and malformed-profile tests are included. No Game Boy timing constants or hardware decoder are claimed; real protocol profiles still require sourced timing diagrams or capture fixtures.

## v0.12-dev — Game Boy packet-to-link integration

Adds `pir_gbc_link_on_packet_bytes()`, which validates a raw packet with the existing Game Boy packet codec and routes the result into the link-session coordinator. Valid packets produce ACK or completion decisions; malformed packets produce NAK decisions and update rejection counters. Host tests cover the integration path. This remains a software seam only; no physical timing or hardware interoperability is claimed.

## v0.11-dev — Game Boy link session coordinator

Adds a portable, allocation-free event-driven state machine for connection echo, receive state, packet ACK/NAK decisions, zero-length completion, counters, and timeout aborts. Host tests cover normal and error transitions. This is not the physical pulse/receiver-status implementation: timing, PIO/DMA, optical hardware, and interoperability remain unvalidated.

## v0.10-dev — Host client hardening and continuous integration

Validates host-client timeout, retry, chunk-size, and receive-size arguments before issuing transfers. Adds a GitHub Actions workflow for the portable C build/tests and Python host tests. CI is not a substitute for RP2040, optical, or end-to-end interoperability testing.

## v0.9-dev — Game Boy IR packet foundation

Introduces a separate byte-level codec for the Game Boy Color packet format described in Nintendo's programming manual. Includes bounds checks, checksum verification, completion-marker handling, and host regression tests. This is not a complete Game Boy IR stack: pulse timing, connection/echo and status state machines, hardware capture/transmit, and game-specific protocols remain future work. See `docs/gameboy-ir-development.md`.

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

The original v0.2 source archive was not recovered; v0.1 captures the initial project specification.

## Validation status

The portable C host test suite contains seven suites covering consumer protocol logic, Game Boy packet logic and link coordination, remote database/signal analysis, console dispatch, timing classification, and sampled-waveform edge conversion. The Python host suite covers the transfer codec and client using a fake serial endpoint. The latest local run passed 7/7 C suites and all 8 Python host tests. No physical RP2040, optical frontend, PIO/DMA timing, dual-CDC USB, or custom MicroPython validation is claimed.

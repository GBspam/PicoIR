# Changelog

## v0.19
- Added explicit leases for continuous RX DMA buffers. Consumers release blocks with their sequence token; release reports if DMA overwrote a block while it was being processed, and overrun accounting includes leased buffers.
- Added an RP2040-only stream stress-test firmware for delayed IRQs, oldest-first ordering of pending blocks, delayed consumers, overrun accounting, stale leases, and stop/restart. The harness uses the SDK's hardware sync header for interrupt masking; Pico SDK CI compiles it but does not run it on physical hardware.
- Updated the Pico backend guide with on-device test instructions and clarified that a successful build does not validate timing or optical interoperability.


## v0.18
- Corrected JVC wire layout to use the full 8-bit address followed by the 8-bit command, LSB-first, and added a spec-derived external reference fixture.
- Corrected Samsung32 repeat encoding to emit a complete 32-bit frame rather than an NEC short-repeat burst; added independent Samsung reference-vector and malformed inverse-byte tests.
- Explicitly limited `PIR_PROTO_PANASONIC` to the legacy 32-bit Panasonic-like subset instead of claiming full Kaseikyo support, and prevented a 48-bit frame prefix from being accepted as that subset.
- Added malformed-frame regressions and a public-library consumer test linked only against `picoir`.
- Renamed the root CMake project to `PicoIR`, added GPLv3-only license text, and reconciled Python package license metadata to `GPL-3.0-only`.


## v0.17
- Corrected RC5 extended-command field encoding, required Samsung command/inverse-byte validation, and forced NEC Extended to retain its 16-bit address representation even when the high byte is zero; added regression tests.
- Hardened host response deadlines for injected serial objects with `timeout=None`, rejected boolean timeout values, and added timeout regressions.
- Included the console implementation in the public `picoir` library target so consumer tests link the library rather than compiling `src/console.c` separately.
- Made Pico backend initialization reject claimed state machines, clean up partial setup, claim its state machines, and added an explicit deinitialization API to release PIO/DMA resources.
- Documented CSV metadata-only versus JSON raw-edge export fidelity and added a Pico SDK build workflow, which passes against Pico SDK 2.1.1. DMA stress and measured waveform validation still require physical hardware; no physical interoperability is claimed.

## v0.16
- Added continuous RP2040 RX capture using two chained DMA channels and caller-owned ping-pong buffers.
- Added block sequence numbers, oldest-pending-block retrieval, and a saturating overrun counter when an unread buffer is reused.
- Added a portable allocation-free frame tracker that reports frame-start marks, inter-frame-gap completion, and frame timeout events from packed sample blocks, with active-high/active-low support and boundary tests.
- Added regression coverage proving frame state and absolute sample indices remain continuous when a frame spans separate DMA-sized block feeds.
- Documented buffer alignment/lifetime constraints and the single active streaming-backend limitation. The Pico SDK backend has not been compiled or hardware-tested here.
- Simplified README project status so versioned history is maintained only in this changelog.

## v0.15
- Added `pir_decode_samples()` to connect packed GPIO samples to the existing mark/space converter and protocol decoder; leading idle-space runs are skipped for decoding.
- Added an end-to-end host regression using a synthetic 1 MHz NEC waveform, plus invalid-input coverage.
- The test covers software integration only; real receiver delay, PIO sampling, and optical interoperability remain unvalidated.

## v0.14
- Added an optional Raspberry Pi Pico SDK backend with PIO digital sampling, DMA-backed finite capture, and PIO carrier-modulated envelope transmission through a transistor-driven IR LED output.
- Added a portable packed-sample-to-mark/space edge converter, including active-low receiver support and run merging across 32-bit DMA words.
- Added host regression tests for sample ordering, boundary merging, polarity, duration scaling, and output-capacity errors.
- The Pico SDK backend is isolated under `firmware/pico/` and was not SDK-compiled or hardware-tested in this environment. Continuous ping-pong capture, physical timing calibration, optical validation, TinyUSB dual-CDC integration, and Game Boy handshake/status signaling remain future work.

## v0.13
- Added a generic allocation-free IR duration classifier driven by explicit caller-supplied active/inactive timing windows.
- Added profile validation for invalid bounds and overlapping windows, plus tests for inclusive boundaries, no-match durations, and invalid arguments.
- Intentionally added no built-in Game Boy timing constants; a protocol profile requires sourced timing evidence or a documented capture fixture.

## v0.12
- Added a byte-oriented Game Boy link adapter that validates raw packet bytes and applies the result to the session state machine.
- Valid packets can be returned to the caller while ACK/completion decisions are made; malformed packets are NAKed and counted.
- Added host regression coverage for packet-to-session integration and invalid call ordering. Optical timing and physical signaling remain unimplemented.

## v0.11
- Added an allocation-free Game Boy link-session state machine for connection-echo progression, receive arming, packet acceptance/rejection decisions, completion markers, counters, and timeout/abort transitions.
- Added host tests for state transitions, invalid call ordering, checksum/framing rejection, recovery after rejected packets, completion, and timeouts.
- The state machine is an event coordinator only; pulse timing, physical ACK/NAK signaling, PIO/DMA, and hardware interoperability remain unimplemented/unverified.

## v0.10
- Hardened the Python host client by validating finite positive timeouts, non-negative integer retry counts, valid packet chunk sizes, and non-negative integer receive limits before starting a transfer.
- Added automated CI to build and test the portable C modules and run Python host tests on pushes and pull requests.
- CI remains host-only; no hardware timing or optical interoperability is implied.

## v0.9
- Started a separate Game Boy Color IR development track instead of treating Game Boy traffic as a consumer-remote protocol.
- Added a byte-level codec for the documented `0x5A` packet format, including length validation, checksum validation, and zero-length completion markers.
- Added a Python host library for byte-stream send/receive, packet validation, chunking, checksum-NAK retries, and a file-transfer CLI.
- Added CDC1 `GB MODE`, `GB SEND`, and `GB RECEIVE` command contracts with injectable hardware callbacks.
- Added C console tests and hardware-independent Python tests for packet and transfer behavior.
- Documented the architecture boundary between optical timing, generic packet framing, and game-specific profiles. No physical IR interoperability is claimed.

## v0.8
- Fixed Sony SIRC12/SIRC15/SIRC20 decoding when the final data space is merged with the long inter-frame gap.
- Added host round-trip regression tests for all three Sony SIRC frame lengths.
- Hardware timing and interoperability remain unverified.

## v0.7
- Added allocation-free RC6 mode-0 decoding for start/mode bits, the double-width toggle bit, 8-bit address, and 8-bit command.
- Added RC6 encode/decode round-trip regression coverage.
- Decoder validation currently covers generated host waveforms; real capture timing and device interoperability remain unverified.

## v0.6
- Added RC5 decoding with Manchester run expansion, toggle extraction, and extended-command field handling.
- Corrected RC6 mode-0 encoding to emit the start bit, mode bits, double-width toggle bit, 8-bit address, and 8-bit command.
- Added host regression tests for RC5 round-trip decoding and RC6 frame structure.
- RC6 decoding, hardware waveform timing, and physical interoperability remain unverified.

## v0.5
- Added hardware-independent CDC1 command dispatch for help, remote listing/learning state, save/delete/send operations, and CSV/JSON export commands.
- Added console host tests.
- Learning commands require external capture integration; the dispatcher is not wired to TinyUSB or receiver hardware.

## v0.4
- Added a fixed-capacity RAM-only remote database.
- Added decoded/raw entry operations and JSON/CSV metadata export.
- Added carrier-period measurement and capture-analysis helpers.
- No flash persistence; carrier analysis requires actual carrier-cycle period samples, not just demodulated receiver edges.

## v0.3
- Added a portable consumer-IR protocol layer with initial encoding/decoding support and host tests.
- Protocol support is partial. RC5/RC6 decode and complete Panasonic/Kaseikyo semantics are not implemented; variants and mappings need verification.

## v0.2 (recovery checkpoint)
- No original v0.2 source archive was found in the recovered project materials. This tag records the point in the reconstructed history; it is not a claim that a complete v0.2 firmware implementation exists.

## v0.1 (design snapshot)
- Captured the initial RP2040 architecture, dual-CDC USB intent, PIO/DMA waveform path, discrete IR front-end goals, protocol roadmap, and proposed GPIO assignments.
- Documentation only; no complete v0.1 firmware source was recovered.

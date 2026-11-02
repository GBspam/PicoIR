# Changelog

## Unreleased
- Added continuous RP2040 RX capture using two chained DMA channels and caller-owned ping-pong buffers.
- Added block sequence numbers, oldest-pending-block retrieval, and a saturating overrun counter when an unread buffer is reused.
- Documented buffer alignment/lifetime constraints and the single active streaming-backend limitation. The Pico SDK backend has not been compiled or hardware-tested here.
- Simplified README project status so versioned history is maintained only in this changelog.

## v0.15 (in development)
- Added `pir_decode_samples()` to connect packed GPIO samples to the existing mark/space converter and protocol decoder; leading idle-space runs are skipped for decoding.
- Added an end-to-end host regression using a synthetic 1 MHz NEC waveform, plus invalid-input coverage.
- The test covers software integration only; real receiver delay, PIO sampling, and optical interoperability remain unvalidated.

## v0.14 (in development)
- Added an optional Raspberry Pi Pico SDK backend with PIO digital sampling, DMA-backed finite capture, and PIO carrier-modulated envelope transmission through a transistor-driven IR LED output.
- Added a portable packed-sample-to-mark/space edge converter, including active-low receiver support and run merging across 32-bit DMA words.
- Added host regression tests for sample ordering, boundary merging, polarity, duration scaling, and output-capacity errors.
- The Pico SDK backend is isolated under `firmware/pico/` and was not SDK-compiled or hardware-tested in this environment. Continuous ping-pong capture, physical timing calibration, optical validation, TinyUSB dual-CDC integration, and Game Boy handshake/status signaling remain future work.

## v0.13 (in development)
- Added a generic allocation-free IR duration classifier driven by explicit caller-supplied active/inactive timing windows.
- Added profile validation for invalid bounds and overlapping windows, plus tests for inclusive boundaries, no-match durations, and invalid arguments.
- Intentionally added no built-in Game Boy timing constants; a protocol profile requires sourced timing evidence or a documented capture fixture.

## v0.12 (in development)
- Added a byte-oriented Game Boy link adapter that validates raw packet bytes and applies the result to the session state machine.
- Valid packets can be returned to the caller while ACK/completion decisions are made; malformed packets are NAKed and counted.
- Added host regression coverage for packet-to-session integration and invalid call ordering. Optical timing and physical signaling remain unimplemented.

## v0.11 (in development)
- Added an allocation-free Game Boy link-session state machine for connection-echo progression, receive arming, packet acceptance/rejection decisions, completion markers, counters, and timeout/abort transitions.
- Added host tests for state transitions, invalid call ordering, checksum/framing rejection, recovery after rejected packets, completion, and timeouts.
- The state machine is an event coordinator only; pulse timing, physical ACK/NAK signaling, PIO/DMA, and hardware interoperability remain unimplemented/unverified.

## v0.10 (in development)
- Hardened the Python host client by validating finite positive timeouts, non-negative integer retry counts, valid packet chunk sizes, and non-negative integer receive limits before starting a transfer.
- Added automated CI to build and test the portable C modules and run Python host tests on pushes and pull requests.
- CI remains host-only; no hardware timing or optical interoperability is implied.

## v0.9 (in development)
- Started a separate Game Boy Color IR development track instead of treating Game Boy traffic as a consumer-remote protocol.
- Added a byte-level codec for the documented `0x5A` packet format, including length validation, checksum validation, and zero-length completion markers.
- Added a Python host library for byte-stream send/receive, packet validation, chunking, checksum-NAK retries, and a file-transfer CLI.
- Added CDC1 `GB MODE`, `GB SEND`, and `GB RECEIVE` command contracts with injectable hardware callbacks.
- Added C console tests and hardware-independent Python tests for packet and transfer behavior.
- Documented the architecture boundary between optical timing, generic packet framing, and game-specific profiles. No physical IR interoperability is claimed.

## v0.8 (in development)
- Fixed Sony SIRC12/SIRC15/SIRC20 decoding when the final data space is merged with the long inter-frame gap.
- Added host round-trip regression tests for all three Sony SIRC frame lengths.
- Hardware timing and interoperability remain unverified.

## v0.7 (in development)
- Added allocation-free RC6 mode-0 decoding for start/mode bits, the double-width toggle bit, 8-bit address, and 8-bit command.
- Added RC6 encode/decode round-trip regression coverage.
- Decoder validation currently covers generated host waveforms; real capture timing and device interoperability remain unverified.

## v0.6 (in development)
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

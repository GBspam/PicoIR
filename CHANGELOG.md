# Changelog

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

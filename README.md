# PicoIR 0.11-dev — Game Boy Link Session State Machine

PicoIR is an early-stage universal IR communications platform for the original RP2040-based Raspberry Pi Pico. The current repository contains portable C protocol, remote-database, signal-analysis, and CDC1 command-dispatch modules with host-side unit tests. Version 0.8-dev fixed Sony SIRC decoding when the final data space is merged with the long inter-frame gap. Version 0.9-dev added the first byte-level Game Boy Color IR packet codec and host transfer library. The 0.10-dev increment hardened host-side transfer argument validation and added continuous integration for the C and Python host suites. The 0.11-dev increment adds a hardware-independent Game Boy link-session state machine with host tests.

## v0.11-dev development increment

Added an allocation-free C session coordinator for connection-echo progression, receive arming, packet ACK/NAK decisions, completion-marker handling, counters, and timeout/abort transitions. The state machine consumes events from a future timing backend; it does not generate pulses, implement a physical acknowledgement waveform, or establish Game Boy interoperability. Invalid packets are NAKed while the receive session remains active; a zero-length valid packet ends the session.

## v0.10-dev development increment

The Python transfer client now rejects invalid timeout values, negative/non-integer retry counts, invalid chunk sizes, and invalid receive-size limits before starting a transfer. A GitHub Actions workflow builds the portable C code and runs the C and Python host tests on pushes and pull requests. CI verifies host software only; it does not validate IR hardware, RP2040 PIO/DMA timing, or USB integration.

## Game Boy IR development

PicoIR now has a separate, portable byte-level codec for the Game Boy Color packet format documented in Nintendo's programming manual (`0x5A` raw-data command, up to 255 payload bytes, 16-bit checksum, and zero-length completion marker). This is a foundation for development, not a complete Game Boy IR implementation. **There is no single IR protocol shared by every Game Boy Color game**; game-specific pulse timings, handshakes, and payload formats must be implemented as distinct profiles. The host transfer library and CDC1 command contract are now present. The optical timing decoder, physical connection/echo pulse implementation, receiver status waveform backend, RP2040 PIO/DMA integration, and game-specific profiles are not implemented yet. The new link-session state machine coordinates decoded events but does not implement their physical timing.

See [`docs/gameboy-ir-development.md`](docs/gameboy-ir-development.md) for architecture, contribution priorities, capture-fixture guidance, and technical references. The Python host package is in [`host/`](host/); it exposes `GameBoyIRClient.send(bytes)` and `.receive()` and includes a file-transfer CLI. Install with `python -m pip install './host[serial]'`.

## Commands implemented

- `HELP`
- `REMOTE LIST`
- `REMOTE LEARN START` / `REMOTE LEARN STOP` (arms/disarms learning state; receiver integration must feed captures and decoded frames)
- `REMOTE SAVE name` (saves the most recent decoded frame if present; otherwise saves the most recent bounded raw capture)
- `REMOTE DELETE name`
- `REMOTE SEND protocol address command [TOGGLE]`
- `REMOTE SEND SAVED name`
- `RAW EXPORT CSV` / `RAW EXPORT JSON` (exports the remote database)

All I/O is injected through callbacks. The transmit callback must copy or consume the waveform synchronously before returning; the console itself does not own the PIO/DMA transmitter. Raw replay passes the saved edges and carrier metadata to a separate backend callback.

## Boundaries that remain explicit

- This is a command-dispatch module, not the complete TinyUSB CDC1 line editor or USB application loop.
- The receiver task must call `pir_console_set_capture()` and/or `pir_console_set_decoded()` when capture/decode results arrive. The `LEARN START` state is an arm flag, not a replacement for that receiver integration.
- `REMOTE SEND SAVED` re-encodes decoded entries. Exact waveform reproduction is available only for raw entries.
- Database remains RAM-only; safe persistent flash storage and power-loss-safe records are not implemented here.
- Protocol coverage is still incomplete. RC5 and RC6 mode-0 decoding have host regression tests, but timing tolerance and interoperability need physical validation. Panasonic/Kaseikyo encoding and decoding are only partial; the current public API does not expose all extended fields.

## Current validation status

- Host tests exercise consumer-IR protocol encoding/decoding, Game Boy packet byte encoding/decoding, remote database/signal analysis, and console dispatch.
- The Sony SIRC12/SIRC15/SIRC20 encoders now round-trip through the host decoder, including the merged final-space/inter-frame-gap edge.
- Game Boy packet tests cover payload sizes, completion packets, malformed lengths, unsupported commands, and checksum corruption. Link-state tests cover legal/illegal transitions, connection echo, ACK/NAK decisions, rejected-packet recovery, completion, counters, and timeouts. Console tests cover the new `GB MODE`, `GB SEND`, and `GB RECEIVE` command contract; Python tests cover packet codec, chunking, retries, and completion handling. These do not validate optical timing or physical interoperability.
- The Game Boy CDC1 command backend is an interface boundary only: its callbacks must be wired to the timing/handshake implementation. This is not yet a complete RP2040 firmware image. TinyUSB dual-CDC, PIO/DMA capture/transmit, the physical optical frontend, custom MicroPython integration, and persistent storage are not implemented or hardware-validated.

## Build

```sh
cmake -S . -B build
cmake --build build
ctest --test-dir build --output-on-failure
```

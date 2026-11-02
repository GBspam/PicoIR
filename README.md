# PicoIR 0.5 — CDC1 Console Command Layer

This repository now includes the recovered portable C source snapshot for PicoIR 0.5, along with its host tests and build configuration. The implementation is hardware-independent and is not yet a complete RP2040 firmware image.

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
- Protocol encoder/decoder limitations documented in earlier packages remain, especially RC5/RC6 decoding and full Panasonic/Kaseikyo semantics.
- TinyUSB dual-CDC integration, PIO/DMA waveform capture/transmit, custom MicroPython integration, and physical frontend validation are not included in this source snapshot.
- The repository commit created for this source sync adds the current source snapshot after the documentation commits; it does not recreate the original local commit ancestry or the `v0.1`–`v0.5` Git tags. The local history bundle remains the way to preserve that complete local history.

## Build and test on a host

```sh
cmake -S . -B build
cmake --build build -j2
ctest --test-dir build --output-on-failure
```

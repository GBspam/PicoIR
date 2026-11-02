# PicoIR 0.8-dev — Consumer IR Protocol Layer

PicoIR is an early-stage universal IR communications platform for the original RP2040-based Raspberry Pi Pico. The current repository contains portable C protocol, remote-database, signal-analysis, and CDC1 command-dispatch modules with host-side unit tests. Version 0.8-dev fixes Sony SIRC decoding when the final data space is merged with the long inter-frame gap.

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

- Host tests exercise protocol encoding/decoding, remote database/signal analysis, and console dispatch.
- The Sony SIRC12/SIRC15/SIRC20 encoders now round-trip through the host decoder, including the merged final-space/inter-frame-gap edge.
- This is not yet a complete RP2040 firmware image. TinyUSB dual-CDC, PIO/DMA capture/transmit, the physical optical frontend, custom MicroPython integration, and persistent storage are not implemented or hardware-validated.

## Build

```sh
cmake -S . -B build
cmake --build build
ctest --test-dir build --output-on-failure
```

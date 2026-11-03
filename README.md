# PicoIR

PicoIR is a universal IR communications platform for the original RP2040-based Raspberry Pi Pico. The repository contains portable C protocol, remote-database, signal-analysis, and CDC1 command-dispatch modules with host-side unit tests, plus an optional RP2040 PIO/DMA backend under `firmware/pico/`.

## Current capabilities

- Portable consumer-IR encoding/decoding with partial protocol coverage, including NEC-family, Sony SIRC, RC5, RC6, JVC (8-bit address + 8-bit command), and Samsung32. Protocol coverage is not a claim of physical interoperability.
- A separate Game Boy Color byte-level packet codec, host transfer library, and event-driven link-session coordinator. Game-specific pulse timing and physical handshake/status signaling are not implemented; there is no single IR protocol shared by every Game Boy Color game.
- Generic explicit timing-window classification, packed GPIO sample conversion to mark/space edges, and a portable frame tracker. `pir_decode_samples()` skips leading idle-space runs; `pir_frame_tracker_feed()` reports frame starts, inter-frame-gap ends, and frame timeouts.
- A CDC1 command-dispatch contract included in the public `picoir` library target, plus a RAM-only remote database and CSV/JSON export helpers. The Pico firmware wires continuous RX DMA into the frame tracker and decoder and includes a TinyUSB dual-CDC application.
- An optional Pico SDK backend for finite DMA capture, continuous two-buffer ping-pong DMA capture with sequence numbers, leased-buffer overwrite detection, and overrun accounting, and PIO carrier-envelope transmission through an external transistor driver. Raw replay carrier metadata is applied by quantizing the TX PIO divider and tracking the resulting effective carrier rate.

## Hardware backend status

The Pico SDK project now builds a dual-CDC firmware image as well as the reusable backend. It copies leased DMA blocks before processing, tracks frame boundaries, attempts protocol decoding, and updates the CDC1 learning state. ISR-side timing counters estimate DMA completion/IRQ cadence and detect coalesced IRQs; these are not logic-analyzer measurements. The application and backend compile in CI but have not been run on RP2040 hardware. See [`firmware/pico/README.md`](firmware/pico/README.md) for pins, stream behavior, and validation limits.

The CDC0 binary channel currently emits unframed little-endian RX sample blocks and has no binary TX command format. The Pico firmware applies a waveform's requested carrier frequency when representable; invalid or unrepresentable carrier requests fail instead of silently reusing the prior frequency. Custom MicroPython integration, persistent flash storage, physical Game Boy connection/echo detection, physical ACK/NAK/status signaling, and game-specific timing profiles remain unimplemented or unvalidated. Do not infer Game Boy timings: use cited timing diagrams or provenance-backed capture fixtures.

## Commands implemented

- `HELP`
- `REMOTE LIST`
- `REMOTE LEARN START` / `REMOTE LEARN STOP` (arms/disarms learning state; receiver integration must feed captures and decoded frames)
- `REMOTE SAVE name` (saves the most recent decoded frame if present; otherwise saves the most recent bounded raw capture)
- `REMOTE DELETE name`
- `REMOTE SEND protocol address command [TOGGLE]`
- `REMOTE SEND SAVED name`
- `RAW EXPORT CSV` / `RAW EXPORT JSON` (exports the remote database)

All I/O is injected through callbacks. The transmit callback must copy or consume the waveform synchronously before returning; the console itself does not own the PIO/DMA transmitter. Raw replay passes saved edges and carrier metadata to a separate backend callback. The CDC1 command parser rejects address or command values that exceed the selected protocol's field width rather than silently truncating them.

## Export fidelity

`RAW EXPORT CSV` is metadata-only and omits raw edge durations and toggle state; it is not suitable for waveform backup or restore. `RAW EXPORT JSON` includes mark/duration edge pairs and carrier frequency for raw entries, and protocol/address/command/toggle metadata for decoded entries. Decoded entries have no captured edges and must be re-encoded to transmit, so JSON is not a bit-exact waveform archive for them. Neither format is a versioned persistence format.

## License

PicoIR is licensed under **GNU GPL version 3 only**. See [`LICENSE`](LICENSE). The Python package metadata uses the same `GPL-3.0-only` identifier.

## Known boundaries

- The portable console library remains a command-dispatch module; the Pico firmware supplies the CDC1 line reader and USB application loop.
- The Pico firmware now calls `pir_console_set_capture()` and `pir_console_set_decoded()` when learning is armed and completed frames arrive; other applications using the portable console must provide their own receiver integration.
- `REMOTE SEND SAVED` re-encodes decoded entries. Exact waveform reproduction is available only for raw entries.
- The database remains RAM-only; safe persistent flash storage and power-loss-safe records are not implemented.
- `PIR_PROTO_PANASONIC` currently means a legacy 32-bit Panasonic-like subset only. Standard 48-bit Kaseikyo/Panasonic is explicitly unsupported because the public API lacks vendor, genre, and parity fields; 48-bit frames are guarded against being misclassified as the 32-bit subset.

## Validation

Run the portable host tests with:

```sh
cmake -S . -B build
cmake --build build
ctest --test-dir build --output-on-failure
PYTHONPATH=host python3 -m unittest discover -s tests/host -v
```

Host tests do not validate PIO/DMA timing, optical hardware, USB integration, or physical interoperability. The Pico SDK backend requires a separate SDK build and hardware validation.

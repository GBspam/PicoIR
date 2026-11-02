# PicoIR

PicoIR is an open-source infrared communications platform targeting the original Raspberry Pi Pico (RP2040).

## Project goals

- Expose two independent USB CDC interfaces: a transparent binary pipe and a separate configuration/diagnostics console.
- Use RP2040 PIO and DMA for timing-sensitive infrared capture and transmission.
- Support consumer IR protocol encoding/decoding, raw waveform learning and replay, and exportable remote-control records.
- Provide a discrete 940 nm IR optical front end rather than a dedicated IR encoder/decoder IC.
- Develop IrDA and selected game-console accessory profiles as later protocol families.

## Current source snapshot

The current development snapshot contains a portable C consumer-IR protocol layer, a RAM-backed remote database, signal-analysis helpers, and a hardware-independent CDC1 command dispatcher. The protocol library currently covers initial transmit encoders for NEC/NEC Extended, Sony SIRC, RC5, RC6, Samsung, Panasonic, and JVC; receive decoding and protocol edge cases remain incomplete.

The command dispatcher is a backend interface, not yet a complete TinyUSB CDC application. The remote database is RAM-only. RP2040 PIO/DMA, the dual-CDC USB integration, MicroPython integration, and physical hardware validation are not yet complete.

## Build and host tests

Requires CMake and a C11 compiler:

```sh
cmake -S . -B build
cmake --build build
ctest --test-dir build --output-on-failure
```

The host test suite covers the portable protocol layer, remote database/export helpers, and console dispatcher. Passing host tests do not imply hardware validation.

## Version history

- **v0.1** — archived project specification.
- **v0.2** — source-recovery checkpoint; original v0.2 source archive was not recovered.
- **v0.3** — portable consumer IR protocol layer.
- **v0.4** — remote database and signal-analysis helpers.
- **v0.5** — CDC1 command dispatcher.

The v0.1 and v0.2 tags are documentation/recovery checkpoints, not claims of recovered firmware source. See `CHANGELOG.md` and `RELEASE_NOTES.md` for details.

## Hardware and safety

The IR LED driver must use appropriate current limiting and transistor drive. Verify electrical limits and optical performance on the target hardware before use. This repository does not claim physical validation of the optical front end or PIO timing.

## License

No license has been selected yet. All rights are reserved by default until a license is added.

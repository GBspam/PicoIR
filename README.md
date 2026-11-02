# PicoIR

PicoIR is an infrared communications platform targeting the original Raspberry Pi Pico (RP2040).

## Project goals

- Expose two independent USB CDC interfaces: a transparent binary pipe and a separate configuration/diagnostics console.
- Use RP2040 PIO and DMA for timing-sensitive infrared capture and transmission.
- Support consumer IR encoding/decoding, raw waveform learning and replay, and exportable remote-control records.
- Use a discrete 940 nm IR optical front end rather than a dedicated IR encoder/decoder IC.
- Develop IrDA and selected game-console accessory profiles as later protocol families.

## Development status

The local development snapshot has a portable C consumer-IR protocol layer, a RAM-backed remote database, signal-analysis helpers, and a hardware-independent CDC1 command dispatcher. The protocol library includes initial transmit encoders for NEC/NEC Extended, Sony SIRC, RC5, RC6, Samsung, Panasonic, and JVC; receive decoding and protocol edge cases remain incomplete.

**Repository sync status:** this GitHub branch currently contains project documentation only. The source files, complete local commit ancestry, and version tags have not yet been mirrored here. Therefore, the build commands below apply to the local source snapshot, not this documentation-only branch.

The command dispatcher is not yet a complete TinyUSB CDC application. The remote database is RAM-only. RP2040 PIO/DMA, dual-CDC USB integration, MicroPython integration, and physical hardware validation are not complete.

## Build and host tests

For the local source snapshot, use CMake and a C11 compiler:

```sh
cmake -S . -B build
cmake --build build
ctest --test-dir build --output-on-failure
```

The local host test suite passed 3/3 tests. Host tests do not imply hardware validation.

## Version history

- **v0.1** — archived project specification.
- **v0.2** — source-recovery checkpoint; original v0.2 source archive was not recovered.
- **v0.3** — portable consumer IR protocol layer.
- **v0.4** — remote database and signal-analysis helpers.
- **v0.5** — CDC1 command dispatcher.

The v0.1 and v0.2 tags are documentation/recovery checkpoints, not claims of recovered firmware source. See `CHANGELOG.md` and `RELEASE_NOTES.md` for details.

## Hardware and safety

The IR LED driver must use appropriate current limiting and transistor drive. Verify electrical limits and optical performance on the target hardware before use. No physical validation of the optical front end or PIO timing is claimed.

## License

No license has been selected yet. All rights are reserved by default until a license is added.

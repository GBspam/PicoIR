# Changelog

## v0.6 (in development)
- Added RC5 decoding with Manchester run expansion, toggle extraction, and extended-command field handling.
- Corrected RC6 mode-0 encoding to emit the start bit, mode bits, double-width toggle bit, 8-bit address, and 8-bit command.
- Added host regression tests for RC5 round-trip decoding and RC6 frame structure.
- RC6 decoding, hardware waveform timing, and physical interoperability remain unverified.

## v0.5 — CDC1 command dispatcher

- Added a hardware-independent CDC1 command dispatcher.
- Added remote listing, learn arming/stopping, decoded/raw save/delete/send commands, and CSV/JSON export dispatch.
- Added console dispatcher host tests.
- The dispatcher still requires integration with TinyUSB, a line editor, and live capture/TX backends.

## v0.4 — Remote database and signal analysis

- Added a fixed-capacity RAM-only remote database.
- Added decoded and raw record storage, lookup/deletion, and CSV/JSON streaming export helpers.
- Added carrier-period measurement helpers and capture analysis entry points.
- Persistent storage and complete raw-edge CSV serialization are not implemented.

## v0.3 — Consumer IR protocol layer

- Added a portable C protocol layer.
- Added initial transmit encoders for NEC, NEC Extended, Sony SIRC, RC5, RC6, Samsung, Panasonic, and JVC.
- Added initial receive decoding for selected protocols.
- Protocol edge cases and full RC5/RC6 decode support remain incomplete.

## v0.2 — Source-recovery checkpoint

- Recorded the recovery status of the earlier project source.
- The original v0.2 source archive was not recovered; this tag is a documentation checkpoint.

## v0.1 — Project specification

- Archived the initial project requirements and architecture notes.
- This tag is a specification snapshot, not a complete firmware release.

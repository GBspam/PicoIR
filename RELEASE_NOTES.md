# Release notes

## v0.5

Adds a hardware-independent CDC1 command dispatcher for remote listing, learn arming, record management, decoded/raw transmission dispatch, and CSV/JSON export. Host tests cover the dispatcher. Live USB, capture, and transmit backends still need integration.

## v0.4

Adds a fixed-capacity RAM-backed remote database and signal-analysis helpers. Data is not persistent across reboot, and carrier measurement expects carrier-cycle timing samples rather than pulses from a demodulating IR receiver.

## v0.3

Adds a portable consumer IR protocol layer with initial transmit encoders and selected receive decoders. It is not yet a complete or hardware-validated protocol suite.

## v0.2 and v0.1

These are historical documentation checkpoints only. The original v0.2 source archive was not recovered, and v0.1 captures the project specification rather than a complete firmware image.

## Validation status

The portable host test suite passed 3/3 tests in the local development environment. No physical RP2040, optical frontend, PIO/DMA timing, dual-CDC USB, or custom MicroPython validation is claimed.

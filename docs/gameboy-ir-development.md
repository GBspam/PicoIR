# Game Boy IR development track

PicoIR's Game Boy work is a separate protocol family. Do not assume that a signal observed from one Game Boy Color game is a universal Game Boy protocol.

## Protocol layers

1. **Optical/raw capture** — preserve edge durations and polarity without lossy normalization. The planned RP2040 PIO/DMA capture path is not implemented yet.
2. **Pulse/event decoding** — classify short IR pulses, intervals, synchronization pulses, and connection/echo handshakes. Timing profiles must be explicit and testable against captures; do not silently classify uncertain timings as valid bits.
3. **Link packet format** — the Nintendo Game Boy Programming Manual describes a packet exchange with a connection pulse, synchronous pulses around sections, a two-byte header, up to 255 data bytes, a two-byte checksum, and receiver status. The command documented for raw data is `0x5A`; a zero length marks completion. Data bits are sent MSB-first.
4. **Game profiles** — game-specific implementations belong in independent profiles with their own handshake, timing, payload rules, and evidence. Examples include Pokémon Gold/Silver/Crystal Mystery Gift and Pokémon TCG Card Pop. These must not be presented as interchangeable with the manual's generic packet format.

## Current code

`include/picoir/gameboy_ir.h` and `src/gameboy_ir.c` implement a portable byte-level codec for the documented `0x5A` packet shape. It checks packet size, command, payload length, and the 16-bit sum of the command, length, and payload. The checksum is serialized high byte first. A zero-length packet is reported as a completion marker.

The packet codec is paired with `include/picoir/gameboy_link.h` / `src/gameboy_link.c`, an allocation-free event-driven session coordinator. It models IDLE → WAIT_ECHO → READY → RECEIVING, valid-packet ACK decisions, malformed/checksum-failed packet NAK decisions, zero-length completion, and timeout aborts. Rejected packets leave the receive session active so a backend can continue or apply its own retry policy. Counters record accepted packets, accepted payload bytes, and rejected packets.

The coordinator does not emit or time optical pulses, decide whether a captured waveform is a valid echo, generate a physical ACK/NAK/status waveform, or schedule retries. Those are responsibilities of a future timing/receiver backend. Host tests prove only deterministic state transitions and byte-level behavior, not interoperability with a physical Game Boy Color.

## Contribution priorities

1. Add a capture corpus with provenance: console/model, game and region, operation performed, hardware/receiver, raw edge timings, and whether the capture is known-good. Preserve original captures alongside normalized fixtures.
2. Implement pulse/event classification as a separate allocation-free layer, using timing profiles and boundary tests. Include normal-speed/double-speed variation where relevant; the manual warns that the two ends must use matching speed for its packet protocol.
3. Extend the event coordinator only alongside a cited timing/status specification or capture corpus; implement connection echo detection, section synchronization, physical status signaling, and bounded retry policy in the timing/backend layer.
4. Add named game profiles only when backed by a capture, disassembly, or a cited technical reference. Keep protocol-specific assumptions in each profile.
5. Test malformed, truncated, noisy, overlong, and timing-boundary cases before connecting the code to hardware.

## Suggested capture fixture format

For each capture, store a small metadata file plus a CSV with `duration_us,level` rows. `level=1` means IR detected / active at the capture input and `level=0` means inactive; record any inversion performed by the receiver. Include a note on whether the capture comes from a demodulating module or a discrete photodiode/TIA/comparator, since their edge timing and polarity differ.

Avoid checking in copyrighted game payloads or personal data unless their redistribution status is clear. Small synthetic fixtures are preferred for unit tests.

## References

- Nintendo, *Game Boy Programming Manual*, section 2.6.3 “Infrared Communication” (packet format, timing diagrams, status flow, and operating notes): https://files.nekoblog.org/uploads/pdf/39999184-GameBoy-Programming-Manual.pdf
- Pan Docs, “GBC Infrared Communication” (RP register behavior and the fact that game protocols vary): https://eldred.fr/pandocs/IR.html
- Shonumi, “GBC Infrared Port Technical Documentation” (reverse-engineering notes and examples): https://github.com/shonumi/gbe-plus/blob/master/src/docs/technical/GBC_IR.txt

When a source documents behavior for a particular game, do not generalize it to every GBC title.

## Host API and CDC1 command contract

The current CDC1 command surface is:

- `GB MODE ON` / `GB MODE OFF` — arm/disarm Game Boy transport mode and reset the connector state.
- `GB SEND <hex-packet>` — send one complete, checksum-valid byte packet; response is `GB STATUS OK`, `GB STATUS CHECKSUM_ERROR`, or `GB ERROR ...`.
- `GB RECEIVE <timeout_ms>` — receive one complete packet; response is `GB PACKET <hex-packet>` or `GB ERROR TIMEOUT`.

The command line is limited to 160 characters, so this first host API limits packet payloads sent through CDC1 to 68 bytes even though the on-wire packet format permits 255. This is a transport limitation, not a protocol limitation. The command dispatcher validates packet framing/checksums before invoking injected callbacks. The callback contract leaves pulse timing, connector echo, packet transmission, status response, and receiver timing to the hardware backend. The new `pir_gbc_link_t` state machine is a separate hardware-independent coordinator: callers feed it validated events and execute its returned actions; it does not itself drive the callbacks or hardware.

The Python package lives in `host/picoir_gameboy`:

- `encode_packet(data)` / `decode_packet(packet)` implement the byte-level packet format.
- `GameBoyIRClient.send(data)` splits arbitrary bytes into packets and sends the completion marker.
- `GameBoyIRClient.receive()` accumulates packets until the completion marker.
- Checksum NAKs are retried; transport timeouts and malformed packets raise explicit exceptions.
- `python -m picoir_gameboy --port PORT send FILE` and `... receive FILE` provide a basic file-transfer CLI.

The sender assumes the remote is already listening. A generic byte transfer does not implement a particular game's handshake or application payload, and host-only tests do not prove interoperability with a Game Boy. A future backend should be validated against captured hardware waveforms and a real Game Boy Color before these commands are described as hardware-supported.

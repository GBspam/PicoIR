# RP2040 firmware, TinyUSB, PIO and DMA

This Raspberry Pi Pico SDK project builds a reusable PIO/DMA backend and a dual-CDC firmware application. CDC0 carries unframed binary RX sample words only; CDC1 is the human-readable command console. The binary channel has no text diagnostics or binary command framing.

## Build

    cmake -S firmware/pico -B build-pico -DPICO_SDK_PATH=/path/to/pico-sdk
    cmake --build build-pico

The Pico SDK CI workflow builds the backend, dual-CDC firmware, and optional hardware stress-test executable against Raspberry Pi Pico SDK 2.1.1. CI verifies compilation/linking only; no RP2040 hardware stress test or waveform measurement has been performed.

## Backend behavior

- Finite RX capture samples one digital GPIO input at `sample_rate_hz`; each sample costs two state-machine cycles. DMA fills a caller-owned array of 32-bit words. The oldest sample is bit 31. Sample rate is limited to 1 MHz.
- Continuous RX capture alternates two caller-owned buffers using chained DMA channels. Each completed block receives a monotonically increasing sequence number. `pir_pico_rx_stream_take()` returns the oldest available block and leases its buffer. Call `pir_pico_rx_stream_release()` after processing; it returns false if DMA overwrote the leased block. `pir_pico_rx_stream_overruns()` counts completed blocks that replace unread or leased buffers. Buffers must be 32-bit aligned, non-overlapping, at least eight words each, and remain allocated until stop.
- Continuous mode uses a shared DMA IRQ handler and supports one active streaming backend instance. Keep block processing short and on the same core. The firmware copies each leased block before processing and discards the copy if release reports that DMA overwrote the source. Overrun counts saturate at `UINT32_MAX`.
- The ISR records completed blocks, IRQ intervals, maximum estimated IRQ lateness against the expected block schedule, and coalesced-interrupt timing faults. If both completion flags are pending together, it aborts/re-arms both channels and explicitly restarts channel 0 to avoid a stalled ping-pong chain. These software estimates are not measurements of the GPIO sample clock; validate with hardware and a logic analyzer.
- RX expects a digital demodulating receiver or comparator output. It is not an ADC path and cannot measure 38 kHz carrier cycles from a typical demodulating receiver.
- `pir_samples_to_edges()` converts packed samples into mark/space runs and merges equal levels across word boundaries. Set `mark_level=false` for active-low receiver output.
- TX consumes up to 256 mark/space runs. PIO generates a nominal carrier and gates it on for mark runs. `pir_pico_tx_set_carrier_hz()` quantizes the requested carrier against the current system clock and records the effective rate; requests outside the representable divider range are rejected. The firmware applies each waveform's carrier metadata before transmission instead of silently ignoring it. Durations are rounded to carrier periods. Per-run overhead means protocol-grade timing tolerance is not claimed; measure with a logic analyzer.
- TX is intended to drive a transistor stage, not an IR LED directly from a GPIO. Design current limiting and transistor/base drive for the selected LED and supply.
- RX and TX use two state machines in the same PIO block. Choose distinct SM indices and pins. The backend does not configure or validate the optical analog frontend.

## API outline

1. Zero-initialize an unused `pir_pico_backend_t`, choose two distinct unclaimed state machines in the same PIO block, and call `pir_pico_backend_init()`. The backend claims both SMs and owns its two PIO programs until `pir_pico_backend_deinit()`.
2. Use either `pir_pico_rx_start()` for a finite capture or `pir_pico_rx_stream_start()` for continuous two-buffer capture. Poll finite capture with `pir_pico_rx_done()`; drain stream blocks with `pir_pico_rx_stream_take()` and inspect `pir_pico_rx_stream_overruns()`.
3. Call `pir_pico_rx_stream_stop()` to stop streaming and release both DMA channels. `pir_pico_rx_stream_get_timing_stats()` snapshots timing and overrun statistics while streaming.
4. While TX is idle, optionally call `pir_pico_tx_set_carrier_hz()` to select a representable carrier. `pir_pico_tx_start()` copies a run array to backend-owned storage and starts DMA/PIO transmission; poll `pir_pico_tx_done()`. Abort calls leave TX inactive.
5. Call `pir_pico_backend_deinit()` before reusing the state machines or PIO instruction memory. It stops active capture/transmission, releases DMA channels, removes owned PIO programs, and unclaims both state machines.

Do not start RX and TX simultaneously on one backend instance. Program/SM ownership is dedicated to this backend instance. Initialization refuses already-claimed state machines or insufficient PIO instruction memory. Do not independently remove its PIO programs or use its claimed state machines. The streaming DMA interrupt handler is shared with other DMA IRQ0 users, but this backend supports only one streaming instance at a time.

## Export fidelity

`RAW EXPORT CSV` is a metadata summary only: it includes name, entry kind, protocol, address, command, carrier frequency, and edge count, but omits edge durations and toggle state. It is not a backup/restore format. `RAW EXPORT JSON` includes raw edge mark/duration pairs and carrier frequency for raw entries; decoded entries export protocol/address/command/toggle metadata and an empty edge list. JSON is higher fidelity, but decoded records must be re-encoded rather than replaying a captured waveform. Neither export is a versioned persistence format.

## Dual-CDC firmware

The default `PICOIR_BUILD_FIRMWARE=ON` target builds `picoir_firmware.uf2`. Connect to the two USB CDC interfaces:

- **CDC0 — Binary Capture:** while the port is open, each accepted RX block is emitted as 256 little-endian 32-bit sample words (1024 bytes by default), oldest sample in bit 31. There is no header or text. The stream is best-effort: if the CDC TX buffer cannot accept a whole block, that block is dropped and the count appears in `STATUS` on CDC1.
- **CDC1 — Command Console:** newline-terminated commands from the portable console (`HELP`, `REMOTE ...`, `RAW EXPORT ...`, and `GB ...`) plus `STATUS`. Diagnostics and decoded-learning reports appear only here. `REMOTE LEARN START` arms capture/decode updates; `REMOTE SAVE name` stores the most recent decoded frame when available, otherwise the bounded raw edge capture.

The firmware uses GPIO2 as active-low receiver input and GPIO3 as active-high TX envelope output, 100 kHz effective RX sampling, 38 kHz nominal TX carrier, a 12 ms frame gap and 180 ms frame timeout. These are compile-time defaults. `STATUS` reports effective sample rate, expected block period, completed blocks, DMA overruns, coalesced-IRQ timing faults, observed IRQ intervals/lateness, and CDC0 dropped blocks. The TX backend pauses RX capture during transmission and restarts the stream afterward. No Game Boy-specific optical timing/handshake profile or binary TX command format is implemented.

## On-device stream stress test

The `picoir_rx_stream_stress` executable is built when configuring with `-DPICOIR_BUILD_HARDWARE_TESTS=ON` (the Pico SDK CI workflow compiles it but does not run it). Flash the generated UF2 and open its USB serial console. It checks a bounded delayed-IRQ window, monotonic sequence ordering, lease invalidation and overrun accounting under a delayed consumer, plus stop/restart behavior. The RX input is held low with a pull-down; no optical front-end is needed for DMA lifecycle tests. A successful run is hardware evidence for these lifecycle checks only, not protocol timing or optical interoperability.

## Validation still required

- Build with the supported Pico SDK/toolchain and resolve any SDK-version API differences.
- Run `picoir_rx_stream_stress` on an RP2040 and retain its console output with board/SDK details. This harness is not run by CI.
- Check PIO timing and DMA behavior on RP2040 hardware; verify interrupt rearming at the selected sample rate and buffer size. This has not been performed; no measured waveform timing or physical interoperability is claimed.
- Verify dual-CDC enumeration on the target host, confirm CDC0 byte order/block cadence and CDC1 command behavior, and run `STATUS` while capturing. The application feeds copied, lease-validated RX blocks into `pir_frame_tracker_feed()` and decodes completed frames; this path has compile-time coverage only until run on hardware.
- Measure mark/space timing and carrier frequency with a logic analyzer or photodiode.
- Verify receiver polarity and propagation delay.
- Add a documented binary transmit protocol if CDC0 must accept TX payloads, and implement Game Boy-specific optical timing, connection echo, and physical ACK/NAK/status waveforms.
- Validate transistor drive, current limit, thermal behavior, optical range, and ambient-light performance before prolonged transmission.

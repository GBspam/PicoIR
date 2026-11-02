# RP2040 PIO/DMA backend

This separate Raspberry Pi Pico SDK project builds a reusable backend library, not a complete firmware image. TinyUSB dual-CDC is not integrated.

## Build

    cmake -S firmware/pico -B build-pico -DPICO_SDK_PATH=/path/to/pico-sdk
    cmake --build build-pico

The repository's Pico SDK CI workflow successfully configured and built this backend against Raspberry Pi Pico SDK 2.1.1. This verifies compilation/linking only; no RP2040 hardware stress test or waveform measurement has been performed.

## Backend behavior

- Finite RX capture samples one digital GPIO input at `sample_rate_hz`; each sample costs two state-machine cycles. DMA fills a caller-owned array of 32-bit words. The oldest sample is bit 31. Sample rate is limited to 1 MHz.
- Continuous RX capture alternates two caller-owned buffers using chained DMA channels. Each completed block receives a monotonically increasing sequence number. `pir_pico_rx_stream_take()` returns the oldest pending block, and `pir_pico_rx_stream_overruns()` reports blocks overwritten before consumption. Buffers must be 32-bit aligned, non-overlapping, at least eight words each, and remain allocated until stop.
- Continuous mode uses a shared DMA IRQ handler and supports one active streaming backend instance. Keep block processing short and on the same core; a returned buffer is only stable until DMA cycles back to it. Overrun counts saturate at `UINT32_MAX`.
- RX expects a digital demodulating receiver or comparator output. It is not an ADC path and cannot measure 38 kHz carrier cycles from a typical demodulating receiver.
- `pir_samples_to_edges()` converts packed samples into mark/space runs and merges equal levels across word boundaries. Set `mark_level=false` for active-low receiver output.
- TX consumes up to 256 mark/space runs. PIO generates a nominal carrier and gates it on for mark runs. Durations are rounded to carrier periods. Per-run overhead means protocol-grade timing tolerance is not claimed; measure with a logic analyzer.
- TX is intended to drive a transistor stage, not an IR LED directly from a GPIO. Design current limiting and transistor/base drive for the selected LED and supply.
- RX and TX use two state machines in the same PIO block. Choose distinct SM indices and pins. The backend does not configure or validate the optical analog frontend.

## API outline

1. Zero-initialize an unused `pir_pico_backend_t`, choose two distinct unclaimed state machines in the same PIO block, and call `pir_pico_backend_init()`. The backend claims both SMs and owns its two PIO programs until `pir_pico_backend_deinit()`.
2. Use either `pir_pico_rx_start()` for a finite capture or `pir_pico_rx_stream_start()` for continuous two-buffer capture. Poll finite capture with `pir_pico_rx_done()`; drain stream blocks with `pir_pico_rx_stream_take()` and inspect `pir_pico_rx_stream_overruns()`.
3. Call `pir_pico_rx_stream_stop()` to stop streaming and release both DMA channels.
4. `pir_pico_tx_start()` copies a run array to backend-owned storage and starts DMA/PIO transmission; poll `pir_pico_tx_done()`. Abort calls leave TX inactive.
5. Call `pir_pico_backend_deinit()` before reusing the state machines or PIO instruction memory. It stops active capture/transmission, releases DMA channels, removes owned PIO programs, and unclaims both state machines.

Do not start RX and TX simultaneously on one backend instance. Program/SM ownership is dedicated to this backend instance. Initialization refuses already-claimed state machines or insufficient PIO instruction memory. Do not independently remove its PIO programs or use its claimed state machines. The streaming DMA interrupt handler is shared with other DMA IRQ0 users, but this backend supports only one streaming instance at a time.

## Export fidelity

`RAW EXPORT CSV` is a metadata summary only: it includes name, entry kind, protocol, address, command, carrier frequency, and edge count, but omits edge durations and toggle state. It is not a backup/restore format. `RAW EXPORT JSON` includes raw edge mark/duration pairs and carrier frequency for raw entries; decoded entries export protocol/address/command/toggle metadata and an empty edge list. JSON is higher fidelity, but decoded records must be re-encoded rather than replaying a captured waveform. Neither export is a versioned persistence format.

## Validation still required

- Build with the supported Pico SDK/toolchain and resolve any SDK-version API differences.
- Add a hardware stress test that deliberately delays block consumption and verifies overrun counts and sequence ordering.
- Check PIO timing and DMA behavior on RP2040 hardware; verify interrupt rearming at the selected sample rate and buffer size. This has not been performed; no measured waveform timing or physical interoperability is claimed.
- Feed completed blocks to `pir_frame_tracker_feed()` to detect frame start, inter-frame gap, and timeout events; automatic wiring into the backend/console remains future work.
- Measure mark/space timing and carrier frequency with a logic analyzer or photodiode.
- Verify receiver polarity and propagation delay.
- Integrate with `pir_decode()`, Game Boy session coordination, physical ACK/NAK/status waveforms, and TinyUSB dual-CDC.
- Validate transistor drive, current limit, thermal behavior, optical range, and ambient-light performance before prolonged transmission.

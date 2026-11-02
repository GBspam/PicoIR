# RP2040 PIO/DMA backend (development)

This separate Raspberry Pi Pico SDK project builds a reusable backend library, not a complete firmware image. TinyUSB dual-CDC is not integrated.

## Build

    cmake -S firmware/pico -B build-pico -DPICO_SDK_PATH=/path/to/pico-sdk
    cmake --build build-pico

The Pico SDK project has not been compiled in this environment because the SDK is unavailable. Treat this code as an integration starting point until a real SDK build succeeds.

## Backend behavior

- RX samples one digital GPIO input at sample_rate_hz; each sample costs two state-machine cycles. DMA fills a caller-owned finite array of 32-bit words. The oldest sample is bit 31. Sample rate is limited to 1 MHz.
- RX expects a digital demodulating receiver or comparator output. It is not an ADC path and cannot measure 38 kHz carrier cycles from a typical demodulating receiver.
- pir_samples_to_edges() converts packed samples to mark/space runs and merges equal levels across word boundaries. Set mark_level=false for active-low receiver output.
- TX consumes up to 256 mark/space runs. PIO generates a nominal carrier and gates it for mark runs. Durations are rounded to carrier periods. Per-run overhead means protocol-grade timing tolerance is not claimed; measure with a logic analyzer.
- TX is intended to drive a transistor stage, not an IR LED directly from a GPIO. Design current limiting and transistor/base drive for the selected LED and supply.
- RX and TX use two state machines in the same PIO block. Choose distinct SM indices and pins. The backend does not configure or validate the optical analog frontend.
- Capture is finite, not continuous ping-pong/ring DMA. There is no overflow accounting or automatic frame trigger yet.

## API outline

1. pir_pico_backend_init() installs both PIO programs and configures sample/carrier clocks.
2. pir_pico_rx_start() starts finite capture; poll pir_pico_rx_done(), then convert with pir_samples_to_edges().
3. pir_pico_tx_start() copies a run array to backend-owned storage and starts DMA/PIO transmission; poll pir_pico_tx_done(). Abort calls leave TX inactive.

Do not start RX and TX simultaneously on one backend instance. Program/SM ownership is dedicated to this backend; there is no general PIO resource manager or deinit routine yet.

## Validation still required

- Build with the supported Pico SDK/toolchain.
- Check PIO timing and DMA behavior on RP2040 hardware.
- Measure mark/space timing and carrier frequency with a logic analyzer or photodiode.
- Verify receiver polarity and propagation delay.
- Add continuous ping-pong capture, overrun reporting, frame trigger/timeout logic, and bounded queues.
- Integrate with pir_decode(), Game Boy session coordination, physical ACK/NAK/status waveforms, and TinyUSB dual-CDC.
- Validate transistor drive, current limit, thermal behavior, optical range, and ambient-light performance before prolonged transmission.

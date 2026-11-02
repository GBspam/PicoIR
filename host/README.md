# PicoIR host tools

The `picoir_gameboy` package provides a Python byte-stream API for the documented Game Boy Color packet protocol over PicoIR's CDC1 text console. It packetizes a file/byte string, verifies packet checksums, retries receiver checksum NAKs, and ends sends with the zero-length completion packet.

## Install / use

From the repository root, add `host` to `PYTHONPATH` or install the package with its serial dependency. Serial support requires `pyserial`:

```sh
python -m pip install './host[serial]'
PYTHONPATH=host python -m picoir_gameboy --port /dev/ttyACM1 send save-data.bin
PYTHONPATH=host python -m picoir_gameboy --port /dev/ttyACM1 receive received.bin
```

Use the **CDC1 console interface**, not CDC0. For Python applications:

```python
from picoir_gameboy import GameBoyIRClient

with GameBoyIRClient("/dev/ttyACM1", timeout=10) as ir:
    ir.send(b"application-defined bytes")
    # On a separately initiated transfer, when the peer is sending:
    received = ir.receive(timeout=30)
```

This is a generic transport for arbitrary payload bytes; it does not implement game-specific protocols or make a game accept arbitrary data. The receiver must be placed into its receive state by the game/device workflow. Current CDC1 packet commands cap payloads at 68 bytes per packet to fit the line buffer. The C console command layer now defines `GB MODE ON|OFF`, `GB SEND <hex-packet>`, and `GB RECEIVE <timeout_ms>`, but actual transfers require the RP2040 pulse/handshake/status backend callbacks. Those callbacks and physical IR timing are not yet implemented by this repository, so the host API is not yet end-to-end usable on hardware.

The Python tests run without hardware or pyserial:

```sh
PYTHONPATH=host python -m unittest discover -s tests/host -v
```

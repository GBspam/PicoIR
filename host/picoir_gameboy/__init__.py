"""Host-side transfer helpers for PicoIR's Game Boy Color IR mode.

The Python API uses PicoIR's CDC1 text command channel. Firmware must include
an IR timing backend for the `GB MODE`, `GB SEND`, and `GB RECEIVE` commands.
"""
from .client import (
    GameBoyIRClient,
    GameBoyIRException,
    GameBoyIRTimeout,
    GameBoyPacketError,
    GameBoyRemoteStatusError,
    encode_packet,
    decode_packet,
)

__all__ = [
    "GameBoyIRClient", "GameBoyIRException", "GameBoyIRTimeout",
    "GameBoyPacketError", "GameBoyRemoteStatusError", "encode_packet",
    "decode_packet",
]

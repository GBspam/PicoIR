"""Game Boy Color IR packet and transfer API for PicoIR.

This implements the byte-level packet format documented in Nintendo's Game
Boy Programming Manual. It does not implement game-specific protocols (for
example, Pokémon Mystery Gift); those payloads belong above this transport.
"""
from __future__ import annotations

from dataclasses import dataclass
import math
import time
from typing import Optional

RAW_DATA_COMMAND = 0x5A
MAX_PAYLOAD = 255
PACKET_OVERHEAD = 4
# CDC1 commands/responses are bounded by PicoIR's 160-byte console line.
DEFAULT_CHUNK_SIZE = 68


class GameBoyIRException(Exception):
    """Base class for PicoIR Game Boy transfer errors."""


class GameBoyIRTimeout(GameBoyIRException):
    """The device or remote did not respond before the configured timeout."""


class GameBoyPacketError(GameBoyIRException):
    """A packet was malformed or failed checksum validation."""


class GameBoyRemoteStatusError(GameBoyIRException):
    """The remote returned a non-success status."""


@dataclass(frozen=True)
class Packet:
    command: int
    data: bytes
    checksum: int

    @property
    def complete(self) -> bool:
        return len(self.data) == 0


def _checksum(command: int, length: int, data: bytes) -> int:
    return (command + length + sum(data)) & 0xFFFF


def encode_packet(data: bytes = b"") -> bytes:
    """Encode one raw-data packet; an empty payload is the completion marker."""
    data = bytes(data)
    if len(data) > MAX_PAYLOAD:
        raise ValueError("Game Boy IR packets support at most 255 payload bytes")
    length = len(data)
    checksum = _checksum(RAW_DATA_COMMAND, length, data)
    return bytes((RAW_DATA_COMMAND, length)) + data + checksum.to_bytes(2, "big")


def decode_packet(raw: bytes) -> Packet:
    """Decode and checksum-validate one complete raw-data packet."""
    raw = bytes(raw)
    if len(raw) < PACKET_OVERHEAD:
        raise GameBoyPacketError("packet is shorter than its header and checksum")
    if raw[0] != RAW_DATA_COMMAND:
        raise GameBoyPacketError(f"unsupported packet command 0x{raw[0]:02X}")
    expected = raw[1] + PACKET_OVERHEAD
    if len(raw) != expected:
        raise GameBoyPacketError(
            f"packet length mismatch: header says {expected} bytes, got {len(raw)}"
        )
    payload = raw[2:-2]
    received = int.from_bytes(raw[-2:], "big")
    calculated = _checksum(raw[0], raw[1], payload)
    if received != calculated:
        raise GameBoyPacketError(
            f"checksum mismatch: received 0x{received:04X}, calculated 0x{calculated:04X}"
        )
    return Packet(raw[0], payload, received)


class GameBoyIRClient:
    """Transfer byte streams through PicoIR's CDC1 command interface.

    `serial_port` may be an already-open pyserial-compatible object, which is
    useful for applications and tests. If omitted, pyserial is imported and a
    serial port is opened from `port`. Use the CDC1/configuration interface,
    not CDC0's transparent binary pipe.
    """

    def __init__(self, port: Optional[str] = None, *, serial_port=None,
                 baudrate: int = 115200, timeout: float = 2.0):
        if serial_port is None:
            if not port:
                raise ValueError("provide port=... or serial_port=...")
            try:
                import serial  # type: ignore
            except ImportError as exc:
                raise RuntimeError(
                    "pyserial is required to open a port; install with `pip install pyserial`"
                ) from exc
            serial_port = serial.Serial(port, baudrate=baudrate, timeout=timeout,
                                        write_timeout=timeout)
            self._owns_serial = True
        else:
            self._owns_serial = False
        if not isinstance(timeout, (int, float)) or not math.isfinite(timeout) or timeout <= 0:
            if self._owns_serial:
                serial_port.close()
            raise ValueError("timeout must be a finite number greater than zero")
        self.serial = serial_port
        self.timeout = float(timeout)
        self._enabled = False

    def close(self) -> None:
        if self._enabled:
            try:
                self.disable()
            except GameBoyIRException:
                pass
        if self._owns_serial:
            self.serial.close()

    def __enter__(self) -> "GameBoyIRClient":
        self.enable()
        return self

    def __exit__(self, exc_type, exc, tb) -> None:
        self.close()

    def _write_command(self, command: str) -> None:
        wire = (command + "\r\n").encode("ascii")
        written = self.serial.write(wire)
        if written is not None and written != len(wire):
            raise GameBoyIRException("short write to PicoIR CDC1")
        flush = getattr(self.serial, "flush", None)
        if flush:
            flush()

    def _readline(self, timeout: Optional[float] = None) -> str:
        # pyserial's timeout is per read; temporarily tighten it for each reply.
        old_timeout = getattr(self.serial, "timeout", None)
        limit = self.timeout if timeout is None else timeout
        deadline = time.monotonic() + max(0.0, limit)
        try:
            while time.monotonic() < deadline:
                if old_timeout is not None:
                    self.serial.timeout = min(old_timeout, max(0.01, deadline - time.monotonic()))
                raw = self.serial.readline()
                if raw:
                    return raw.decode("ascii", errors="replace").strip()
            raise GameBoyIRTimeout("timed out waiting for PicoIR CDC1 response")
        finally:
            if old_timeout is not None:
                self.serial.timeout = old_timeout

    def _command(self, command: str, *, timeout: Optional[float] = None) -> str:
        self._write_command(command)
        response = self._readline(timeout)
        if response.startswith("GB ERROR ") or response.startswith("ERROR:"):
            if "TIMEOUT" in response:
                raise GameBoyIRTimeout(response)
            raise GameBoyIRException(response)
        return response

    def enable(self) -> None:
        response = self._command("GB MODE ON")
        if response != "GB MODE ON":
            raise GameBoyIRException(f"unexpected response to GB MODE ON: {response!r}")
        self._enabled = True

    def disable(self) -> None:
        response = self._command("GB MODE OFF")
        if response != "GB MODE OFF":
            raise GameBoyIRException(f"unexpected response to GB MODE OFF: {response!r}")
        self._enabled = False

    def _require_enabled(self) -> None:
        if not self._enabled:
            raise GameBoyIRException("Game Boy mode is not enabled; call enable() first")

    def send_packet(self, packet_bytes: bytes, *, retries: int = 3) -> None:
        """Send one encoded packet, retrying checksum NAKs up to `retries` times."""
        self._require_enabled()
        if isinstance(retries, bool) or not isinstance(retries, int) or retries < 0:
            raise ValueError("retries must be a non-negative integer")
        packet = decode_packet(packet_bytes)
        if len(packet_bytes) > DEFAULT_CHUNK_SIZE + PACKET_OVERHEAD:
            raise ValueError("packet is too large for CDC1; use send() to chunk a stream")
        attempts = 0
        while True:
            response = self._command("GB SEND " + bytes(packet_bytes).hex().upper())
            if response == "GB STATUS OK":
                return
            if response == "GB STATUS CHECKSUM_ERROR":
                if attempts >= retries:
                    raise GameBoyRemoteStatusError("remote reported checksum error after retries")
                attempts += 1
                continue
            raise GameBoyRemoteStatusError(f"unexpected receiver status: {response!r}")

    def send(self, data: bytes, *, chunk_size: int = DEFAULT_CHUNK_SIZE,
             retries: int = 3, finish: bool = True) -> int:
        """Send a byte stream as packets and (by default) a completion marker.

        The receiver must already be in the appropriate Game Boy receive state.
        Returns the number of payload bytes sent. The manual's packet protocol
        does not itself define what those payload bytes mean to a particular game.
        """
        self._require_enabled()
        data = bytes(data)
        if isinstance(chunk_size, bool) or not isinstance(chunk_size, int) or not 1 <= chunk_size <= DEFAULT_CHUNK_SIZE:
            raise ValueError(f"chunk_size must be an integer in 1..{DEFAULT_CHUNK_SIZE}")
        if isinstance(retries, bool) or not isinstance(retries, int) or retries < 0:
            raise ValueError("retries must be a non-negative integer")
        for offset in range(0, len(data), chunk_size):
            self.send_packet(encode_packet(data[offset:offset + chunk_size]), retries=retries)
        if finish:
            self.send_packet(encode_packet(), retries=retries)
        return len(data)

    def receive_packet(self, *, timeout: Optional[float] = None) -> Packet:
        """Receive and validate one packet from the remote Game Boy device."""
        self._require_enabled()
        seconds = self.timeout if timeout is None else timeout
        if not isinstance(seconds, (int, float)) or not math.isfinite(seconds) or seconds <= 0:
            raise ValueError("timeout must be a finite number greater than zero")
        timeout_ms = max(1, min(60000, int(seconds * 1000)))
        response = self._command(f"GB RECEIVE {timeout_ms}", timeout=seconds + 0.25)
        prefix = "GB PACKET "
        if not response.startswith(prefix):
            raise GameBoyIRException(f"unexpected response to GB RECEIVE: {response!r}")
        try:
            raw = bytes.fromhex(response[len(prefix):])
        except ValueError as exc:
            raise GameBoyPacketError("device returned non-hex packet data") from exc
        if len(raw) > DEFAULT_CHUNK_SIZE + PACKET_OVERHEAD:
            raise GameBoyPacketError("device returned a packet too large for CDC1")
        return decode_packet(raw)

    def receive(self, *, timeout: Optional[float] = None,
                max_bytes: int = 1_048_576) -> bytes:
        """Receive packets until the zero-length completion marker arrives."""
        self._require_enabled()
        seconds = self.timeout if timeout is None else timeout
        if not isinstance(seconds, (int, float)) or not math.isfinite(seconds) or seconds <= 0:
            raise ValueError("timeout must be a finite number greater than zero")
        if isinstance(max_bytes, bool) or not isinstance(max_bytes, int) or max_bytes < 0:
            raise ValueError("max_bytes must be a non-negative integer")
        deadline = time.monotonic() + seconds
        result = bytearray()
        while True:
            remaining = deadline - time.monotonic()
            if remaining <= 0:
                raise GameBoyIRTimeout("timed out waiting for Game Boy completion packet")
            packet = self.receive_packet(timeout=remaining)
            if packet.complete:
                return bytes(result)
            if len(result) + len(packet.data) > max_bytes:
                raise GameBoyIRException(f"received transfer exceeds max_bytes={max_bytes}")
            result.extend(packet.data)

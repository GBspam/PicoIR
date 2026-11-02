"""Small command-line utility: python -m picoir_gameboy --port PORT send|receive FILE"""
import argparse
from pathlib import Path

from .client import GameBoyIRClient


def main() -> int:
    parser = argparse.ArgumentParser(description="Transfer bytes using PicoIR Game Boy mode")
    parser.add_argument("--port", required=True, help="PicoIR CDC1 serial port")
    parser.add_argument("--timeout", type=float, default=10.0, help="seconds per receive operation")
    sub = parser.add_subparsers(dest="action", required=True)
    send = sub.add_parser("send", help="send a file to an already-listening Game Boy peer")
    send.add_argument("file", type=Path)
    receive = sub.add_parser("receive", help="receive a transfer into a file")
    receive.add_argument("file", type=Path)
    args = parser.parse_args()

    with GameBoyIRClient(args.port, timeout=args.timeout) as client:
        if args.action == "send":
            count = client.send(args.file.read_bytes())
            print(f"Sent {count} bytes")
        else:
            data = client.receive(timeout=args.timeout)
            args.file.write_bytes(data)
            print(f"Received {len(data)} bytes")
    return 0


if __name__ == "__main__":
    raise SystemExit(main())

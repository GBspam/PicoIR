import unittest

from picoir_gameboy import (
    GameBoyIRClient, GameBoyIRException, GameBoyPacketError,
    encode_packet, decode_packet,
)


class FakeSerial:
    def __init__(self, *, checksum_naks=0, incoming=()):
        self.timeout = 0.05
        self.lines = []
        self.writes = []
        self.checksum_naks = checksum_naks
        self.incoming = list(incoming)
        self.sent_packets = []

    def write(self, data):
        self.writes.append(data)
        command = data.decode("ascii").strip()
        if command == "GB MODE ON":
            self.lines.append(b"GB MODE ON\r\n")
        elif command == "GB MODE OFF":
            self.lines.append(b"GB MODE OFF\r\n")
        elif command.startswith("GB SEND "):
            raw = bytes.fromhex(command[8:])
            self.sent_packets.append(raw)
            if self.checksum_naks:
                self.checksum_naks -= 1
                self.lines.append(b"GB STATUS CHECKSUM_ERROR\r\n")
            else:
                self.lines.append(b"GB STATUS OK\r\n")
        elif command.startswith("GB RECEIVE "):
            if not self.incoming:
                self.lines.append(b"GB ERROR TIMEOUT\r\n")
            else:
                self.lines.append(b"GB PACKET " + self.incoming.pop(0).hex().upper().encode() + b"\r\n")
        return len(data)

    def flush(self):
        pass

    def readline(self):
        return self.lines.pop(0) if self.lines else b""


class CodecTests(unittest.TestCase):
    def test_round_trip_and_completion(self):
        for payload in (b"", b"hello", bytes(range(255))):
            self.assertEqual(decode_packet(encode_packet(payload)).data, payload)
        self.assertTrue(decode_packet(encode_packet()).complete)

    def test_bad_checksum_and_lengths(self):
        raw = bytearray(encode_packet(b"abc"))
        raw[2] ^= 1
        with self.assertRaises(GameBoyPacketError):
            decode_packet(raw)
        with self.assertRaises(GameBoyPacketError):
            decode_packet(encode_packet(b"abc")[:-1])
        with self.assertRaises(ValueError):
            encode_packet(bytes(256))


class ClientTests(unittest.TestCase):
    def test_send_chunks_and_completion(self):
        fake = FakeSerial()
        client = GameBoyIRClient(serial_port=fake)
        client.enable()
        data = bytes(range(150))
        self.assertEqual(client.send(data), len(data))
        packets = [decode_packet(p) for p in fake.sent_packets]
        self.assertEqual(b"".join(p.data for p in packets[:-1]), data)
        self.assertTrue(packets[-1].complete)
        self.assertLessEqual(max(len(p.data) for p in packets), 68)

    def test_checksum_retry(self):
        fake = FakeSerial(checksum_naks=1)
        client = GameBoyIRClient(serial_port=fake)
        client.enable()
        client.send_packet(encode_packet(b"retry"))
        self.assertEqual(len(fake.sent_packets), 2)

    def test_receive_until_completion(self):
        fake = FakeSerial(incoming=[encode_packet(b"abc"), encode_packet(b"def"), encode_packet()])
        client = GameBoyIRClient(serial_port=fake)
        client.enable()
        self.assertEqual(client.receive(timeout=1), b"abcdef")

    def test_requires_enable(self):
        client = GameBoyIRClient(serial_port=FakeSerial())
        with self.assertRaises(GameBoyIRException):
            client.send(b"no")

    def test_rejects_invalid_transfer_limits(self):
        client = GameBoyIRClient(serial_port=FakeSerial())
        client.enable()
        for retries in (-1, 1.5, True):
            with self.subTest(retries=retries):
                with self.assertRaises(ValueError):
                    client.send_packet(encode_packet(b"x"), retries=retries)
        for chunk_size in (0, 69, 1.5, True):
            with self.subTest(chunk_size=chunk_size):
                with self.assertRaises(ValueError):
                    client.send(b"x", chunk_size=chunk_size)
        for max_bytes in (-1, 1.5, True):
            with self.subTest(max_bytes=max_bytes):
                with self.assertRaises(ValueError):
                    client.receive(timeout=1, max_bytes=max_bytes)

    def test_rejects_invalid_timeouts(self):
        for timeout in (0, -1, float("nan"), float("inf")):
            with self.subTest(timeout=timeout):
                with self.assertRaises(ValueError):
                    GameBoyIRClient(serial_port=FakeSerial(), timeout=timeout)
        client = GameBoyIRClient(serial_port=FakeSerial())
        client.enable()
        with self.assertRaises(ValueError):
            client.receive_packet(timeout=0)


if __name__ == "__main__":
    unittest.main()

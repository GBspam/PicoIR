# External protocol reference fixtures

These fixtures are specification-derived vectors, not captures from a physical remote. They exercise decoding and encoding independently of the production encoder.

- **JVC**: address 0x59, command 0x35; 38 kHz carrier, 8400/4200 µs header, 525 µs marks, 525/1575 µs spaces, 16 LSB-first bits with address byte before command byte. Reference: Arduino-IRremote's [JVC implementation and IRP](https://arduino-irremote.github.io/Arduino-IRremote/ir__JVC_8hpp_source.html), which cites [SB-Projects JVC protocol](https://www.sbprojects.net/knowledge/ir/jvc.php).
- **Samsung32**: address 0x1234, command 0x56, inverse byte 0xA9; 4500/4500 µs header, 32 LSB-first bits, 560 µs marks, 560/1690 µs spaces, 560 µs stop mark. The test verifies that repeat emits a complete data frame rather than the NEC short-repeat burst. Protocol-family reference: [IRremoteESP8266 Samsung support](https://github.com/crankyoldgit/IRremoteESP8266/blob/master/src/ir_Samsung.h).
- **Kaseikyo length guard**: synthetic 48-bit timing sequence with the documented 3456/1728 µs header and 432 µs unit. This is a negative length/prefix test; its payload is not claimed to be a valid manufacturer frame. Reference: [Arduino-IRremote protocol constants](https://arduino-irremote.github.io/Arduino-IRremote/group__Decoder.html).

These are nominal spec-derived timings, not measured captures. Receiver distortion and actual-device interoperability require physical validation.

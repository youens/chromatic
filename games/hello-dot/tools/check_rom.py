"""Check the release cartridge header and checksums, without an emulator."""
from pathlib import Path
import hashlib
import sys
rom = Path(sys.argv[1]).read_bytes()
assert len(rom) == 32768, f"Unexpected ROM length: {len(rom)}"
assert rom[0x134:0x13D] == b"HELLO DOT"
assert rom[0x143] == 0xC0, "Must be marked Game Boy Color only"
assert rom[0x147:0x14A] == bytes(3), "Expected ROM-only, 32 KiB, no cartridge RAM"
checksum = 0
for byte in rom[0x134:0x14D]:
    checksum = (checksum - byte - 1) & 255
assert checksum == rom[0x14D], "Header checksum mismatch"
assert (sum(rom) - rom[0x14E] - rom[0x14F]) & 65535 == int.from_bytes(rom[0x14E:0x150], 'big'), "Global checksum mismatch"
print(f"PASS: 32 KiB GBC ROM, valid checksums; SHA-256 {hashlib.sha256(rom).hexdigest()}")

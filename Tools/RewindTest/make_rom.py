# Builds rewindtest.nes: a minimal NROM program used to verify rewind determinism.
# Every NMI (once per frame) it reads controller 1 and folds the buttons into
# checksums at $02/$03, logs the buttons to $0200+frame, and counts frames in $00/$04.
# Usage: python make_rom.py <output.nes>
import sys

code = bytearray()
labels = {}
fixups = []  # (offset, label, kind)
ORG = 0xC000

def here(): return ORG + len(code)
def label(n): labels[n] = here()
def emit(*b): code.extend(b)
def abs16(op, lbl): emit(op, 0, 0); fixups.append((len(code) - 2, lbl, "abs"))
def branch(op, lbl): emit(op, 0); fixups.append((len(code) - 1, lbl, "rel"))

# RESET
label("reset")
emit(0x78, 0xD8, 0xA2, 0xFF, 0x9A)          # SEI, CLD, LDX #$FF, TXS
emit(0xA9, 0x00, 0x8D, 0x00, 0x20)          # LDA #0, STA $2000
emit(0x8D, 0x01, 0x20)                      # STA $2001
emit(0xA2, 0x00, 0xA9, 0x00)                # LDX #0, LDA #0
label("clr")
emit(0x95, 0x00, 0x9D, 0x00, 0x02, 0xE8)    # STA $00,X / STA $0200,X / INX
branch(0xD0, "clr")                         # BNE clr
label("vb1"); emit(0x2C, 0x02, 0x20); branch(0x10, "vb1")   # BIT $2002 / BPL
label("vb2"); emit(0x2C, 0x02, 0x20); branch(0x10, "vb2")
emit(0xA9, 0x80, 0x8D, 0x00, 0x20)          # LDA #$80, STA $2000 (NMI on)
label("loop"); abs16(0x4C, "loop")          # JMP loop

# NMI
label("nmi")
emit(0x48, 0x8A, 0x48)                      # PHA, TXA, PHA
emit(0xA9, 0x01, 0x8D, 0x16, 0x40)          # strobe controller
emit(0xA9, 0x00, 0x8D, 0x16, 0x40)
emit(0xA2, 0x08)                            # LDX #8
label("rd")
emit(0xAD, 0x16, 0x40, 0x4A, 0x26, 0x01, 0xCA)  # LDA $4016, LSR, ROL $01, DEX
branch(0xD0, "rd")
emit(0xA5, 0x02, 0x0A, 0x65, 0x01, 0x45, 0x00, 0x85, 0x02)  # $02 = (($02<<1)+$01)^$00
emit(0xA5, 0x03, 0x65, 0x02, 0x85, 0x03)    # $03 += $02
emit(0xA6, 0x00, 0xA5, 0x01, 0x9D, 0x00, 0x02)  # $0200+$00 = buttons
emit(0xE6, 0x00)                            # INC $00
branch(0xD0, "skip")
emit(0xE6, 0x04)                            # INC $04
label("skip")
emit(0x68, 0xAA, 0x68, 0x40)                # PLA, TAX, PLA, RTI
label("irq"); emit(0x40)                    # RTI

for off, lbl, kind in fixups:
    target = labels[lbl]
    if kind == "abs":
        code[off] = target & 0xFF
        code[off + 1] = target >> 8
    else:
        rel = target - (ORG + off + 1)
        assert -128 <= rel <= 127, lbl
        code[off] = rel & 0xFF

prg = bytearray(0x4000)
prg[:len(code)] = code
for addr, lbl in ((0xFFFA, "nmi"), (0xFFFC, "reset"), (0xFFFE, "irq")):
    prg[addr - 0xC000] = labels[lbl] & 0xFF
    prg[addr - 0xC000 + 1] = labels[lbl] >> 8

header = b"NES\x1a" + bytes([1, 1, 0, 0]) + bytes(8)
with open(sys.argv[1], "wb") as f:
    f.write(header + prg + bytes(0x2000))
print("wrote", sys.argv[1], len(code), "bytes of code")

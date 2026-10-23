# Builds scrolltest.nes: a minimal NROM program that scrolls a striped background horizontally
# by exactly 2 pixels per frame. Used to check frame generation: without it, the visible scroll
# position only changes in steps of 2 pixels; with it, in-between positions appear.
# Usage: python make_scroll_rom.py <output.nes>
import sys

code = bytearray()
labels = {}
fixups = []
ORG = 0xC000

def here(): return ORG + len(code)
def label(n): labels[n] = here()
def emit(*b): code.extend(b)
def abs16(op, lbl): emit(op, 0, 0); fixups.append((len(code) - 2, lbl, "abs"))
def branch(op, lbl): emit(op, 0); fixups.append((len(code) - 1, lbl, "rel"))

label("reset")
emit(0x78, 0xD8, 0xA2, 0xFF, 0x9A)              # SEI, CLD, LDX #$FF, TXS
emit(0xA9, 0x00, 0x8D, 0x00, 0x20, 0x8D, 0x01, 0x20)  # PPUCTRL = PPUMASK = 0
emit(0x85, 0x00, 0x85, 0x01)                    # scroll X ($00) = 0, nametable bit ($01) = 0
label("vb1"); emit(0x2C, 0x02, 0x20); branch(0x10, "vb1")
label("vb2"); emit(0x2C, 0x02, 0x20); branch(0x10, "vb2")

# Palette: $3F00-$3F0F
emit(0xA9, 0x3F, 0x8D, 0x06, 0x20, 0xA9, 0x00, 0x8D, 0x06, 0x20)
emit(0xA2, 0x00)
label("pal")
abs16(0xBD, "paldata")                          # LDA paldata,X
emit(0x8D, 0x07, 0x20, 0xE8, 0xE0, 0x10)        # STA $2007, INX, CPX #16
branch(0xD0, "pal")

# Nametables 0 and 1 ($2000-$27FF): tile = column index & 15 (attributes get the same values -> varied palettes)
emit(0xA9, 0x20, 0x8D, 0x06, 0x20, 0xA9, 0x00, 0x8D, 0x06, 0x20)
emit(0xA0, 0x08)                                # LDY #8 (8 pages of 256 bytes)
emit(0xA2, 0x00)                                # LDX #0
label("fill")
emit(0x8A, 0x29, 0x0F, 0x8D, 0x07, 0x20, 0xE8)  # TXA, AND #15, STA $2007, INX
branch(0xD0, "fill")
emit(0x88)                                      # DEY
branch(0xD0, "fill")

emit(0x2C, 0x02, 0x20)                          # reset address latch
emit(0xA9, 0x00, 0x8D, 0x05, 0x20, 0x8D, 0x05, 0x20)  # scroll 0,0
emit(0xA9, 0x0A, 0x8D, 0x01, 0x20)              # PPUMASK: show background (incl. left 8 pixels)
emit(0xA9, 0x80, 0x8D, 0x00, 0x20)              # PPUCTRL: NMI on
label("loop"); abs16(0x4C, "loop")

# NMI: scroll += 2 (toggling the nametable when crossing 256), then set the scroll registers
label("nmi")
emit(0x48)                                      # PHA
emit(0xA5, 0x00, 0x18, 0x69, 0x02, 0x85, 0x00)  # LDA $00, CLC, ADC #2, STA $00
branch(0x90, "noflip")
emit(0xA5, 0x01, 0x49, 0x01, 0x85, 0x01)        # $01 ^= 1
label("noflip")
emit(0x2C, 0x02, 0x20)                          # BIT $2002
emit(0xA5, 0x00, 0x8D, 0x05, 0x20)              # X scroll
emit(0xA9, 0x00, 0x8D, 0x05, 0x20)              # Y scroll
emit(0xA5, 0x01, 0x09, 0x80, 0x8D, 0x00, 0x20)  # PPUCTRL = $80 | nametable
emit(0x68, 0x40)                                # PLA, RTI
label("irq"); emit(0x40)

label("paldata")
emit(0x0F, 0x30, 0x16, 0x2A, 0x0F, 0x12, 0x27, 0x30, 0x0F, 0x1A, 0x00, 0x24, 0x0F, 0x38, 0x01, 0x2C)

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

# CHR: 16 tiles with different vertical stripe patterns, so every column of the screen is distinctive
chr_rom = bytearray(0x2000)
for n in range(16):
    p0 = (n * 37 + 11) & 0xFF
    p1 = (n * 91 + 5) & 0xFF
    for row in range(8):
        chr_rom[n * 16 + row] = p0
        chr_rom[n * 16 + 8 + row] = p1

header = b"NES\x1a" + bytes([1, 1, 1, 0]) + bytes(8)  # vertical mirroring (nametables side by side)
with open(sys.argv[1], "wb") as f:
    f.write(header + prg + chr_rom)
print("wrote", sys.argv[1], len(code), "bytes of code")

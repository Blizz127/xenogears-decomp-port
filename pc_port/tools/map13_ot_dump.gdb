set pagination off
set confirm off
set env SDL_VIDEODRIVER x11
set env SDL_AUDIODRIVER dummy
set env XENO_FIELD_TEST 1
set env XENO_KERNEL_SEL 0
set env XENO_FIELD_MAP 13
set env XENO_FIELD_ENTRANCE 1
set env XENO_FIELD_TEST_INPUT 0:0x6000,22:0

break DrawOTag if g_FieldDiagFrameCount >= 28
commands
silent
printf "OT_DUMP frame=%d head=%p\n", g_FieldDiagFrameCount, $rdi
python
import gdb
import struct

inferior = gdb.selected_inferior()
head = int(gdb.parse_and_eval("$rdi"))
link = struct.unpack("<I", inferior.read_memory(head, 4).tobytes())[0]
step = 0
primitive = 0

def signed16(value):
    return value - 65536 if value >= 32768 else value

layouts = {
    0x20: [8, 12, 16],
    0x24: [8, 16, 24],
    0x28: [8, 12, 16, 20],
    0x2c: [8, 16, 24, 32],
    0x30: [8, 16, 24],
    0x34: [8, 20, 32],
    0x38: [8, 16, 24, 32],
    0x3c: [8, 20, 32, 44],
}

while (link & 0xffffff) != 0xffffff and step < 200000:
    address = link & 0xffffff
    length = (link >> 24) & 0xff
    try:
        raw = inferior.read_memory(address, max(4, (length + 1) * 4)).tobytes()
    except gdb.MemoryError:
        print("OT_MEMERR step=%d addr=%06x len=%d" %
              (step, address, length))
        break
    if length:
        code = raw[7] if len(raw) > 7 else -1
        rgb = tuple(raw[4:7]) if len(raw) >= 7 else (0, 0, 0)
        xy = []
        for offset in layouts.get(code & 0xfc, []):
            if offset + 4 <= len(raw):
                x, y = struct.unpack_from("<HH", raw, offset)
                xy.append((signed16(x), signed16(y)))
        print("OT_PRIM n=%d step=%d addr=%06x len=%d code=%02x "
              "rgb=%02x,%02x,%02x xy=%s raw=%s" %
              (primitive, step, address, length, code, rgb[0], rgb[1],
               rgb[2], xy, raw.hex()))
        primitive += 1
    link = struct.unpack_from("<I", raw, 0)[0]
    step += 1

print("OT_END steps=%d prims=%d" % (step, primitive))

# Candidate Map 13 wall/cutaway FT4 at packet 0x6c5920 uses tpage 0x0087,
# CLUT 0x7940, and UV rectangle u=0..53/v=29..127.  Sample the CPU VRAM
# mirror at the exact PSX 8-bpp page and palette so a missing upload can be
# distinguished from bad host-side texture sampling.
vram = int(gdb.parse_and_eval("&vram[0]"))
page_x = (0x0087 & 0x0f) * 64
page_y = ((0x0087 >> 4) & 1) * 256
clut_x = (0x7940 & 0x3f) * 16
clut_y = (0x7940 >> 6) & 0x1ff
indices = {}
colors = {}
for v in range(29, 128):
    for u in range(0, 54):
        word_offset = ((page_y + v) * 1024 + page_x + (u >> 1)) * 2
        word = struct.unpack("<H", inferior.read_memory(vram + word_offset, 2).tobytes())[0]
        index = (word >> (8 if (u & 1) else 0)) & 0xff
        indices[index] = indices.get(index, 0) + 1
for index, count in indices.items():
    color_offset = (clut_y * 1024 + clut_x + index) * 2
    color = struct.unpack("<H", inferior.read_memory(vram + color_offset, 2).tobytes())[0]
    colors[color] = colors.get(color, 0) + count
print("OT_TEX tpage=0087 depth=8bpp page=%d,%d clut=7940 clutxy=%d,%d "
      "indices=%d zero_indices=%d colors=%d black_texels=%d top_indices=%s "
      "top_colors=%s" %
      (page_x, page_y, clut_x, clut_y, len(indices), indices.get(0, 0),
       len(colors), colors.get(0, 0),
       sorted(indices.items(), key=lambda item: item[1], reverse=True)[:12],
       [("%04x" % color, count) for color, count in
        sorted(colors.items(), key=lambda item: item[1], reverse=True)[:12]]))
end
quit
end

run

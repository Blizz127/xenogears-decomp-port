import struct

def decompress(in_file, out_file):
    comp_size = len(in_file.read())
    in_file.seek(0)
    decomp_size = struct.unpack("<I", in_file.read(4))[0]

    while out_file.tell() < decomp_size:
        ctl_byte = int.from_bytes(in_file.read(1), "little")
        ctl_bit = ctl_byte & 1
        for i in range(8):
            if in_file.tell() >= comp_size:
                return

            ctl_byte = ctl_byte >> 1

            if ctl_bit == 0:
                out_file.write(in_file.read(1))
            else:
                v1 = int.from_bytes(in_file.read(1), "little")
                v0 = int.from_bytes(in_file.read(1), "little")
                offset = ((v0 & 0xF) << 8) | v1
                length = (v0 >> 4) + 3

                of_cur = out_file.tell()
                readback = out_file.tell() - offset

                for i in range(length):
                    out_file.seek(readback + i)
                    tmp = out_file.read(1)
                    out_file.seek(of_cur + i)
                    out_file.write(tmp)
            ctl_bit = ctl_byte & 1

import math
import mmap
import yaml
import psx.lzss as lzss

from cdrom.cdxa import CdromXa

"""
Parses a YAML file into Overlay objects that can be fed into OverlayExtractor.extract()
"""
def parse_yaml(yaml_file: str):
    with open(yaml_file) as fh:
        spec = yaml.safe_load(fh)

    if 'overlays' not in spec:
        raise ValueError("Invalid yaml: top level key 'overlays' not found")

    return [ Overlay(**ovl) for ovl in spec['overlays']]

class Overlay(object):
    def __init__(self, name: str, file_id: int, directory_id: int, compressed: bool=False):
        self.compressed = compressed
        self.name = name
        self.file_id = file_id
        self.directory_id = directory_id


class OverlayExtractor(object):
    def __init__(self, disc: CdromXa):
        header = disc.read_sector(0x28)
        disc.seek_sector(0x18)
        table = b''
        for i in range(0x10):
            table += disc.read_next_sector()
        self.header = header
        self.table = table
        self.current_directory_id = 0
        self.disc = disc

    def extract(self, overlay: Overlay, output_dir='.'):
        self.__set_directory_id(overlay.directory_id)
        sector = self.__get_starting_sector(overlay.file_id)
        size = self.__get_size_aligned(overlay.file_id)
        file_path = output_dir + "/" + overlay.name

        if overlay.compressed:
            size = math.ceil(
                self.__get_size_aligned(overlay.file_id) / CdromXa.SECTOR_DATA_SIZE) * CdromXa.SECTOR_DATA_SIZE

            with mmap.mmap(-1, size) as compressed:
                self.disc.extract_sectors(sector, size, compressed)
                compressed.seek(0)
                with open(file_path, "+wb") as uncompressed:
                    lzss.decompress(compressed, uncompressed)
        else:
            with open(file_path, "wb") as fh:
                self.disc.extract_sectors(sector, size, fh)

    def __set_directory_id(self, nybble_index, offset=0):
        byte_offset = offset + (nybble_index * 2)
        lsb, msb = self.header[byte_offset:byte_offset+2]
        self.current_directory_id = (msb << 8) + lsb - 1


    def __get_starting_sector(self, index):
        offset = self.__get_file_id_offset(index)
        return (self.table[offset]
                + (self.table[offset + 1] <<8)
                + (self.table[offset + 2] <<16))

    def __get_size(self, index):
        offset = self.__get_file_id_offset(index)
        return (self.table[offset+3]
                + (self.table[offset+4] <<8)
                + (self.table[offset+5] <<16)
                + (self.table[offset+6] <<24))

    def __get_file_id_offset(self, file_id):
        return (file_id + self.current_directory_id - 1) * 7


    def __get_size_aligned(self, index):
        size = self.__get_size(index)
        v = size + 3
        if v < 0:
            v = size + 6
        return (v >> 2) << 2


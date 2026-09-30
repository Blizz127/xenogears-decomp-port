import math


class CdromXa(object):
    SECTOR_SIZE = 2352
    SECTOR_DATA_SIZE = 0x800

    def __init__(self, filename: str):
        self.filename = filename
        self.handle = None
        self.current_sector = 0

    def __enter__(self):
        self.handle = open(self.filename, "rb")
        return self

    def __exit__(self, exc_type, exc_val, exc_tb):
        if self.handle is not None:
            self.handle.close()
            self.handle = None

    def seek_sector(self, sector: int):
        self.current_sector = sector
        self.handle.seek(sector * CdromXa.SECTOR_SIZE, 0)

    def read_sector(self, sector: int):
        self.seek_sector(sector)
        return self.read_next_sector()

    def read_next_sector(self):
        sector_data = self.__read_sector_xa_mode2_form1()
        self.current_sector += 1

        return sector_data

    def __read_sector_xa_mode2_form1(self):
        sync_pattern = self.handle.read(12)
        address = self.handle.read(3)
        mode = self.handle.read(1)
        subheader = self.handle.read(4)
        subheader_copy = self.handle.read(4)
        data = self.handle.read(2048)
        error_detection = self.handle.read(4)
        error_correction = self.handle.read(276)
        assert mode == b'\x02'
        assert sync_pattern == b'\x00\xff\xff\xff\xff\xff\xff\xff\xff\xff\xff\x00'
        assert subheader == subheader_copy
        return data

    def extract_sectors(self, start_sector, byte_count, fh):
        self.seek_sector(start_sector)
        count = math.ceil(byte_count / CdromXa.SECTOR_DATA_SIZE)
        for i in range(count):
            fh.write(self.read_next_sector())

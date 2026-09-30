import struct


class VolumeDescriptor:
    def __init__(self, data):
        self.vol_descriptor_type = data[0]
        self.standard_identifier = data[1:6]
        self.vol_descriptor_version = data[6:7]
        self.system_identifier = data[8:40]
        self.volume_identifier = data[0x28:0x48]
        self.vol_space_size = struct.unpack("<I", data[0x50:0x54])[0]
        self.vol_space_size_c = struct.unpack(">I", data[0x54:0x58])[0]
        assert self.vol_space_size == self.vol_space_size_c
        self.vol_seq_number = struct.unpack("<H", data[0x7C:0x7E])[0]
        vol_seq_number_c = struct.unpack(">H", data[0x7E:0x80])[0]
        assert self.vol_seq_number == vol_seq_number_c
        self.logical_block_size = struct.unpack("<H", data[0x80:0x82])[0]
        logical_block_size_c = struct.unpack(">H", data[0x82:0x84])[0]
        assert self.logical_block_size == logical_block_size_c
        self.pt_size = struct.unpack("<I", data[0x84:0x88])[0]
        pt_size_c = struct.unpack(">I", data[0x88:0x8C])[0]
        assert self.pt_size == pt_size_c
        self.pt1_block_number = struct.unpack("<I", data[0x8C:0x90])[0]
        self.pt2_block_number = struct.unpack("<I", data[0x90:0x94])[0]
        self.pt3_block_number = struct.unpack(">I", data[0x94:0x98])[0]
        self.pt4_block_number = struct.unpack(">I", data[0x98:0x9C])[0]
        self.app_identifier = data[0x23E:0x23E + 128]
        self.copyright_filename = data[0x2BE:0x2BE + 37]
        self.abstract_filename = data[0x2E3:0x2E3 + 37]
        self.bibliographic_filename = data[0x308:0x308 + 37]
        self.vol_creation_timestamp = data[0x32D:0x32D + 17]
        self.cd_xa_id_sig = data[0x400:0x408]
        self.root_dir_record = DirectoryRecord(data[0x9C:0x9C + 34])

    def print(self):
        print()
        print("Volume Descriptor")
        print("------------------")
        if self.vol_descriptor_type == 0x1:
            print("Volume Descriptor Type: Primary Volume Descriptor")
        elif self.vol_descriptor_type == 0x2:
            print("Volume Descriptor Type: Supplementary Volume Descriptor")
        print("Standard Identifier:", self.standard_identifier)
        if self.vol_descriptor_version == 0x1:
            print("Volume Descriptor Version: Standard")
        print("System Identifier:", self.system_identifier)
        print("Volume Identifier:", self.volume_identifier)
        print("Volume Sequence Number:", hex(self.vol_seq_number))
        print("Volume Space Size:", hex(self.vol_space_size))
        print("Logical block size:", hex(self.logical_block_size))
        print("Path Table Size in Bytes:", hex(self.pt_size))
        print("Path Table 1 Block Number:", self.pt1_block_number)
        print("Path Table 2 Block Number:", self.pt2_block_number)
        print("Path Table 3 Block Number:", self.pt3_block_number)
        print("Path Table 4 Block Number:", self.pt4_block_number)
        self.root_dir_record.print()
        print("Application Identifier:", self.app_identifier)
        print("Copyright Filename:", self.copyright_filename)
        print("Abstract Filename:", self.abstract_filename)
        print("Bibliographic Filename:", self.bibliographic_filename)
        print("Volume Creation Timestamp", self.vol_creation_timestamp)
        print("CD-XA Identifying Signature:", self.cd_xa_id_sig)
        print()

        # assert data[0x0] == 0x1 # Volume descriptor type == Primary volume descriptor
        # assert data[0x1 : 0x1 + 5] == b'CD001' # Standard Identifier
        # assert data[0x6 : 0x6 + 1] == b'\x01' # Volume descriptor version == Standard
        # assert data[0x8 : 0x8 + 11] == b'PLAYSTATION' # System Identifier


class DirectoryRecord:
    def __init__(self, data):
        self.length_of_directory_record = data[0]
        self.data_logical_block_num = struct.unpack("<I", data[2:6])[0]
        data_logical_block_num_c = struct.unpack(">I", data[6:10])[0]
        assert self.data_logical_block_num == data_logical_block_num_c
        self.data_size = struct.unpack("<I", data[10:14])[0]
        data_size_c = struct.unpack(">I", data[14:18])[0]
        assert self.data_size == data_size_c
        self.file_flags = data[0x19]
        self.name_len = data[0x20]
        self.name = data[0x21: 0x21 + self.name_len]

    def print(self):
        print()
        print("Directory record")
        print("------------------")
        print("Length of Directory Record:", hex(self.length_of_directory_record))
        print("Data Logical Block Number:", self.data_logical_block_num)
        print("Data Size in Bytes:", hex(self.data_size))
        if self.file_flags == 0x0:
            print("File Flags: File")
        elif self.file_flags == 0x2:
            print("File Flags: Directory")
        print("Name:", self.name)
        print()


class PathTableEntry:
    def __init__(self, data):
        self.dir_name_len = data[0]
        self.ext_attr_record_len = data[1]
        self.dir_logical_block_num = data[2:6]
        self.parent_dir_number = data[6:8]
        self.dir_name = data[8:8 + self.dir_name_len]
        self.size = self.dir_name_len + 8 + 1

    def print(self):
        print()
        print("Path Table Entry")
        print("------------------")
        print("Directory name:", self.dir_name)
        print()


class PathTable:
    def __init__(self, data, table_size):
        self.entries = []
        cur_idx = 0
        while cur_idx < table_size:
            entry = PathTableEntry(data[cur_idx: table_size])
            self.entries.append(entry)
            cur_idx += entry.size

    def print(self):
        for entry in self.entries:
            entry.print()

import struct
import sys
import os
from elftools.elf.elffile import ELFFile

SECTOR_SIZE = 512

def parse_map_symbols(map_file_path):
    """Extracts symbol addresses from map file."""
    symbols = {}
    with open(map_file_path, 'r') as f:
        to_try = ['phys', 'stage_2_info']
        for line in f:
            for sym in to_try:
                if sym in line:
                    parts = line.split()
                    for part in parts:
                        try:
                            addr = int(part, 16)
                            symbols[sym] = addr
                            to_try.remove(sym)
                            break
                        except ValueError:
                            continue
    if 'phys' not in symbols or 'stage_2_info' not in symbols:
        raise RuntimeError("Missing 'phys' or 'stage_2_info' symbol in map file")
    return symbols['phys'], symbols['stage_2_info']

def get_elf_info(elf_path):
    """Gets entry point and first PT_LOAD physical address from ELF."""
    with open(elf_path, 'rb') as f:
        elf = ELFFile(f)
        entry = elf.header['e_entry']
        for segment in elf.iter_segments():
            if segment['p_type'] == 'PT_LOAD':
                return entry, segment['p_paddr'] - segment['p_offset']
    raise RuntimeError("No PT_LOAD segment found in ELF")

def get_elf_file_size_in_sectors(elf_path):
    """Gets size of ELF file in 512-byte sectors."""
    size_bytes = os.path.getsize(elf_path)
    return (size_bytes + SECTOR_SIZE - 1) // SECTOR_SIZE

def patch_binary(bin_path, file_offset, lba, load, start, size):
    """Patches the binary at the given offset with stage_2_info data."""
    with open(bin_path, 'r+b') as f:
        f.seek(file_offset)
        packed = struct.pack('<BQHH', size, lba, load, start)
        f.write(packed)

def main(bin_path, map_path, elf_path, lba=2):
    print("[*] Reading map file...")
    phys, stage2_info_addr = parse_map_symbols(map_path)
    file_offset = stage2_info_addr - phys
    if file_offset < 0:
        raise ValueError(f"stage_2_info address {hex(stage2_info_addr)} is before phys {hex(phys)}")
    print(f"    phys:             0x{phys:X}")
    print(f"    stage_2_info:     0x{stage2_info_addr:X}")
    print(f"    file offset:      0x{file_offset:X}")

    print("[*] Reading ELF...")
    entry, load_addr = get_elf_info(elf_path)
    sectors = get_elf_file_size_in_sectors(elf_path)
    print(f"    Entry point:      0x{entry:X}")
    print(f"    Load address:     0x{load_addr:X}")
    print(f"    File size:        {sectors} sector(s)")

    print("[*] Patching binary...")
    patch_binary(bin_path, file_offset, lba, load_addr & 0xFFFF, entry & 0xFFFF, sectors)
    print("[+] Done.")

if __name__ == '__main__':
    if len(sys.argv) != 4:
        print(f"Usage: {sys.argv[0]} <raw_binary.bin> <linker.map> <stage2.elf>")
        sys.exit(1)
    main(sys.argv[1], sys.argv[2], sys.argv[3])

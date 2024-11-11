import argparse
from elftools.elf.elffile import ELFFile
import sys

symbols_to_check = ["esp_amp_start_subcore", "esp_amp_load_sub"]

def check_symbols(elf_file_path):
    try:
        with open(elf_file_path, 'rb') as elf_file:
            elf = ELFFile(elf_file)

            # Collect all symbols from the symbol tables
            found_symbols = set()
            for section in elf.iter_sections():
                if section.header['sh_type'] in ['SHT_SYMTAB', 'SHT_DYNSYM']:
                    for symbol in section.iter_symbols():
                        found_symbols.add(symbol.name)

            # Check if each of the given symbols exists in the ELF file
            all_symbols_found = True

            for symbol in symbols_to_check:
                if symbol not in found_symbols:
                    print(f"The symbol '{symbol}' is NOT present in '{elf_file_path}'.")
                    all_symbols_found = False

            return all_symbols_found

    except Exception as e:
        print(f"Error reading ELF file: {e}")
        sys.exit(1)

if __name__ == "__main__":
    parser = argparse.ArgumentParser(description="Check if specified symbols exist in an ELF file.")
    parser.add_argument("--elf_file", help="Path to the ELF file to check.")
    args = parser.parse_args()

    result = check_symbols(args.elf_file)
    if not result:
        sys.exit(1)

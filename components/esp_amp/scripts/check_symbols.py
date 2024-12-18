import argparse
from elftools.elf.elffile import ELFFile
import sys

symbols_to_check = ["esp_amp_init", "esp_amp_start_subcore", "esp_amp_load_sub"]

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

            missing_symbols = set(symbols_to_check) - found_symbols
            missing_symbols_str = '\n'.join(missing_symbols)
            print(f"\033[1;33mThe following symbols are missing:\n{missing_symbols_str}\033[0m") if missing_symbols else None

    except Exception as e:
        print(f"\033[1;31mError reading ELF file: {e}\033[0m")
        sys.exit(1)

if __name__ == "__main__":
    parser = argparse.ArgumentParser(description="Check if specified symbols exist in an ELF file.")
    parser.add_argument("--elf_file", help="Path to the ELF file to check.")
    args = parser.parse_args()

    result = check_symbols(args.elf_file)

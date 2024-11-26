set(CMAKE_SYSTEM_NAME Generic)

set(CMAKE_C_COMPILER "riscv32-esp-elf-gcc")
set(CMAKE_CXX_COMPILER "riscv32-esp-elf-g++")
set(CMAKE_ASM_COMPILER "riscv32-esp-elf-gcc")
set(CMAKE_OBJCOPY "riscv32-esp-elf-objcopy")
set(_CMAKE_TOOLCHAIN_PREFIX "riscv32-esp-elf-")

set(CMAKE_C_FLAGS "-march=rv32imafc_zicsr_zifencei_xesppie -mabi=ilp32f"
    CACHE STRING "C Compiler Base Flags")

set(CMAKE_CXX_FLAGS "-march=rv32imafc_zicsr_zifencei_xesppie -mabi=ilp32f"
    CACHE STRING "C++ Compiler Base Flags")

set(CMAKE_ASM_FLAGS "-march=rv32imafc_zicsr_zifencei_xesppie -mabi=ilp32f"
    CACHE STRING "Asm Compiler Base Flags")

set(CMAKE_EXE_LINKER_FLAGS "-nostartfiles -march=rv32imafc_zicsr_zifencei_xesppie -mabi=ilp32f --specs=nano.specs --specs=nosys.specs"
    CACHE STRING "Linker Base Flags")

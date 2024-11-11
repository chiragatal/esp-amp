# CMake toolchain file for ULP LP core
set(CMAKE_SYSTEM_NAME Generic)

set(CMAKE_C_COMPILER "riscv32-esp-elf-gcc")
set(CMAKE_CXX_COMPILER "riscv32-esp-elf-g++")
set(CMAKE_ASM_COMPILER "riscv32-esp-elf-gcc")
set(CMAKE_OBJCOPY "riscv32-esp-elf-objcopy")
set(_CMAKE_TOOLCHAIN_PREFIX "riscv32-esp-elf-")

set(CMAKE_C_FLAGS "-Os -ggdb -march=rv32imac_zicsr_zifencei -mdiv -fdata-sections -ffunction-sections -fno-builtin"
    CACHE STRING "C Compiler Base Flags")
set(CMAKE_CXX_FLAGS "-Os -ggdb -march=rv32imac_zicsr_zifencei -mdiv -fdata-sections -ffunction-sections -fno-builtin"
    CACHE STRING "C++ Compiler Base Flags")
set(CMAKE_ASM_FLAGS "-Os -ggdb -march=rv32imac_zicsr_zifencei -x assembler-with-cpp -fno-builtin"
    CACHE STRING "Assembler Base Flags")
set(CMAKE_EXE_LINKER_FLAGS "-march=rv32imac_zicsr_zifencei --specs=nosys.specs --specs=nano.specs"
    CACHE STRING "Linker Base Flags")
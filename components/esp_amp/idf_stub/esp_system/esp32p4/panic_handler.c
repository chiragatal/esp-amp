/*
* SPDX-FileCopyrightText: 2024 Espressif Systems (Shanghai) CO LTD
*
* SPDX-License-Identifier: Apache-2.0
*/

#include "esp_rom_sys.h"
#include "esp_rom_uart.h"
#include "riscv/rvruntime-frames.h"
#include "riscv/csr.h"

#define LDTVAL0         0xBE8
#define LDTVAL1         0xBE9
#define STTVAL0         0xBF8
#define STTVAL1         0xBF9
#define STTVAL2         0xBFA

static void panic_print_char_uart(const char c)
{
    esp_rom_output_putc(c);
}

void panic_print_char(const char c)
{
    panic_print_char_uart(c);
}

static void panic_print_str(const char *str)
{
    for (int i = 0; str[i] != 0; i++) {
        panic_print_char(str[i]);
    }
}

static void panic_print_hex(int h)
{
    int x;
    int c;
    // Does not print '0x', only the digits (8 digits to print)
    for (x = 0; x < 8; x++) {
        c = (h >> 28) & 0xf; // extract the leftmost byte
        if (c < 10) {
            panic_print_char('0' + c);
        } else {
            panic_print_char('a' + c - 10);
        }
        h <<= 4; // move the 2nd leftmost byte to the left, to be extracted next
    }
}

static void dump_stack(RvExcFrame *frame, int exccause)
{
    uint32_t i = 0;
    uint32_t sp = frame->sp;
    panic_print_str("\n\nStack memory:\n");
    const int per_line = 8;
    for (i = 0; i < 1024; i += per_line * sizeof(uint32_t)) {
        uint32_t *spp = (uint32_t *)(sp + i);
        panic_print_hex(sp + i);
        panic_print_str(": ");
        for (int y = 0; y < per_line; y++) {
            panic_print_str("0x");
            panic_print_hex(spp[y]);
            panic_print_char(y == per_line - 1 ? '\n' : ' ');
        }
    }
    panic_print_str("\n");
}

static const char *desc[] = {
    "MEPC    ", "RA      ", "SP      ", "GP      ", "TP      ", "T0      ", "T1      ", "T2      ",
    "S0/FP   ", "S1      ", "A0      ", "A1      ", "A2      ", "A3      ", "A4      ", "A5      ",
    "A6      ", "A7      ", "S2      ", "S3      ", "S4      ", "S5      ", "S6      ", "S7      ",
    "S8      ", "S9      ", "S10     ", "S11     ", "T3      ", "T4      ", "T5      ", "T6      ",
    "MSTATUS ", "MTVEC   ", "MCAUSE  ", "MTVAL   ", "MHARTID "
};

STRUCT_BEGIN
STRUCT_FIELD(long, 4, RV_BUS_LDPC0,   ldpc0)           /* Load bus error PC register 0 */
STRUCT_FIELD(long, 4, RV_BUS_LDTVAL0, ldtval0)         /* Load bus error access address register 0 */
STRUCT_FIELD(long, 4, RV_BUS_LDPC1,   ldpc1)           /* Load bus error PC register 1 */
STRUCT_FIELD(long, 4, RV_BUS_LDTVAL1, ldtval1)         /* Load bus error access address register 1 */
STRUCT_FIELD(long, 4, RV_BUS_STPC0,   stpc0)           /* Store bus error PC register 0 */
STRUCT_FIELD(long, 4, RV_BUS_STTVAL0, sttval0)         /* Store bus error access address register 0 */
STRUCT_FIELD(long, 4, RV_BUS_STPC1,   stpc1)           /* Store bus error PC register 1 */
STRUCT_FIELD(long, 4, RV_BUS_STTVAL1, sttval1)         /* Store bus error access address register 1*/
STRUCT_FIELD(long, 4, RV_BUS_STPC2,   stpc2)           /* Store bus error PC register 2 */
STRUCT_FIELD(long, 4, RV_BUS_STTVAL2, sttval2)         /* Store bus error access address register 2*/
STRUCT_END(RvExtraExcFrame)

static const char *extra_desc[] = {
    "LDPC0   ", "LDTVAL0 ", "LDPC1   ", "LDTVAL1 ", "STPC0   ", "STTVAL0 ", "STPC1   ", "STTVAL1 ",
    "STPC2   ", "STTVAL2 "
};

static RvExtraExcFrame extraFrame;

static const char *reason[] = {
    "Instruction address misaligned",
    "Instruction access fault",
    "Illegal instruction",
    "Breakpoint",
    "Load address misaligned",
    "Load access fault",
    "Store address misaligned",
    "Store access fault",
    "Environment call from U-mode",
    "Environment call from S-mode",
    NULL,
    "Environment call from M-mode",
    "Instruction page fault",
    "Load page fault",
    NULL,
    "Store page fault",
};

void panic_handler(RvExcFrame *frame, int exccause)
{
#define DIM(arr) (sizeof(arr)/sizeof(*arr))

    const char *exccause_str = "Unhandled interrupt/Unknown cause";

    if (exccause < DIM(reason) && reason[exccause] != NULL) {
        exccause_str = reason[exccause];
    }

    panic_print_str("Guru Meditation Error: SubCore panic'ed ");
    panic_print_str(exccause_str);
    panic_print_str("\n");
    panic_print_str("Core 1 register dump:\n");

    uint32_t* frame_ints = (uint32_t*) frame;
    for (int x = 0; x < DIM(desc); x++) {
        if (desc[x][0] != 0) {
            const int not_last = (x + 1) % 4;
            panic_print_str(desc[x]);
            panic_print_str(": 0x");
            panic_print_hex(frame_ints[x]);
            panic_print_char(not_last ? ' ' : '\n');
        }
    }

    extraFrame.ldpc0    = RV_READ_CSR(LDPC0);
    extraFrame.ldtval0  = RV_READ_CSR(LDTVAL0);
    extraFrame.ldpc1    = RV_READ_CSR(LDPC1);
    extraFrame.ldtval1  = RV_READ_CSR(LDTVAL1);
    extraFrame.stpc0    = RV_READ_CSR(STPC0);
    extraFrame.sttval0  = RV_READ_CSR(STTVAL0);
    extraFrame.stpc1    = RV_READ_CSR(STPC1);
    extraFrame.sttval1  = RV_READ_CSR(STTVAL1);
    extraFrame.stpc2    = RV_READ_CSR(STPC2);
    extraFrame.sttval2  = RV_READ_CSR(STTVAL2);

    panic_print_str("\n");
    frame_ints = (uint32_t*)(&extraFrame);
    for (int x = 0; x < DIM(extra_desc); x++) {
        if (extra_desc[x][0] != 0) {
            const int not_last = (x + 1) % 4;
            panic_print_str(extra_desc[x]);
            panic_print_str(": 0x");
            panic_print_hex(frame_ints[x]);
            panic_print_char(not_last ? ' ' : '\n');
        }
    }

    dump_stack(frame, exccause);

    /* idf-monitor uses this string to mark the end of a panic dump */
    panic_print_str("ELF file SHA256: No SHA256 Embedded\n");
}

void xt_unhandled_exception(RvExcFrame *frame)
{
    panic_handler(frame, frame->mcause);
    while (1);
}

void panicHandler(RvExcFrame *frame)
{
    panic_handler(frame, frame->mcause);
    while (1);
}

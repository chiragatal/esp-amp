/*
 * SPDX-FileCopyrightText: 2024 Espressif Systems (Shanghai) CO LTD
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include "sdkconfig.h"
#include "riscv/rvruntime-frames.h"

/* Copy from portasm.S under IDF */
#if CONFIG_ESP_AMP_SUBCORE_ENABLE_HW_FPU
/* Bit to set in mstatus to enable the FPU */
#define CSR_MSTATUS_FPU_ENABLE      (1 << 13)
/* Bit to clear in mstatus to disable the FPU */
#define CSR_MSTATUS_FPU_DISABLE     (3 << 13)

    .section .data,"aw"
    .align 3
    .global fpu_ctx
fpu_ctx:
    .space 160

    .align 2
    .global _bm_intr_nesting_cnt
.align 3
_bm_intr_nesting_cnt:
    .word 0

.macro add_intr_nesting imm=1
    la t0, _bm_intr_nesting_cnt
    lw t1, 0(t0)
    addi t1, t1, \imm
    sw t1, 0(t0)
.endm

.macro fpu_save_regs frame
    la      \frame, fpu_ctx
    fsw     ft0,  0(\frame)
    fsw     ft1,  4(\frame)
    fsw     ft2,  8(\frame)
    fsw     ft3,  12(\frame)
    fsw     ft4,  16(\frame)
    fsw     ft5,  20(\frame)
    fsw     ft6,  24(\frame)
    fsw     ft7,  28(\frame)
    fsw     fs0,  32(\frame)
    fsw     fs1,  36(\frame)
    fsw     fa0,  40(\frame)
    fsw     fa1,  44(\frame)
    fsw     fa2,  48(\frame)
    fsw     fa3,  52(\frame)
    fsw     fa4,  56(\frame)
    fsw     fa5,  60(\frame)
    fsw     fa6,  64(\frame)
    fsw     fa7,  68(\frame)
    fsw     fs2,  72(\frame)
    fsw     fs3,  76(\frame)
    fsw     fs4,  80(\frame)
    fsw     fs5,  84(\frame)
    fsw     fs6,  88(\frame)
    fsw     fs7,  92(\frame)
    fsw     fs8,  96(\frame)
    fsw     fs9,  100(\frame)
    fsw     fs10, 104(\frame)
    fsw     fs11, 108(\frame)
    fsw     ft8,  112(\frame)
    fsw     ft9,  116(\frame)
    fsw     ft10, 120(\frame)
    fsw     ft11, 124(\frame)
    csrr    t1,   fcsr
    sw      t1,   128(\frame)
.endm

.macro fpu_restore_regs frame
    la      \frame, fpu_ctx
    flw     ft0,  0(\frame)
    flw     ft1,  4(\frame)
    flw     ft2,  8(\frame)
    flw     ft3,  12(\frame)
    flw     ft4,  16(\frame)
    flw     ft5,  20(\frame)
    flw     ft6,  24(\frame)
    flw     ft7,  28(\frame)
    flw     fs0,  32(\frame)
    flw     fs1,  36(\frame)
    flw     fa0,  40(\frame)
    flw     fa1,  44(\frame)
    flw     fa2,  48(\frame)
    flw     fa3,  52(\frame)
    flw     fa4,  56(\frame)
    flw     fa5,  60(\frame)
    flw     fa6,  64(\frame)
    flw     fa7,  68(\frame)
    flw     fs2,  72(\frame)
    flw     fs3,  76(\frame)
    flw     fs4,  80(\frame)
    flw     fs5,  84(\frame)
    flw     fs6,  88(\frame)
    flw     fs7,  92(\frame)
    flw     fs8,  96(\frame)
    flw     fs9,  100(\frame)
    flw     fs10, 104(\frame)
    flw     fs11, 108(\frame)
    flw     ft8,  112(\frame)
    flw     ft9,  116(\frame)
    flw     ft10, 120(\frame)
    flw     ft11, 124(\frame)
    lw      t1,   128(\frame)
    csrw    fcsr, t1
.endm


.macro fpu_read_dirty_bit reg
    csrr    \reg, mstatus
    srli    \reg, \reg, 13
    andi    \reg, \reg, 1
.endm


.macro fpu_clear_dirty_bit reg
    li      \reg, 1 << 13
    csrc    mstatus, \reg
.endm


.macro fpu_enable reg
    li     \reg, CSR_MSTATUS_FPU_ENABLE
    csrs   mstatus, \reg
    or a0, a0, \reg
.endm


.macro fpu_disable reg
    li     \reg, CSR_MSTATUS_FPU_DISABLE
    csrc   mstatus, \reg
.endm
#endif


.section .text
    .global rtos_int_enter
    .global fpu_ctx
    .type rtos_int_enter, @function
rtos_int_enter:
    add_intr_nesting 1
    #if CONFIG_ESP_AMP_SUBCORE_ENABLE_HW_FPU
    #if CONFIG_ESP_AMP_SUBCORE_ENABLE_HW_FPU_IN_ISR
    fpu_save_regs t0
    #else
    fpu_disable t0
    #endif
    #endif
    ret

    .global rtos_int_exit
    .type rtos_int_exit, @function
rtos_int_exit:
    #if CONFIG_ESP_AMP_SUBCORE_ENABLE_HW_FPU
    fpu_enable t0
    #if CONFIG_ESP_AMP_SUBCORE_ENABLE_HW_FPU_IN_ISR
    fpu_restore_regs t0
    #endif
    #endif
    add_intr_nesting -1
    ret

    .global rtos_save_pie_coproc
    .type rtos_save_pie_coproc, @function
rtos_save_pie_coproc:
    ebreak
    j rtos_save_pie_coproc


    .global rtos_save_fpu_coproc
    .type rtos_save_fpu_coproc, @function
rtos_save_fpu_coproc:
    ebreak
    j rtos_save_fpu_coproc

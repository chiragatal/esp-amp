/*
 * SPDX-FileCopyrightText: 2023 Espressif Systems (Shanghai) CO LTD
 *
 * SPDX-License-Identifier: Apache-2.0
 */

	.section .text.vectors
	.global reset_vector

/* The reset vector, jumps to startup code */
reset_vector:

    /* _vector_table: Only 256-byte aligned addresses are allowed */
    la      t0, _vector_table
    csrw    mtvec, t0

	j __start

__start:
	/* setup the stack pointer */
	la sp, __stack_top

    la      a0, __libc_fini_array   # Register global termination functions
    call    __libc_init_array       # Run global initialization functions

	call lp_core_startup

    /* These don't have to do anything since we use init_array/fini_array. */
    .global _init
    .type   _init, @function
    .global _fini
    .type   _fini, @function
_init:
_fini:
    ret

    .size  _init, .-_init
    .size  _fini, .-_fini

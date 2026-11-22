//==- x86_64_layout.h - x86-64 Context Record Layout -----------------------==//
//
// Copyright (c) 2026 Ian Pike
// Part of fiber_context, under the Apache License v2.0 with LLVM Exceptions.
// See LICENSE in the project root for license information.
// SPDX-License-Identifier: Apache-2.0 WITH LLVM-exception
//
//===----------------------------------------------------------------------===//
//
// Defines the assembly offsets and initializes the matching C++ record for the
// bootstrap's first call.
//
//===----------------------------------------------------------------------===//

// Include guard is needed for C++ and assembly.
#ifndef FIBER_CONTEXT_DETAIL_X86_64_LAYOUT_H
#define FIBER_CONTEXT_DETAIL_X86_64_LAYOUT_H

#if FIBER_CONTEXT_ASM_CANARIES
    // Record byte 0 holds this record's address.
    #define FIBER_CONTEXT_CTX_HEAD 0

    // The register block starts at record byte 8 with the saved RBX.
    #define FIBER_CONTEXT_CTX_RBX 8

    // Record byte 80 holds the saved SP XOR the record address.
    #define FIBER_CONTEXT_CTX_SP_WITNESS 80

    // Record byte 88 holds the saved PC XOR the record address.
    #define FIBER_CONTEXT_CTX_PC_WITNESS 88

    // When bounds are set, all 8 return-slot bytes must fit in [LOW, HIGH).

    // Record byte 96 holds the lowest usable stack address.
    #define FIBER_CONTEXT_CTX_LOW 96

    // Record byte 104 holds the address just past the usable stack.
    #define FIBER_CONTEXT_CTX_HIGH 104

    // Record byte 112 points to the stack band or holds zero.
    #define FIBER_CONTEXT_CTX_BAND 112

    // Record byte 120 holds the band pointer XOR the record address.
    #define FIBER_CONTEXT_CTX_BAND_WITNESS 120

    // Record byte 128 holds LOW XOR HIGH XOR the record address.
    #define FIBER_CONTEXT_CTX_BOUNDS_WITNESS 128

    // Record byte 136 holds the complement of the record address.
    #define FIBER_CONTEXT_CTX_TAIL 136

    // The canary record occupies 144 bytes.
    #define FIBER_CONTEXT_CTX_SIZE 144

#else
    // The register block starts at record byte 0 with the saved RBX.
    #define FIBER_CONTEXT_CTX_RBX 0

    // The plain record occupies 80 bytes.
    #define FIBER_CONTEXT_CTX_SIZE 80

#endif

// Block byte 8 holds the saved RBP.
#define FIBER_CONTEXT_CTX_RBP (FIBER_CONTEXT_CTX_RBX + 8)

// Block byte 16 holds R12 or the entry address for the first switch.
#define FIBER_CONTEXT_CTX_R12 (FIBER_CONTEXT_CTX_RBX + 16)

// Block byte 24 holds the saved R13.
#define FIBER_CONTEXT_CTX_R13 (FIBER_CONTEXT_CTX_RBX + 24)

// Block byte 32 holds the saved R14.
#define FIBER_CONTEXT_CTX_R14 (FIBER_CONTEXT_CTX_RBX + 32)

// Block byte 40 holds the saved R15.
#define FIBER_CONTEXT_CTX_R15 (FIBER_CONTEXT_CTX_RBX + 40)

// Block byte 48 holds the address of the return slot on the stack.
#define FIBER_CONTEXT_CTX_SP (FIBER_CONTEXT_CTX_RBX + 48)

// Block byte 56 holds the bootstrap or resume address for ret.
#define FIBER_CONTEXT_CTX_PC (FIBER_CONTEXT_CTX_RBX + 56)

// Block byte 64 holds the SSE control and status register.
#define FIBER_CONTEXT_CTX_MXCSR (FIBER_CONTEXT_CTX_RBX + 64)

// Block byte 68 holds the x87 control word.
#define FIBER_CONTEXT_CTX_X87CW (FIBER_CONTEXT_CTX_RBX + 68)

#ifndef __ASSEMBLER__
    #include <__fiber_context/config.h>

    #include "asm/helpers/x86_64/fp_control.h"
    #include "asm/symbols.h"

    #include <cstddef>
    #include <cstdint>
    #include <cstring>

struct alignas(16) fiber_context_x86_record
{
    #if FIBER_CONTEXT_ASM_CANARIES
    std::uintptr_t head; // Record address for the leading canary.
    #endif

    std::uintptr_t rbx; // Callee-saved RBX.
    std::uintptr_t rbp; // Callee-saved RBP which may hold the frame pointer.
    std::uintptr_t r12; // Callee-saved R12 holds the entry address on first use.
    std::uintptr_t r13; // Callee-saved R13.
    std::uintptr_t r14; // Callee-saved R14.
    std::uintptr_t r15; // Callee-saved R15.

    std::uintptr_t sp; // Address of the eight-byte return slot.
    std::uintptr_t pc; // Bootstrap or resume address written to the return slot.

    std::uint32_t mxcsr; // Saved SSE control and status register.
    std::uint16_t x87cw; // Saved x87 control word.

    #if FIBER_CONTEXT_ASM_CANARIES
    std::uintptr_t sp_witness;     // Saved SP XOR the record address.
    std::uintptr_t pc_witness;     // Saved PC XOR the record address.
    std::uintptr_t low;            // First usable stack address.
    std::uintptr_t high;           // One past the usable stack.
    std::uintptr_t band;           // Address of the two stack canaries or zero.
    std::uintptr_t band_witness;   // Band address XOR the record address.
    std::uintptr_t bounds_witness; // Low bound XOR high bound XOR the record address.
    std::uintptr_t tail;           // Bitwise complement of the record address.
    #endif
};

    // Each C++ field must match its assembly byte offset.
    #define FIBER_CONTEXT_X86_OFFSET(field, value) static_assert(offsetof(fiber_context_x86_record, field) == (value))

FIBER_CONTEXT_X86_OFFSET(rbx, FIBER_CONTEXT_CTX_RBX);
FIBER_CONTEXT_X86_OFFSET(rbp, FIBER_CONTEXT_CTX_RBP);
FIBER_CONTEXT_X86_OFFSET(r12, FIBER_CONTEXT_CTX_R12);
FIBER_CONTEXT_X86_OFFSET(r13, FIBER_CONTEXT_CTX_R13);
FIBER_CONTEXT_X86_OFFSET(r14, FIBER_CONTEXT_CTX_R14);
FIBER_CONTEXT_X86_OFFSET(r15, FIBER_CONTEXT_CTX_R15);
FIBER_CONTEXT_X86_OFFSET(sp, FIBER_CONTEXT_CTX_SP);
FIBER_CONTEXT_X86_OFFSET(pc, FIBER_CONTEXT_CTX_PC);
FIBER_CONTEXT_X86_OFFSET(mxcsr, FIBER_CONTEXT_CTX_MXCSR);
FIBER_CONTEXT_X86_OFFSET(x87cw, FIBER_CONTEXT_CTX_X87CW);

    #if FIBER_CONTEXT_ASM_CANARIES
FIBER_CONTEXT_X86_OFFSET(head, FIBER_CONTEXT_CTX_HEAD);
FIBER_CONTEXT_X86_OFFSET(tail, FIBER_CONTEXT_CTX_TAIL);
FIBER_CONTEXT_X86_OFFSET(sp_witness, FIBER_CONTEXT_CTX_SP_WITNESS);
FIBER_CONTEXT_X86_OFFSET(pc_witness, FIBER_CONTEXT_CTX_PC_WITNESS);
FIBER_CONTEXT_X86_OFFSET(low, FIBER_CONTEXT_CTX_LOW);
FIBER_CONTEXT_X86_OFFSET(high, FIBER_CONTEXT_CTX_HIGH);
FIBER_CONTEXT_X86_OFFSET(band, FIBER_CONTEXT_CTX_BAND);
FIBER_CONTEXT_X86_OFFSET(band_witness, FIBER_CONTEXT_CTX_BAND_WITNESS);
FIBER_CONTEXT_X86_OFFSET(bounds_witness, FIBER_CONTEXT_CTX_BOUNDS_WITNESS);
    #endif

    #undef FIBER_CONTEXT_X86_OFFSET

static_assert(sizeof(fiber_context_x86_record) == FIBER_CONTEXT_CTX_SIZE);

STDX_FIBER_CONTEXT_NAMESPACE::native
{
    inline void fiber_context_x86_initialize(fiber_context_x86_record &record, void (*entry)(), void *sp, [[maybe_unused]] void *low, [[maybe_unused]] void *high,
        [[maybe_unused]] std::uintptr_t *band) noexcept
    {
        record = {};
        if (entry != nullptr)
        {
            // The first ret enters the bootstrap, which calls R12.
            record.sp  = reinterpret_cast<std::uintptr_t>(sp);
            record.pc  = reinterpret_cast<std::uintptr_t>(&asm_bridge::x86_bootstrap);
            record.r12 = reinterpret_cast<std::uintptr_t>(entry);
        }

        // New fibers inherit their creator's SSE and x87 control settings.
        fiber_context_x86_read_fp_control(&record.mxcsr, &record.x87cw);

    #if FIBER_CONTEXT_ASM_CANARIES
        const auto address = reinterpret_cast<std::uintptr_t>(&record);
        record.head        = address;
        record.tail        = ~address;

        // Each witness ties its saved values to this record's address.
        record.sp_witness     = record.sp ^ address;
        record.pc_witness     = record.pc ^ address;
        record.low            = reinterpret_cast<std::uintptr_t>(low);
        record.high           = reinterpret_cast<std::uintptr_t>(high);
        record.band           = reinterpret_cast<std::uintptr_t>(band);
        record.band_witness   = record.band ^ address;
        record.bounds_witness = record.low ^ record.high ^ address;
        if (band != nullptr)
        {
            const std::uintptr_t markers[] = {record.band, ~record.band};
            std::memcpy(band, markers, sizeof(markers));
        }
    #endif
    }
}
#endif
#endif

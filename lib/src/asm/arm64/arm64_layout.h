//==- arm64_layout.h - ARM64 Context Record Layout -------------------------==//
//
// Copyright (c) 2026 Ian Pike
// Part of fiber_context, under the Apache License v2.0 with LLVM Exceptions.
// See LICENSE in the project root for license information.
// SPDX-License-Identifier: Apache-2.0 WITH LLVM-exception
//
//===----------------------------------------------------------------------===//
//
// Defines the assembly offsets and initializes the matching C++ record with the
// entry address and stack pointer and the initial value of FPCR.
//
//===----------------------------------------------------------------------===//

#pragma once

/// The head marker and band pointer occupy the first 16 bytes in canary builds.
#if FIBER_CONTEXT_ASM_CANARIES

    // Record byte 0 holds this record's address.
    #define FIBER_CONTEXT_CTX_HEAD 0

    // Record byte 8 points to the stack band or holds zero.
    #define FIBER_CONTEXT_CTX_BAND 8

    // Register block starts at record byte 16 after the canary fields.
    #define FIBER_CONTEXT_ARM_REGS 16

#else

    // Register block starts at record byte 0.
    #define FIBER_CONTEXT_ARM_REGS 0

#endif

/// Each X offset names the first register in a paired store.

// Block bytes 0 and 8 save x19 and x20.
#define FIBER_CONTEXT_ARM_X19 (FIBER_CONTEXT_ARM_REGS + 0)

// Block bytes 16 and 24 save x21 and x22.
#define FIBER_CONTEXT_ARM_X21 (FIBER_CONTEXT_ARM_REGS + 16)

// Block bytes 32 and 40 save x23 and x24.
#define FIBER_CONTEXT_ARM_X23 (FIBER_CONTEXT_ARM_REGS + 32)

// Block bytes 48 and 56 save x25 and x26.
#define FIBER_CONTEXT_ARM_X25 (FIBER_CONTEXT_ARM_REGS + 48)

// Block bytes 64 and 72 save x27 and x28.
#define FIBER_CONTEXT_ARM_X27 (FIBER_CONTEXT_ARM_REGS + 64)

// Block bytes 80 and 88 save frame pointer x29 and link register x30.
#define FIBER_CONTEXT_ARM_X29 (FIBER_CONTEXT_ARM_REGS + 80)

/// PC holds the entry point on first use and the saved x30 on resume.

// Block byte 88 holds the entry or resume address.
#define FIBER_CONTEXT_CTX_PC (FIBER_CONTEXT_ARM_REGS + 88)

/// The ABI preserves only the low 64 bits of v8 through v15.
/// https://github.com/ARM-software/abi-aa/blob/2025Q1/aapcs64/aapcs64.rst#L874-L898

// Block bytes 96 and 104 save d8 and d9.
#define FIBER_CONTEXT_ARM_D8 (FIBER_CONTEXT_ARM_REGS + 96)

// Block bytes 112 and 120 save d10 and d11.
#define FIBER_CONTEXT_ARM_D10 (FIBER_CONTEXT_ARM_REGS + 112)

// Block bytes 128 and 136 save d12 and d13.
#define FIBER_CONTEXT_ARM_D12 (FIBER_CONTEXT_ARM_REGS + 128)

// Block bytes 144 and 152 save d14 and d15.
#define FIBER_CONTEXT_ARM_D14 (FIBER_CONTEXT_ARM_REGS + 144)

// Block byte 160 holds the saved stack pointer.
#define FIBER_CONTEXT_CTX_SP (FIBER_CONTEXT_ARM_REGS + 160)

// Block byte 168 holds the floating-point control register.
#define FIBER_CONTEXT_CTX_FPCR (FIBER_CONTEXT_ARM_REGS + 168)

#if FIBER_CONTEXT_ASM_CANARIES

    /// SP may equal HIGH on entry. Two zero bounds disable the range check.

    // Record byte 192 holds the lowest usable stack address.
    #define FIBER_CONTEXT_CTX_LOW 192

    // Record byte 200 holds the address just past the usable stack.
    #define FIBER_CONTEXT_CTX_HIGH 200

    // Record byte 208 holds the saved SP XOR the record address.
    #define FIBER_CONTEXT_CTX_SP_WITNESS 208

    // Record byte 216 holds the saved PC XOR the record address.
    #define FIBER_CONTEXT_CTX_PC_WITNESS 216

    // Record byte 224 holds the complement of the band pointer.
    #define FIBER_CONTEXT_ARM_BAND_WITNESS 224

    // Record byte 232 holds the complement of the record address.
    #define FIBER_CONTEXT_CTX_TAIL 232

    // The canary record occupies 240 bytes.
    #define FIBER_CONTEXT_CTX_SIZE 240

#else

    // The plain record occupies 176 bytes.
    #define FIBER_CONTEXT_CTX_SIZE 176

#endif

#ifndef __ASSEMBLER__
    #include "../helpers/arm/fpcr.h"

    #include <cstddef>
    #include <cstdint>

struct alignas(16) fiber_context_arm_record
{
    #if FIBER_CONTEXT_ASM_CANARIES

    std::uintptr_t head; // Record address for the leading canary.
    std::uintptr_t band; // Address of the two stack canaries or zero.

    #endif // FIBER_CONTEXT_ASM_CANARIES

    std::uintptr_t x19_to_x29[11]; // Callee-saved integer registers including the frame pointer.
    std::uintptr_t pc;             // Entry address on first use or saved x30 on resume.
    std::uint64_t d8_to_d15[8];    // Preserved low halves of v8 through v15.
    std::uintptr_t sp;             // Stack pointer restored on entry or resume.
    std::uint64_t fpcr;            // Floating-point control settings restored on resume.

    #if FIBER_CONTEXT_ASM_CANARIES

    std::uintptr_t low;          // First usable stack address.
    std::uintptr_t high;         // One past the usable stack and a valid initial SP.
    std::uintptr_t sp_witness;   // Saved SP XOR the record address.
    std::uintptr_t pc_witness;   // Saved PC XOR the record address.
    std::uintptr_t band_witness; // Bitwise complement of the band address.
    std::uintptr_t tail;         // Bitwise complement of the record address.

    #endif // FIBER_CONTEXT_ASM_CANARIES
};

static_assert(sizeof(fiber_context_arm_record) == FIBER_CONTEXT_CTX_SIZE);
static_assert(offsetof(fiber_context_arm_record, x19_to_x29) == FIBER_CONTEXT_ARM_X19);
static_assert(offsetof(fiber_context_arm_record, pc) == FIBER_CONTEXT_CTX_PC);
static_assert(offsetof(fiber_context_arm_record, d8_to_d15) == FIBER_CONTEXT_ARM_D8);
static_assert(offsetof(fiber_context_arm_record, sp) == FIBER_CONTEXT_CTX_SP);
static_assert(offsetof(fiber_context_arm_record, fpcr) == FIBER_CONTEXT_CTX_FPCR);
    #if FIBER_CONTEXT_ASM_CANARIES
static_assert(offsetof(fiber_context_arm_record, head) == FIBER_CONTEXT_CTX_HEAD);
static_assert(offsetof(fiber_context_arm_record, band) == FIBER_CONTEXT_CTX_BAND);
static_assert(offsetof(fiber_context_arm_record, low) == FIBER_CONTEXT_CTX_LOW);
static_assert(offsetof(fiber_context_arm_record, high) == FIBER_CONTEXT_CTX_HIGH);
static_assert(offsetof(fiber_context_arm_record, sp_witness) == FIBER_CONTEXT_CTX_SP_WITNESS);
static_assert(offsetof(fiber_context_arm_record, pc_witness) == FIBER_CONTEXT_CTX_PC_WITNESS);
static_assert(offsetof(fiber_context_arm_record, band_witness) == FIBER_CONTEXT_ARM_BAND_WITNESS);
static_assert(offsetof(fiber_context_arm_record, tail) == FIBER_CONTEXT_CTX_TAIL);
    #endif // FIBER_CONTEXT_ASM_CANARIES

inline void fiber_context_arm_initialize(fiber_context_arm_record &record, void (*entry)(), void *sp, [[maybe_unused]] void *low, [[maybe_unused]] void *high,
    [[maybe_unused]] std::uintptr_t *band) noexcept
{
    record    = {};
    record.pc = reinterpret_cast<std::uintptr_t>(entry);
    record.sp = reinterpret_cast<std::uintptr_t>(sp);

    // A new fiber inherits its creator's floating-point control settings.
    record.fpcr = fiber_context_arm_read_fpcr();

    #if FIBER_CONTEXT_ASM_CANARIES

    /*
     * The head and tail mark the ends of the context record.
     * When present, the band marks the bottom of the fiber stack with its address and its complement.
     */
    const auto address = reinterpret_cast<std::uintptr_t>(&record);
    record.head        = address;
    record.tail        = ~address;

    // The witnesses bind the saved SP and PC to this record's address.
    record.sp_witness   = record.sp ^ address;
    record.pc_witness   = record.pc ^ address;
    record.low          = reinterpret_cast<std::uintptr_t>(low);
    record.high         = reinterpret_cast<std::uintptr_t>(high);
    record.band         = reinterpret_cast<std::uintptr_t>(band);
    record.band_witness = ~record.band;

    if (band != nullptr)
    {
        band[0] = record.band;
        band[1] = ~record.band;
    }

    #endif // FIBER_CONTEXT_ASM_CANARIES
}
#endif

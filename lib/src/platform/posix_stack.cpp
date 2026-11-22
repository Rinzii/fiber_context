//==- posix_stack.cpp - Guarded POSIX Stacks -------------------------------==//
//
// Copyright (c) 2026 Ian Pike
// Part of fiber_context, under the Apache License v2.0 with LLVM Exceptions.
// See LICENSE in the project root for license information.
// SPDX-License-Identifier: Apache-2.0 WITH LLVM-exception
//
//===----------------------------------------------------------------------===//
//
// Maps each owned stack above an inaccessible guard page and unmaps it on
// destruction.
//
//===----------------------------------------------------------------------===//

#include "native/context.h"

#include <__fiber_context/config.h>

#include <cerrno>
#include <new>
#include <system_error>

#include <sys/mman.h>
#include <unistd.h>

STDX_FIBER_CONTEXT_NAMESPACE::native
{
    namespace
    {
        constexpr std::size_t default_size = 512 * 1024;
    } // namespace

    stack::stack()
    {
        const long page_result = sysconf(_SC_PAGESIZE);
        if (page_result <= 0)
        {
            throw std::system_error(std::make_error_code(std::errc::not_supported), "page size");
        }

        const auto page            = static_cast<std::size_t>(page_result);
        const unsigned long usable = ((default_size + page - 1) / page) * page;
        m_mapping_size             = page + usable;
        m_mapping                  = mmap(nullptr, m_mapping_size, PROT_NONE, MAP_PRIVATE | MAP_ANONYMOUS, -1, 0);

        if (m_mapping == MAP_FAILED)
        {
            if (errno == ENOMEM)
            {
                throw std::bad_alloc{};
            }

            throw std::system_error(errno, std::generic_category(), "mmap fiber stack");
        }

        auto *base = static_cast<std::byte *>(m_mapping) + page;
        if (mprotect(base, usable, PROT_READ | PROT_WRITE) != 0)
        {
            const int error = errno;
            munmap(m_mapping, m_mapping_size);

            throw std::system_error(error, std::generic_category(), "mprotect fiber stack");
        }

        m_bytes = {base, usable};
    }

    stack::~stack() { munmap(m_mapping, m_mapping_size); }

} // namespace stdx::native

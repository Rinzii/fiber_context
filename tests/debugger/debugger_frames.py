#==- debugger_frames.py - Saved Frame Walk Tests ----------------------------==#
#
# Copyright (c) 2026 Ian Pike
# Part of fiber_context, under the Apache License v2.0 with LLVM Exceptions.
# See LICENSE in the project root for license information.
# SPDX-License-Identifier: Apache-2.0 WITH LLVM-exception
#
#===------------------------------------------------------------------------===#
#
# Checks that a parked fiber's backtrace stays within stack bounds and stops at
# an invalid frame link.
#
#===------------------------------------------------------------------------===#

from pathlib import Path
import sys
import unittest

sys.path.insert(0, str(Path(__file__).resolve().parents[2] / "tools"))
from fiber_context_debug import Fiber, fiber_type, frame_addresses, summary


class ParkedFrames(unittest.TestCase):
    parked = Fiber(context=0x1000, owner=1, stack="fiber", low=0x2000, high=0x3000,
                   sp=0x2800, fp=0x2900, pc=0x4000)

    def test_fiber_type_with_optional_abi_namespace(self):
        for name in ("stdx::fiber_context", "stdx::v1::fiber_context", "stdx::release_42::fiber_context"):
            self.assertIsNotNone(fiber_type.fullmatch(name))

        for name in ("other::fiber_context", "stdx::v1::nested::fiber_context", "stdx::1::fiber_context"):
            self.assertIsNone(fiber_type.fullmatch(name))

    def test_walk_stops_at_stack_top(self):
        saved = {0x2900: 0x2ff0, 0x2908: 0x4100, 0x2ff0: 0x3000, 0x2ff8: 0x4200}
        self.assertEqual(list(frame_addresses(self.parked, saved.__getitem__)), [0x4000, 0x4100, 0x4200])

    def test_invalid_frames_are_never_read(self):
        for fp in (0, 0x2700, 0x2901, 0x2ff8, 0x3000):
            with self.subTest(fp=fp):
                self.assertEqual(list(frame_addresses(self.parked._replace(fp=fp), self.fail)), [0x4000])

        for caller in (0x2800, 0x2900, 0x2908):
            saved = {0x2900: caller, 0x2908: 0x4100}
            self.assertEqual(list(frame_addresses(self.parked, saved.__getitem__)), [0x4000, 0x4100])

        for sp in (0x1ff0, 0x3010):
            with self.assertRaises(ValueError):
                list(frame_addresses(self.parked._replace(sp=sp), self.fail))

        for low, high in ((0, 0x3000), (0x2800, 0x2800), (0x3000, 0x2000)):
            with self.assertRaises(ValueError):
                list(frame_addresses(self.parked._replace(low=low, high=high), self.fail))

    def test_walk_limits_a_long_chain(self):
        fiber = self.parked._replace(high=0x8000)

        def read_pointer(address):
            return address + 16 if address % 16 == 0 else 0x4100

        self.assertEqual(len(list(frame_addresses(fiber, read_pointer))), 128)

    def test_stack_without_bounds_reports_only_the_saved_pc(self):
        thread = self.parked._replace(stack="thread", low=0, high=0)
        self.assertEqual(list(frame_addresses(thread, self.fail)), [thread.pc])
        self.assertIn("bounds unknown", summary(thread))

    def test_summary_reports_the_active_stack(self):
        self.assertEqual(summary(None), "empty")
        self.assertIn("not started", summary(self.parked._replace(owner=0)))
        self.assertIn("parked, cleanup stack [0x2000, 0x3000) (4096 bytes)",
                      summary(self.parked._replace(stack="cleanup")))

    def test_handles_without_a_continuation(self):
        for fiber in (None, self.parked._replace(owner=0), self.parked._replace(pc=0)):
            with self.assertRaises(ValueError):
                list(frame_addresses(fiber, self.fail))


if __name__ == "__main__":
    unittest.main()

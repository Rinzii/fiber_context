#==- debugger_lldb.py - LLDB Fiber Integration Test -------------------------==#
#
# Copyright (c) 2026 Ian Pike
# Part of fiber_context, under the Apache License v2.0 with LLVM Exceptions.
# See LICENSE in the project root for license information.
# SPDX-License-Identifier: Apache-2.0 WITH LLVM-exception
#
#===------------------------------------------------------------------------===#
#
# Stops the ping-pong example in LLDB and checks the debugger's summary and
# backtrace for the parked fiber.
#
#===------------------------------------------------------------------------===#

from pathlib import Path
import subprocess
import sys

lldb, example, config = sys.argv[1:]
if config not in ("Debug", "RelWithDebInfo"):
    sys.exit(77)

root = Path(__file__).resolve().parents[2]
source = (root / "examples/ping_pong.cpp").read_text().splitlines()
stop = next(n for n, line in enumerate(source, 1) if "back in main" in line)
yield_line = next(n for n, line in enumerate(source, 1) if "caller = std::move(caller).resume()" in line)
run = subprocess.run(
    [lldb, "-b", "-x", example,
     "-o", "settings set target.disable-aslr false",
     "-o", "command script import tools/fiber_context_lldb.py",
     "-o", f"breakpoint set --file ping_pong.cpp --line {stop}",
     "-o", "run", "-o", "frame variable worker", "-o", "fiber-bt worker"],
    cwd=root, stdout=subprocess.PIPE, stderr=subprocess.STDOUT, text=True,
)

print(run.stdout)
if run.returncode:
    if any(reason in run.stdout.lower() for reason in ("operation not permitted", "not allowed to attach")):
        sys.exit(77)

    sys.exit(run.returncode)

frames = [line for line in run.stdout.splitlines() if line.startswith("#")]
if "worker = parked, fiber stack" not in run.stdout or not any(f"ping_pong.cpp:{yield_line}" in line for line in frames):
    sys.exit("LLDB did not resolve the parked worker's suspension line")

# ATmega328P shared prescaler: native regression reproducer

## Status (2026-11-19)

This branch includes the **candidate** shared synchronous prescaler fix in
`avr_timer.c`, `avr_timer.h` and `sim_megax8.h`, together with **five**
ATmega328P native regressions. A fresh independent Linux source build and
native test run **passed 29/29 tests**, including the staggered start phase
cases, without using GitHub Actions.

The run used a disposable Railway build of branch
`test/weightos-railway-prescaler-gate` at
`cf58418cb234742a66d875088a011039beb58e1b`. This test branch was
based on the fix and provided a Dockerfile plus WeightOS fixture probes.
The new native phase regression is also committed directly on the fix branch.
**Do not yet open a ready-for-review upstream PR**: physical timing,
PWM/CTC, Timer2 PSRASY and cross-device behavior are not fully validated.

## What the test proves

The ATmega328P Timer0 and Timer1 must be suspended while GTCCR contains
\`TSM | PSRSYNC\`. The test's guest AVR firmware:

1. Enables Timer0 and Timer1 with CPU clock /8.
2. Waits 32 NOP instructions.
3. Writes \`0x81\` to GTCCR (\`0x43\` data space).
4. Reads the real guest-visible \`GTCCR\`, \`TCNT0\`, and \`TCNT1L\` registers.
5. Executes 80 NOP instructions and reads both counters again.
6. Signals completion by writing \`0xA5\` to SRAM data address \`0x100\`.

The native C harness runs the real \`atmega328p\` CPU in \`libsimavr\`, then
examines the GPRs populated by guest \`IN\` and \`LDS\` instructions. It
does not inspect stale \`avr->data[TCNTx]\` memory as a substitute for a timer read.

## Verified baseline failure

Compiled with \`cc -O2 -std=c11 -Wall -Wextra -Werror\` against the *official*
upstream Linux x86-64 build from 2026-10-13 (commit
\`5b08da60497f7564f50941f74a549fe2851db030\`, CI artifact
\`11796061327\`). Result:

\`\`\`text
observed: GTCCR=0x81 TCNT0=4->15 TCNT1L=4->15
TEST FAILURE: Timer0 counted during TSM: first=4, second=15
\`\`\`

The host test exited with code **1**, as expected for the unfixed version.
The source code on current \`master\` still uses per-timer \`tov_base\`; the
failure has not yet been rerun with a fresh binary built from current \`master\`.

## Build/run

This file is automatically compiled by the existing \`tests/Makefile\` rule
for \`test_*.c\`. Once the library is built:

\`\`\`sh
make -C tests t
\`\`\`

The guest \`.text\` bytes are embedded in \`test_atmega328p_shared_prescaler.c\`
to avoid requiring an AVR assembler when running native tests. The matching
human-readable AVR source is \`atmega328p_tsm_prescaler.S\`; to regenerate:

\`\`\`sh
clang --target=avr -mmcu=atmega328p -c \
  tests/atmega328p_tsm_prescaler.S -o /tmp/tsm.o
llvm-objcopy -O binary -j .text /tmp/tsm.o /tmp/tsm.bin
\`\`\`

Check that the output has **516 bytes** and matches the embedded designated
byte initializers in the native C test before changing the firmware.

## Second independent regression: resume on release

\`atmega328p_tsm_release.S\` and
\`test_atmega328p_tsm_release.c\` add a second acceptance condition:

- Both counters remain unchanged while GTCCR holds \`TSM|PSRSYNC\`.
- When firmware writes zero to GTCCR, they resume counting after 32 NOPs.
- GTCCR reads back zero after release.

The same official upstream binary failed this second test with both counter
pairs at \`4->15\` during the hold. This proves that an implementation which
only freezes the counters but never resumes them will not satisfy the
regression suite.


## Additional regression and current implementation scope

- `atmega328p_tsm_configure.S` and `test_atmega328p_tsm_configure.c`
  require TCNT0 and TCNT1 to accept writes while suspended, remain stable
  during an 80-NOP hold, and resume after release. The exact embedded binary
  was generated using LLVM AVR and the baseline official library fails it.
- The implementation records the shared synchronization epoch and uses it
  to align Timer0 and Timer1 when started at different cycles. It holds
  timer event scheduling during TSM, preserves TCNT values, and handles
  standalone PSRSYNC reset as a strobe.
- **Important limitation:** PSRASY / Timer2 asynchronous reset is not
  implemented by this candidate. Regressions across this and other chip
  families must pass before any upstream PR.

## How the candidate will be tested

A dedicated Linux workflow is present at
`.github/workflows/atmega328p-prescaler-regression.yml`. It checks out
the **fork branch**, builds libsimavr from source, and runs the repository's
native tests. It uses one standard public-repository Linux runner, with a
12-minute job timeout and read-only token.

The fork's Actions have now attempted to run. One representative workflow
execution is https://github.com/pedrosaunu/simavr/actions/runs/44452958065.
It failed **before any runner was assigned**: zero job steps, empty runner
name, runner_id 0, completed in approximately two seconds and no downloadable
job logs. This is **NOT a C build or functional-test result**; do not
diagnose it as a compiler error. The Actions/billing/runner-provisioning
restriction must be resolved independently before interpreting CI as pass/fail.

The four original host-side regressions demonstrated **RED** against the
official 2026-10-13 upstream library. A fresh source build of the patched
fork independently demonstrated **GREEN**, and a fifth native regression
confirmed the shared divider phase when Timer0/Timer1 start at different
CPU cycles:

- Staggered /8 probes: original `TCNT0/TCNT1L=1/0` and `2/1`;
  corrected fork `1/1` and `2/2` using the same guest AVR code
- `GTCCR.PSRSYNC`: original remains `1`; corrected fork self-clears to `0`
- Native library suite: **29 passed, 0 failed**, compiled with GCC/AVR-GCC
  in a Debian-based isolated Railway build without GitHub Actions
- WeightOS v1.26 repeated-IRQ probe: **13 vectors (7 Timer0, 6 Timer1)**,
  with exactly the same event-cycle and saved-return-PC values as the
  original `simavr`. This patch does **not** fix the separate Timer1
  repeated-interrupt timing discrepancy because that firmware uses /1

The source-build verification is **not** a silicon timing measurement, and
the fork has not passed exhaustive PWM/CTC/overflow and cross-device tests.
Preserve the upstream version as an independent reference; do not replace
the WeightOS temporal oracle solely with the corrected fork.

## Requirements for the eventual fix

- Shared, free-running synchronous timer clock for Timer0 and Timer1, not
  merely independent resets of both timers' counters.
- Correct GTCCR TSM / PSRSYNC hold, clear and release semantics.
- Do not conflate Timer2's asynchronous PSRASY clock.
- Check normal mode, PWM/CTC, different prescaler taps, overflow and ISR
  regressions, and actual chip timing where simulator behavior is ambiguous.

Related upstream: issue #169 and closed unmerged PR #601.
Reference: Microchip ATmega328P datasheet synchronous timer prescaler section.

**Important**: a regression-only commit is not presented as a fix.
# ATmega328P shared prescaler: native regression reproducer

## Status

This branch currently contains a **failing test**, not a fix. Please do not merge
or open a ready-for-review upstream PR before the timer implementation is corrected.

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